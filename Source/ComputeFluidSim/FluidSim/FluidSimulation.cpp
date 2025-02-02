#include "FluidSimulation.h"

#include "RenderGraph.h"
#include "GlobalShader.h"
#include "RHI.h"
#include "TextureResource.h"
#include "Engine/TextureRenderTargetVolume.h"
#include "FluidSimLog.h"
#include "FluidShaderImplementation.h"

// This will tell the engine to create the shader and where the shader entry point is.
//                            ShaderType                            ShaderPath                     Shader function name    Type
IMPLEMENT_GLOBAL_SHADER(FObjectGPUAdvectionShader,			"/DynamicsShaders/FluidSimShader.usf", "AdvectionShader",		SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FObjectGPUDissipationShader,		"/DynamicsShaders/FluidSimShader.usf", "DissipationShader",		SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FObjectGPUDiffusionShader,			"/DynamicsShaders/FluidSimShader.usf", "DiffusionShader",		SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FObjectGPUDivergenceShader,			"/DynamicsShaders/FluidSimShader.usf", "DivergenceShader",		SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FObjectGPUProjectPressureShader,	"/DynamicsShaders/FluidSimShader.usf", "ProjPressureShader",	SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FObjectGPUProjectGradientShader,	"/DynamicsShaders/FluidSimShader.usf", "ProjGradientVelShader", SF_Compute);

// Injection
IMPLEMENT_GLOBAL_SHADER(FObjectGPUInjectionShader, "/DynamicsShaders/FluidSimInjectionShader.usf", "InjectionShader", SF_Compute);


bool UFluidSimulation::Setup(const FGridDescription& Desc, const FContentBrowserTextures& CBTexts)
{
	GridDescription = Desc;
	RT_Density_Vol = CBTexts.RT_Density_Vol;
	RT_Velocity_Vol = CBTexts.RT_Velocity_Vol;
	RT_Pressure_Vol = CBTexts.RT_Pressure_Vol;
	RT_Divergence_Vol = CBTexts.RT_Divergence_Vol;
	
	GroupCount = FIntVector(32,32,32); // Can expose if needed.

	// Validate injection events at start.
	ResetInjectionEvents();

	// Makes sure the GPU Is ready
	if (IsInRenderingThread()) {
		SetupRenderThread(GetImmediateCommandList_ForRenderCommand());
	}
	else
	{
		ENQUEUE_RENDER_COMMAND(GPUFluidSimCommand)(
			[this](FRHICommandListImmediate& RHICmdList)
			{
				UFluidSimulation::SetupRenderThread(RHICmdList);
			});
	}

	return IsValid(RT_Density_Vol) && IsValid(RT_Velocity_Vol) && IsValid(RT_Pressure_Vol) && IsValid(RT_Divergence_Vol) ; 
}

void UFluidSimulation::SetupRenderThread(FRHICommandListImmediate& RHICmdList)
{
	StopRenderThread(RHICmdList);
	
	constexpr EPixelFormat TextureFormat = EPixelFormat::PF_FloatRGBA; 

	if (RT_Density == nullptr || RT_Velocity == nullptr || RT_Pressure == nullptr || RT_Divergence == nullptr )
	{
		CreateRHITextureResource(RT_Divergence, TEXT("FluidSim_RT_Divergence"), TextureFormat);
		CreateRHITextureResource(RT_Pressure, TEXT("FluidSim_RT_Pressure"), TextureFormat);
		CreateRHITextureResource(RT_Density, TEXT("FluidSim_RT_Density"), TextureFormat);
		CreateRHITextureResource(RT_Velocity, TEXT("FluidSim_RT_Velocity"), TextureFormat);
	}

	// Clear RTs
	ClearRenderTarget(RHICmdList, RT_Pressure);
	ClearRenderTarget(RHICmdList, RT_Divergence);
	ClearRenderTarget(RHICmdList, RT_Density);
	ClearRenderTarget(RHICmdList, RT_Velocity);

	ReadyToRender = 
		RT_Velocity &&
		RT_Divergence &&
		RT_Pressure &&
		RT_Density &&
		RT_Density_Vol && // Content Browser textures exist.
		RT_Velocity_Vol &&
		RT_Pressure_Vol &&
		RT_Divergence_Vol &&
		GridDescription.GridResolution.X > 0 && GridDescription.GridResolution.Y > 0 && GridDescription.GridResolution.Z > 0;

}

void UFluidSimulation::SimulationStep(const FFluidSolverSettings& InSettings)
{
	if (!ReadyToRender)
	{
		return;
	}

	// Make copy so we don't edit the data going to the GPU.
	const FObjectGPUDispatchParams GPUParams = FObjectGPUDispatchParams(GroupCount, InSettings, InjectionEventsPerFrame);
	
	if (IsInRenderingThread()) {
		DispatchRenderThread(GetImmediateCommandList_ForRenderCommand(), GPUParams);
	}
	else
	{
		ENQUEUE_RENDER_COMMAND(GPUFluidSimCommand)(
			[this, Params=GPUParams](FRHICommandListImmediate& RHICmdList)
			{
				UFluidSimulation::DispatchRenderThread(RHICmdList, Params);
			});
	}

	// Clear events after sending to GPU.
	ResetInjectionEvents();
}

void UFluidSimulation::DispatchRenderThread(FRHICommandListImmediate& RHICmdList, const FObjectGPUDispatchParams& Params)
{
	{
		if (!ReadyToRender) return;
	}
	
	FRDGBuilder GraphBuilder(RHICmdList, FRDGEventName(TEXT("UFluidSimulation::SimulationStep")));
	TSharedPtr<FComputeStageIntrinsics> StageIntrinsics = MakeShared<FComputeStageIntrinsics>(RHICmdList, GraphBuilder, Params.GroupCount, Params.Settings);
	
	// Register external textures with the graph builder.
	StageIntrinsics->SH_RT_Density = RegisterExternalTexture(GraphBuilder, RT_Density, TEXT("FluidSim_RT_Density"));
	StageIntrinsics->SH_RT_Velocity = RegisterExternalTexture(GraphBuilder, RT_Velocity, TEXT("FluidSim_RT_Velocity"));
	StageIntrinsics->SH_RT_Pressure = RegisterExternalTexture(GraphBuilder, RT_Pressure, TEXT("FluidSim_RT_Pressure"));
	StageIntrinsics->SH_RT_Divergence = RegisterExternalTexture(GraphBuilder, RT_Divergence, TEXT("FluidSim_RT_Divergence"));
	
	// Add simulation steps.
	Dissipate(StageIntrinsics, StageIntrinsics->SH_RT_Density, StageIntrinsics->Settings.DissipationDensity );
	Dissipate(StageIntrinsics, StageIntrinsics->SH_RT_Velocity, StageIntrinsics->Settings.DissipationVelocity );
	
	InjectSources(StageIntrinsics, Params);
	
	Diffusion(StageIntrinsics, StageIntrinsics->SH_RT_Density);

	// Projection
	ClearRenderTarget(RHICmdList, RT_Pressure);
	ClearRenderTarget(RHICmdList, RT_Divergence);
	Divergence(StageIntrinsics);
	
	int Itr = 0;
	while (Itr < Params.Settings.PressureIterations)
	{
		ProjectPressure(StageIntrinsics);
		Itr++;
	}
	ProjectGradient(StageIntrinsics);

	// Final Advection
	Advect(StageIntrinsics);

	// Copy the field to the RT which is then used with other actors/materials.
	FRDGTextureRef GameRTVelocity = RegisterExternalTexture(GraphBuilder, RT_Velocity_Vol->GetRenderTargetResource()->GetRenderTargetTexture(), TEXT("ObjectGPUFluidSimulation_OutRTVel"));
	AddCopyTexturePass(GraphBuilder, StageIntrinsics->SH_RT_Velocity, GameRTVelocity, FRHICopyTextureInfo() ); 

	FRDGTextureRef GameRTDensity = RegisterExternalTexture(GraphBuilder, RT_Density_Vol->GetRenderTargetResource()->GetRenderTargetTexture(), TEXT("ObjectGPUFluidSimulation_OutRTDensity"));
	AddCopyTexturePass(GraphBuilder, StageIntrinsics->SH_RT_Density, GameRTDensity, FRHICopyTextureInfo() ); 

	FRDGTextureRef GameRTPressure = RegisterExternalTexture(GraphBuilder, RT_Pressure_Vol->GetRenderTargetResource()->GetRenderTargetTexture(), TEXT("ObjectGPUFluidSimulation_OutRTPressure"));
	AddCopyTexturePass(GraphBuilder, StageIntrinsics->SH_RT_Pressure, GameRTPressure, FRHICopyTextureInfo() );

	FRDGTextureRef GameRTDivergence = RegisterExternalTexture(GraphBuilder, RT_Divergence_Vol->GetRenderTargetResource()->GetRenderTargetTexture(), TEXT("ObjectGPUFluidSimulation_OutRTDivergence"));
	AddCopyTexturePass(GraphBuilder, StageIntrinsics->SH_RT_Divergence, GameRTDivergence, FRHICopyTextureInfo() );
	
	StageIntrinsics.Reset();
	GraphBuilder.Execute();
}

void UFluidSimulation::SourceSim(FFluidSimSourceData SourceData)
{
	switch(SourceData.SourceType )
	{
		case EFluidSourceType::FAN:
		{
			// Can experiment with adding pressure again later.
			//SourceData.ShaderData.InjectionType = static_cast<uint32>(EFluidInjectionType::FANPRESSURE);
			//AddSimInjection(SourceData.ShaderData);
				
			SourceData.ShaderData.InjectionType = static_cast<uint32>(EFluidInjectionType::VELOCITY);
			AddSimInjection(SourceData.ShaderData);

			SourceData.ShaderData.InjectionType = static_cast<uint32>(EFluidInjectionType::DENSITY);
			AddSimInjection(SourceData.ShaderData);

			break;
		}
		case EFluidSourceType::EXPLOSION:
		{
			const float SourceStrength = SourceData.ShaderData.Strength;
			SourceData.ShaderData.InjectionType = static_cast<uint32>(EFluidInjectionType::VELOCITY);
			SourceData.ShaderData.Strength = SourceStrength * ExplosionVelocityScale;
			AddSimInjection(SourceData.ShaderData);

			SourceData.ShaderData.InjectionType = static_cast<uint32>(EFluidInjectionType::PRESSURE);
			SourceData.ShaderData.Strength = SourceStrength * ExplosionPressureScale;
			AddSimInjection(SourceData.ShaderData);

			SourceData.ShaderData.InjectionType = static_cast<uint32>(EFluidInjectionType::DENSITY);
			SourceData.ShaderData.Strength = SourceStrength * ExplosionDensityScale;
			AddSimInjection(SourceData.ShaderData);

			break;
		}
		case EFluidSourceType::WAKE:
		{
			SourceData.ShaderData.InjectionType = static_cast<uint32>(EFluidInjectionType::VELOCITY);
			AddSimInjection(SourceData.ShaderData);

			SourceData.ShaderData.InjectionType = static_cast<uint32>(EFluidInjectionType::DENSITY);
			AddSimInjection(SourceData.ShaderData);

			break;
		}
		case EFluidSourceType::DENSITY:
		{
			SourceData.ShaderData.InjectionType = static_cast<uint32>(EFluidInjectionType::DENSITY);
			AddSimInjection(SourceData.ShaderData);

			break;
		}
	}
}

void UFluidSimulation::AddSimInjection(FFluidSimSourceShaderData InjectEvent)
{
	InjectionEventsPerFrame.Emplace(InjectEvent);
}

void UFluidSimulation::Stop()
{
	ReadyToRender = false;

	if (IsInRenderingThread()) {
		StopRenderThread(GetImmediateCommandList_ForRenderCommand() );
	}
	else
	{
		ENQUEUE_RENDER_COMMAND(GPUFluidSimCommandStop)(
			[this](FRHICommandListImmediate& RHICmdList)
			{
				UFluidSimulation::StopRenderThread(RHICmdList);
			});
	}
}

void UFluidSimulation::Dissipate(const TSharedPtr<FComputeStageIntrinsics>& Stage, const FRDGTextureRef& DissipationTexture, const float& Strength)
{
	if (Stage->Settings.Debug < EFluidStageDebug::Dissipate) { return; }
	
	// Instantiate shader.
	FObjectGPUDissipationShader::FPermutationDomain PermutationVector;
	TShaderMapRef<FObjectGPUDissipationShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel), PermutationVector);

	if (!ComputeShader.IsValid())
	{
		UE_LOG(LogFluidSim, Warning, TEXT("Dissipate failed for '%s'"), DissipationTexture->Name);
		return;
	}

	FObjectGPUDissipationShader::FParameters* PassParameters = Stage->GraphBuilder.AllocParameters<FObjectGPUDissipationShader::FParameters>();

	// Shader parameters.
	PassParameters->RT_DissipationField = Stage->GraphBuilder.CreateUAV(DissipationTexture);
	PassParameters->DissipationGain = Strength;

	// Construct compute pass.
	Stage->GraphBuilder.AddPass(
		RDG_EVENT_NAME("ExecuteGPUObjectFluidSimDissipation"),
		PassParameters,
		ERDGPassFlags::AsyncCompute,
		[Params=PassParameters, CS=ComputeShader, Group=Stage->GPUGroupCount](FRHIComputeCommandList& CmdList)
		{
			FComputeShaderUtils::Dispatch(CmdList, CS, *Params, Group);
		}
	);
	
}

void UFluidSimulation::Diffusion(const TSharedPtr<FComputeStageIntrinsics>& Stage, const FRDGTextureRef& DiffusionTexture)
{
	if (Stage->Settings.Debug < EFluidStageDebug::Diffuse) { return; }
	
	FObjectGPUDiffusionShader::FPermutationDomain PermutationVector;
	TShaderMapRef<FObjectGPUDiffusionShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel), PermutationVector);

	if (!ComputeShader.IsValid())
	{
		UE_LOG(LogFluidSim, Warning, TEXT("Diffusion failed for '%s'"), DiffusionTexture->Name);
		return;
	}

	// Shader parameters.
	FObjectGPUDiffusionShader::FParameters* PassParameters = Stage->GraphBuilder.AllocParameters<FObjectGPUDiffusionShader::FParameters>();
	PassParameters->RT_DiffusionField = Stage->GraphBuilder.CreateUAV(DiffusionTexture);
	PassParameters->DiffusionGain = 1.0f - Stage->Settings.DiffusionStrength;

	// Construct compute pass.
	Stage->GraphBuilder.AddPass(
		RDG_EVENT_NAME("ExecuteGPUObjectFluidSimDiffusion"),
		PassParameters,
		ERDGPassFlags::AsyncCompute,
		[Params=PassParameters, CS=ComputeShader, Group=Stage->GPUGroupCount](FRHIComputeCommandList& CmdList)
		{
			FComputeShaderUtils::Dispatch(CmdList, CS, *Params, Group);
		}
	);
	
}

void UFluidSimulation::Divergence(const TSharedPtr<FComputeStageIntrinsics>& Stage)
{
	if (Stage->Settings.Debug < EFluidStageDebug::Divergence) { return; }
	
	FObjectGPUDivergenceShader::FPermutationDomain PermutationVector;
	TShaderMapRef<FObjectGPUDivergenceShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel), PermutationVector);

	if (!ComputeShader.IsValid())
	{
		UE_LOG(LogFluidSim, Warning, TEXT("Divergence failed."));
		return;
	}

	// Shader parameters.
	FObjectGPUDivergenceShader::FParameters* PassParameters = Stage->GraphBuilder.AllocParameters<FObjectGPUDivergenceShader::FParameters>();
	PassParameters->RT_Divergence = Stage->GraphBuilder.CreateUAV(Stage->SH_RT_Divergence);
	PassParameters->RT_Divergence_Vel = Stage->GraphBuilder.CreateSRV(Stage->SH_RT_Velocity);
	
	// Construct compute pass.
	Stage->GraphBuilder.AddPass(
		RDG_EVENT_NAME("ExecuteGPUObjectFluidSimDivergence"),
		PassParameters,
		ERDGPassFlags::AsyncCompute,
		[Params=PassParameters, CS=ComputeShader, Group=Stage->GPUGroupCount](FRHIComputeCommandList& CmdList)
		{
			FComputeShaderUtils::Dispatch(CmdList, CS, *Params, Group);
		}
	);
}

void UFluidSimulation::ProjectPressure(const TSharedPtr<FComputeStageIntrinsics>& Stage)
{
	if (Stage->Settings.Debug < EFluidStageDebug::Pressure) { return; }
	
	FObjectGPUProjectPressureShader::FPermutationDomain PermutationVector;
	TShaderMapRef<FObjectGPUProjectPressureShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel), PermutationVector);

	if (!ComputeShader.IsValid())
	{
		UE_LOG(LogFluidSim, Warning, TEXT("Project pressure failed."));
		return;
	}

	// Shader parameters.
	FObjectGPUProjectPressureShader::FParameters* PassParameters = Stage->GraphBuilder.AllocParameters<FObjectGPUProjectPressureShader::FParameters>();
	PassParameters->RT_ProjPressure_Divergence = Stage->GraphBuilder.CreateSRV(Stage->SH_RT_Divergence);
	PassParameters->RT_ProjPressure_Pressure = Stage->GraphBuilder.CreateUAV(Stage->SH_RT_Pressure);
	
	// Construct compute pass.
	Stage->GraphBuilder.AddPass(
		RDG_EVENT_NAME("ExecuteGPUObjectFluidSimProjectPressure"),
		PassParameters,
		ERDGPassFlags::AsyncCompute,
		[Params=PassParameters, CS=ComputeShader, Group=Stage->GPUGroupCount](FRHIComputeCommandList& CmdList)
		{
			FComputeShaderUtils::Dispatch(CmdList, CS, *Params, Group);
		}
	);
}

void UFluidSimulation::ProjectGradient(const TSharedPtr<FComputeStageIntrinsics>& Stage)
{
	if (Stage->Settings.Debug < EFluidStageDebug::Project) { return; }
	
	FObjectGPUProjectGradientShader::FPermutationDomain PermutationVector;
    TShaderMapRef<FObjectGPUProjectGradientShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel), PermutationVector);

    if (!ComputeShader.IsValid())
    {
    	UE_LOG(LogFluidSim, Warning, TEXT("Project gradient failed."));
    	return;
    }

    // Shader parameters.
    FObjectGPUProjectGradientShader::FParameters* PassParameters = Stage->GraphBuilder.AllocParameters<FObjectGPUProjectGradientShader::FParameters>();
    PassParameters->RT_ProjGradient_Pressure = Stage->GraphBuilder.CreateSRV(Stage->SH_RT_Pressure);
    PassParameters->RT_ProjGradient_Velocity = Stage->GraphBuilder.CreateUAV(Stage->SH_RT_Velocity);
    
    // Construct compute pass.
    Stage->GraphBuilder.AddPass(
    	RDG_EVENT_NAME("ExecuteGPUObjectFluidSimProjectGradient"),
    	PassParameters,
    	ERDGPassFlags::AsyncCompute,
    	[Params=PassParameters, CS=ComputeShader, Group=Stage->GPUGroupCount](FRHIComputeCommandList& CmdList)
    	{
    		FComputeShaderUtils::Dispatch(CmdList, CS, *Params, Group);
    	}
    	);
}

void UFluidSimulation::Advect(const TSharedPtr<FComputeStageIntrinsics>& Stage)
{
	if (Stage->Settings.Debug < EFluidStageDebug::Advect) { return; }
	
	// Instantiate shader.
	FObjectGPUAdvectionShader::FPermutationDomain PermutationVector;
	TShaderMapRef<FObjectGPUAdvectionShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel), PermutationVector);

	if (!ComputeShader.IsValid())
	{
		UE_LOG(LogFluidSim, Warning, TEXT("Advection failed."));
		return;
	}
	
	// Shader parameters.
	FObjectGPUAdvectionShader::FParameters* PassParameters = Stage->GraphBuilder.AllocParameters<FObjectGPUAdvectionShader::FParameters>();
	PassParameters->RT_Field_Read = Stage->GraphBuilder.CreateSRV(Stage->SH_RT_Density );
	PassParameters->RT_Field_Write = Stage->GraphBuilder.CreateUAV(Stage->SH_RT_Density);
	PassParameters->RT_Velocity = Stage->GraphBuilder.CreateSRV(Stage->SH_RT_Velocity );
	PassParameters->RT_Vel_Write = Stage->GraphBuilder.CreateUAV(Stage->SH_RT_Velocity);
	PassParameters->FieldSize = Stage->SH_RT_Velocity->Desc.GetSize();
	
	// Create the sampler
	FSamplerStateInitializerRHI SamplerStateInitializer
	(
		SF_Trilinear, // Filter mode: Trilinear
		AM_Clamp, // Addressing mode: Clamp
		AM_Clamp,
		AM_Clamp
	);
	const FSamplerStateRHIRef TriLinearSampler =  RHICreateSamplerState(SamplerStateInitializer);
	PassParameters->SamplerTrilinear = TriLinearSampler;
	
	// Construct compute pass.
	Stage->GraphBuilder.AddPass(
		RDG_EVENT_NAME("ExecuteGPUObjectFluidSimAdvection"),
		PassParameters,
		ERDGPassFlags::AsyncCompute,
		[Params=PassParameters, CS=ComputeShader, Group=Stage->GPUGroupCount](FRHIComputeCommandList& CmdList)
		{
			FComputeShaderUtils::Dispatch(CmdList, CS, *Params, Group);
		}
	);
}

void UFluidSimulation::InjectSources(const TSharedPtr<FComputeStageIntrinsics>& Stage, const FObjectGPUDispatchParams& Params)
{
	if (Stage->Settings.Debug < EFluidStageDebug::Inject) { return; }
	
	// Instantiate shader.
	FObjectGPUInjectionShader::FPermutationDomain PermutationVector;
	TShaderMapRef<FObjectGPUInjectionShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel), PermutationVector);

	if (!ComputeShader.IsValid()) return; // Add some warning/error text here later on.

	// Shader parameters.
	FObjectGPUInjectionShader::FParameters* PassParameters = Stage->GraphBuilder.AllocParameters<FObjectGPUInjectionShader::FParameters>();

	// Assign common textures. 
	PassParameters->RT_Velocity = Stage->GraphBuilder.CreateUAV(Stage->SH_RT_Velocity);
	PassParameters->RT_Pressure = Stage->GraphBuilder.CreateUAV(Stage->SH_RT_Pressure);
	PassParameters->RT_Density = Stage->GraphBuilder.CreateUAV(Stage->SH_RT_Density);
	PassParameters->FieldResolution = Stage->SH_RT_Velocity->Desc.GetSize();

	// Create Input events buffer.
	FRDGBufferRef InputBuffer = CreateStructuredBuffer(
		Stage->GraphBuilder,
		TEXT("FluidSimSourcingBuffer"),
		sizeof(FFluidSimSourceData),
		Params.InjectionEvents.Num(),
		Params.InjectionEvents.GetData(),
		Params.InjectionEvents.Num() * sizeof(FFluidSimSourceData));

	PassParameters->InjectionEventBuffer = Stage->GraphBuilder.CreateSRV(InputBuffer);
	PassParameters->BufferLength = Params.InjectionEvents.Num();

	// Construct render pass.
	Stage->GraphBuilder.AddPass(
		RDG_EVENT_NAME("ExecuteGPUObjectFluidSimInjection"),
		PassParameters,
		ERDGPassFlags::AsyncCompute,
		[Params=PassParameters, CS=ComputeShader, Group=Stage->GPUGroupCount](FRHIComputeCommandList& CmdList)
		{
			FComputeShaderUtils::Dispatch(CmdList, CS, *Params, Group);
		}
	);
}

void UFluidSimulation::StopRenderThread(FRHICommandListImmediate& RHICmdList)
{
	ReleaseRHITextureResource(RT_Divergence);
	ReleaseRHITextureResource(RT_Pressure);
	ReleaseRHITextureResource(RT_Density);
	ReleaseRHITextureResource(RT_Velocity);

	ReadyToRender = false; 
}

void UFluidSimulation::CreateRHITextureResource(FTextureRHIRef& TexReference, const TCHAR* TexName, const EPixelFormat& TexType, const FLinearColor& ClearColour)
{
	const FRHITextureCreateDesc CDesc = FRHITextureCreateDesc::Create3D(
		TexName, 
		GridDescription.GridResolution.X,
		GridDescription.GridResolution.Y,
		GridDescription.GridResolution.Z,
		TexType)
		.SetFlags(ETextureCreateFlags::External | ETextureCreateFlags::UAV | ETextureCreateFlags::RenderTargetable | ETextureCreateFlags::ShaderResource )
		.SetInitialState(ERHIAccess::UAVCompute)
		.SetExtent(GridDescription.GridResolution.X, GridDescription.GridResolution.Y)
		.SetDepth(GridDescription.GridResolution.Z)
		.SetClearValue(FClearValueBinding(ClearColour ) );
	
	TexReference = RHICreateTexture(CDesc);
}

void UFluidSimulation::ReleaseRHITextureResource(FTextureRHIRef Texture)
{
	if (Texture != nullptr) { Texture = nullptr;	}
}

void UFluidSimulation::ResetInjectionEvents()
{
	InjectionEventsPerFrame.Empty(false);

	FFluidSimSourceShaderData EmptyEvent = FFluidSimSourceShaderData();
	EmptyEvent.InjectionType = static_cast<int32>(EFluidInjectionType::NONE);
	InjectionEventsPerFrame.Add(EmptyEvent);
}

void UFluidSimulation::BeginDestroy()
{
	Stop(); // Make sure stop is being called
	Super::BeginDestroy();
}

void UFluidSimulation::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Stop(); // Make sure stop is being called
	Super::EndPlay(EndPlayReason);
}
