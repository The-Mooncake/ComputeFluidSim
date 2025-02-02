
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FluidStructs.h"

#include "FluidSimulationManager.generated.h"

UCLASS(Blueprintable)
class AFluidSimulationManager : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AFluidSimulationManager();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UTextureRenderTargetVolume* RT_Velocity_Vol = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UTextureRenderTargetVolume* RT_Density_Vol = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UTextureRenderTargetVolume* RT_Pressure_Vol = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UTextureRenderTargetVolume* RT_Divergence_Vol = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntVector GridResolution = FIntVector(64, 64, 32);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0.001)) // Clamp to not zero.
	float VoxelSize = 1.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FFluidSolverSettings SolverSettings;

	// Draw a debug bounds for the simulation domain.
	UPROPERTY(EditInstanceOnly, AdvancedDisplay)
	bool bDrawBounds = false;

	//Draw a voxel in the 0,0,0 voxel to debug the size.
	UPROPERTY(EditInstanceOnly, AdvancedDisplay)
	bool bDrawVoxel = false;

	// Draw a plane to debug the fields in the simulation domain.
	UPROPERTY(EditInstanceOnly, AdvancedDisplay)
	bool bVisualDebug = false;

	UPROPERTY(EditInstanceOnly, AdvancedDisplay, NonPIEDuplicateTransient)
	EFluidSimDebug FieldToDebug = EFluidSimDebug::Velocity;

	UPROPERTY(EditInstanceOnly, AdvancedDisplay, NonPIEDuplicateTransient)
	float DebugZPlaneHeight = 0.5f;

	UPROPERTY(EditInstanceOnly, AdvancedDisplay, NonPIEDuplicateTransient)
	float DebugValueMultiplier = 1.0f;

	UPROPERTY(EditInstanceOnly, AdvancedDisplay, NonPIEDuplicateTransient)
	EFluidStageDebug SimStageDebug = EFluidStageDebug::None;
	
	UPROPERTY(EditDefaultsOnly)
	UStaticMesh* DebugStaticMesh = nullptr;

	UPROPERTY(EditDefaultsOnly)
	UMaterial* DebugMaterial = nullptr;
	
	UPROPERTY(VisibleAnywhere, Transient)
	class UStaticMeshComponent* DebugMeshComponent = nullptr;
	
	UPROPERTY(Transient)
	UMaterialInstanceDynamic* DebugMaterialDynamic = nullptr;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	UPROPERTY(VisibleAnywhere, Transient)
	class UFluidSimulation* Solver = nullptr;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	virtual void PostLoad() override;

	virtual void PostActorCreated() override;

	void Source(FFluidSimSourceData SourceData) const;

	FGridDescription GetSimGridDescription() const;

private:

	UPROPERTY(Transient)
	bool SolverCPUReady = false;

	UPROPERTY(Transient)
	class UFluidSimSubsystem* FluidSimSubSystem;

	void SetupDebug();
	void UpdateDebug();
};
