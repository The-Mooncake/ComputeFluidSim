
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FluidStructs.h"

#include "FluidSimulation.generated.h"


UCLASS()
class UFluidSimulation : public UActorComponent
{
	GENERATED_BODY()

public: // Simulation CPU
	bool Setup(const FGridDescription& Desc, const FContentBrowserTextures& CBTexts);
	void Stop();

	// Simulation Actions
	void SimulationStep(const FFluidSolverSettings& InSettings);
	void SourceSim(FFluidSimSourceData SourceData);
	FGridDescription GetGridDescription() const { return GridDescription; }

	// UObject Overrides
	virtual void BeginDestroy() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// End UObject Overrides

private: // Simulation
	void SetupRenderThread(FRHICommandListImmediate& RHICmdList);
	void StopRenderThread(FRHICommandListImmediate& RHICmdList);
	void DispatchRenderThread(FRHICommandListImmediate& RHICmdList, const FObjectGPUDispatchParams& Params);

	void AddSimInjection(FFluidSimSourceShaderData InjectEvent);
	
	// Simulation Stages
	void Dissipate(const TSharedPtr<FComputeStageIntrinsics>& Stage, const FRDGTextureRef& DissipationTexture, const float& Strength);
	void Diffusion(const TSharedPtr<FComputeStageIntrinsics>& Stage, const FRDGTextureRef& DiffusionTexture);
	void Divergence(const TSharedPtr<FComputeStageIntrinsics>& Stage);
	void ProjectPressure(const TSharedPtr<FComputeStageIntrinsics>& Stage);
	void ProjectGradient(const TSharedPtr<FComputeStageIntrinsics>& Stage);
	void Advect(const TSharedPtr<FComputeStageIntrinsics>& Stage);
	void InjectSources(const TSharedPtr<FComputeStageIntrinsics>& Stage, const FObjectGPUDispatchParams& Params);

private: // Helpers GPU
	void CreateRHITextureResource(FTextureRHIRef& TexReference,
	                              const TCHAR* TexName,
	                              const EPixelFormat& TexType,
	                              const FLinearColor& ClearColour = FLinearColor::Black);

	void ReleaseRHITextureResource(FTextureRHIRef Texture);

	void ResetInjectionEvents();

private: // GPU Thread Variables/Textures
	FTextureRHIRef RT_Density = nullptr;
	FTextureRHIRef RT_Velocity = nullptr;
	FTextureRHIRef RT_Divergence = nullptr;
	FTextureRHIRef RT_Pressure = nullptr;

public: // CPU Thread
	// Explosions require multipliers for a realistic shape.
	// They are a concentration of high pressure and energy.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ExplosionPressureScale = 100000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ExplosionVelocityScale = 100000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ExplosionDensityScale = 5000.0f;
	
private: // CPU Thread
	UPROPERTY()
	bool ReadyToRender = false;

	UPROPERTY()
	FGridDescription GridDescription;

	UPROPERTY()
	FIntVector GroupCount;
	
	// Textures to copy to content browser.
	UPROPERTY()
	class UTextureRenderTargetVolume* RT_Velocity_Vol = nullptr;
	
	UPROPERTY()
	class UTextureRenderTargetVolume* RT_Density_Vol = nullptr;

	UPROPERTY()
	class UTextureRenderTargetVolume* RT_Pressure_Vol = nullptr;

	UPROPERTY()
	class UTextureRenderTargetVolume* RT_Divergence_Vol = nullptr;

	UPROPERTY()
	TArray<FFluidSimSourceShaderData> InjectionEventsPerFrame;
};

