
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Math/Float16Color.h"

#include "FluidSimSubsystem.generated.h"


UCLASS()
class UFluidSimSubsystem : public UTickableWorldSubsystem
{
private:
	GENERATED_BODY()

public:
	// Begin USubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	// End USubsystem

	virtual TStatId GetStatId() const override;
	virtual void Tick(float DeltaTime) override;

	void RegisterSimManager(class AFluidSimulationManager* Manager);
	class AFluidSimulationManager* GetSimManager() const { return SimManager; };
	
	bool RegisterSource(class AFluidSimulationSource* SimSource);

	FVector GetFieldUVs(const AActor& InActor) const;

	FVector GetVelocity(const FVector& InLocation) const;
	
private:

	void FetchVelocityData(class UTextureRenderTargetVolume* InVel, TArray<FFloat16Color>& Data);
	
	UPROPERTY()
	class AFluidSimulationManager* SimManager = nullptr; 

	UPROPERTY()
	class UTextureRenderTargetVolume* VelocityField = nullptr;
	
	UPROPERTY()
	TArray<TObjectPtr<class AFluidSimulationSource>> SourceArray;
	
	TArray<FFloat16Color> VelocityData;
};
