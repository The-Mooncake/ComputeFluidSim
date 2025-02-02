// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FluidSimVelocityComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class COMPUTEFLUIDSIM_API UFluidSimVelocityComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UFluidSimVelocityComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere)
	float VelocityScale = 1.0f;
private:

	void CacheSimulation();
	void ResolveVelocity();

	UPROPERTY()
	TSoftObjectPtr<class AFluidSimulationManager> SimObject = nullptr;

	UPROPERTY()
	TSoftObjectPtr<class UFluidSimSubsystem> FluidSimSubsystem = nullptr;

	UPROPERTY()
	class UTextureRenderTargetVolume* VelocityField = nullptr;
	
	
};
