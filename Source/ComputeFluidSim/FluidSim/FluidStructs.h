

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RenderGraphDefinitions.h"
#include "RHICommandList.h"

#include "FluidStructs.generated.h"

UENUM()
enum class EFluidSimDebug : uint8
{
	Velocity = 0,
	Density,
	Divergence,
	Pressure
};

UENUM()
enum class EFluidStageDebug : uint8
{
	Dissipate = 0,
	Inject,
	Diffuse,
	Divergence,
	Pressure,
	Project,
	Advect,
	None
};

USTRUCT(BlueprintType)
struct FFluidSolverSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, meta=(ClampMin="0.0", ClampMax="0.9999"))
	float DiffusionStrength = 0.1f;

	UPROPERTY(EditAnywhere, meta=(ClampMin="0.0", ClampMax="1.0"))
	float DissipationDensity = 0.05f;

	UPROPERTY(EditAnywhere, meta=(ClampMin="0.0", ClampMax="1.0"))
	float DissipationVelocity = 0.1f;

	UPROPERTY(EditAnywhere, meta=(ClampMin="1"))
	int PressureIterations = 8;

	UPROPERTY()
	EFluidStageDebug Debug = EFluidStageDebug::None;
};

USTRUCT(BlueprintType)
struct FGridDescription
{
	GENERATED_BODY();

	UPROPERTY()
	float VoxelSizeWS;

	UPROPERTY()
	FIntVector GridResolution;

	UPROPERTY()
	FVector3f GridSizeWS;

	FGridDescription(const float VoxSize = 1.0,const FIntVector GridRes = FIntVector(64, 64, 32)) : VoxelSizeWS(VoxSize), GridResolution(GridRes)
	{
		GridSizeWS = FVector3f(GridResolution.X * VoxelSizeWS, GridResolution.Y * VoxelSizeWS, GridResolution.Z * VoxelSizeWS);
	}
};

struct FContentBrowserTextures
{
	TObjectPtr<class UTextureRenderTargetVolume> RT_Velocity_Vol = nullptr;
	TObjectPtr<class UTextureRenderTargetVolume> RT_Density_Vol = nullptr;
	TObjectPtr<class UTextureRenderTargetVolume> RT_Pressure_Vol = nullptr;
	TObjectPtr<class UTextureRenderTargetVolume> RT_Divergence_Vol = nullptr;

	FContentBrowserTextures(TObjectPtr<class UTextureRenderTargetVolume> Vel, TObjectPtr<class UTextureRenderTargetVolume> Density, TObjectPtr<class UTextureRenderTargetVolume> Pressure, TObjectPtr<class UTextureRenderTargetVolume> Divergence)
		: RT_Velocity_Vol(Vel), RT_Density_Vol(Density) , RT_Pressure_Vol(Pressure), RT_Divergence_Vol(Divergence)
	{}
};

struct FComputeStageIntrinsics
{
	class FRHICommandListImmediate& RHICmdList;
	class FRDGBuilder& GraphBuilder;
	FIntVector GPUGroupCount;
	FFluidSolverSettings Settings;
	
	// Shader textures, must be defined in the initial GraphBuilder GPU pass.
	FRDGTextureRef SH_RT_Density = nullptr;
	FRDGTextureRef SH_RT_Velocity = nullptr;
	FRDGTextureRef SH_RT_Pressure = nullptr;
	FRDGTextureRef SH_RT_Divergence = nullptr;

	FComputeStageIntrinsics(class FRHICommandListImmediate& InRHICmd, class FRDGBuilder& InGraph, const FIntVector InGPUGroup, const FFluidSolverSettings InSettings)
		: RHICmdList(InRHICmd), GraphBuilder(InGraph), GPUGroupCount(InGPUGroup), Settings(InSettings)
	{}
};

UENUM(BlueprintType)
enum class EFluidSourceType : uint8 {
	FAN = 0			UMETA(DisplayName = "Fan"),
	EXPLOSION = 1	UMETA(DisplayName = "Explosion"),
	WAKE = 2		UMETA(DisplayName = "Wake"),
	DENSITY = 3		UMETA(DisplayName = "Debug Density"),
};

namespace FluidSimStructs
{
	constexpr EFluidSourceType DefaultFluidSimSourceType = EFluidSourceType::FAN;
}

UENUM(BlueprintType, meta=(Bitflags) )
enum class EFluidInjectionType : uint8 {
	NONE =			0		UMETA(Hidden),
	VELOCITY =		1		UMETA(DisplayName = "Velocity"),
	PRESSURE =		2		UMETA(DisplayName = "Pressure"),
	FANPRESSURE =	4		UMETA(DisplayName = "FanDensity"),
	DENSITY =		8		UMETA(DisplayName = "Density"),
};
ENUM_CLASS_FLAGS(EFluidInjectionType);

USTRUCT()
struct FFluidSimSourceShaderData
{
	GENERATED_BODY();

	uint32 InjectionType = 0;
	FIntVector PositionIdx = FIntVector(0, 0, 0);
	FVector3f DirectionVectorWS = FVector3f::ZeroVector;
	float Strength = 1.0f;
	float Size = 1.0f;
	float Hardness = 0.5f;
};


USTRUCT()
struct FFluidSimSourceData
{
	GENERATED_BODY();
	
	EFluidSourceType SourceType = FluidSimStructs::DefaultFluidSimSourceType;
	FVector3f PositionWS = FVector3f::ZeroVector;

	FFluidSimSourceShaderData ShaderData = FFluidSimSourceShaderData();
};


struct FObjectGPUDispatchParams
{
	FIntVector GroupCount = FIntVector(32, 32, 32);
	FFluidSolverSettings Settings;
	TArray<FFluidSimSourceShaderData> InjectionEvents;

	FObjectGPUDispatchParams(const FIntVector InGroupCount, const FFluidSolverSettings InSettings, const TArray<FFluidSimSourceShaderData>& FrameInjectionEvents )
		: GroupCount(InGroupCount), Settings(InSettings), InjectionEvents(FrameInjectionEvents)
	{}
};

