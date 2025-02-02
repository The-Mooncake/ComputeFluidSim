
#pragma once

#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"


class FObjectGPUAdvectionShader : public FGlobalShader
{
public:

	DECLARE_GLOBAL_SHADER(FObjectGPUAdvectionShader);
	SHADER_USE_PARAMETER_STRUCT(FObjectGPUAdvectionShader, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture3D<FVector4f>, RT_Field_Read)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture3D<FVector4f>, RT_Velocity)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture3D<FVector4f>, RT_Field_Write)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture3D<FVector4f>, RT_Vel_Write)
		SHADER_PARAMETER_SAMPLER(SamplerState, SamplerTrilinear)
		SHADER_PARAMETER(FIntVector, FieldSize)
	END_SHADER_PARAMETER_STRUCT()

public:
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);

};

class FObjectGPUInjectionShader : public FGlobalShader
{
public:

	DECLARE_GLOBAL_SHADER(FObjectGPUInjectionShader);
	SHADER_USE_PARAMETER_STRUCT(FObjectGPUInjectionShader, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture3D<FVector4f>, RT_Velocity)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture3D<FVector4f>, RT_Pressure)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture3D<FVector4f>, RT_Density)
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<float>, InjectionEventBuffer)
		SHADER_PARAMETER(int, BufferLength)
		SHADER_PARAMETER(FIntVector, FieldResolution)	
	END_SHADER_PARAMETER_STRUCT()

public:
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);

};

class FObjectGPUDissipationShader : public FGlobalShader
{
public:

	DECLARE_GLOBAL_SHADER(FObjectGPUDissipationShader);
	SHADER_USE_PARAMETER_STRUCT(FObjectGPUDissipationShader, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture3D<FVector4f>, RT_DissipationField)
		SHADER_PARAMETER(float, DissipationGain)
	END_SHADER_PARAMETER_STRUCT()

public:
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);

};

class FObjectGPUDiffusionShader : public FGlobalShader
{
public:

	DECLARE_GLOBAL_SHADER(FObjectGPUDiffusionShader);
	SHADER_USE_PARAMETER_STRUCT(FObjectGPUDiffusionShader, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture3D<FVector4f>, RT_DiffusionField)
		SHADER_PARAMETER(float, DiffusionGain)
	END_SHADER_PARAMETER_STRUCT()

public:
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);

};

class FObjectGPUDivergenceShader : public FGlobalShader
{
public:

	DECLARE_GLOBAL_SHADER(FObjectGPUDivergenceShader);
	SHADER_USE_PARAMETER_STRUCT(FObjectGPUDivergenceShader, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture3D<FVector4f>, RT_Divergence_Vel)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture3D<FVector4f>, RT_Divergence)
	END_SHADER_PARAMETER_STRUCT()

public:
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);

};

class FObjectGPUProjectPressureShader : public FGlobalShader
{
public:

	DECLARE_GLOBAL_SHADER(FObjectGPUProjectPressureShader);
	SHADER_USE_PARAMETER_STRUCT(FObjectGPUProjectPressureShader, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture3D<FVector4f>, RT_ProjPressure_Divergence)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture3D<FVector4f>, RT_ProjPressure_Pressure)
	END_SHADER_PARAMETER_STRUCT()

public:
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);

};

class FObjectGPUProjectGradientShader : public FGlobalShader
{
public:
	
	DECLARE_GLOBAL_SHADER(FObjectGPUProjectGradientShader);
	SHADER_USE_PARAMETER_STRUCT(FObjectGPUProjectGradientShader, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture3D<FVector4f>, RT_ProjGradient_Pressure)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture3D<FVector4f>, RT_ProjGradient_Velocity)
	END_SHADER_PARAMETER_STRUCT()

public:
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);

};

