
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FluidStructs.h"

#include "FluidSimulationSource.generated.h"
 

UCLASS(ClassGroup = (FluidSim), meta = (BlueprintSpawnableComponent), hidecategories = ("Rendering", "Components", "WorldPartition", "Tags", "Replication", "Collision", "Input", "HLOD", "Actor", "LOD", "Tags", "Cooking", "DataLayers", "Physics", "Networking"))
class AFluidSimulationSource : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AFluidSimulationSource();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
	void RegisterWithSim();
	void Trigger();
	FFluidSimSourceData CreateShaderSourceData();
	FIntVector GetSimPositionIdx() const;
	void CalculateSourceDirectionVector();
	FVector3f GetSourceDirectionVector();

private:
	UPROPERTY()
	class UFluidSimSubsystem* FluidSimSubSystem = nullptr;

	UPROPERTY()
	class AFluidSimulationManager* SimManager = nullptr;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EFluidSourceType SourceType = FluidSimStructs::DefaultFluidSimSourceType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool IsActive = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Strength = 1.0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Size = 1.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Hardness = 0.5;

private:
	bool SourceReady = false; 

	FVector3f SourceVelocity = FVector3f::Zero();
	FVector LastFramePosition = FVector::Zero();

	int ExplosionFrameCount = 0;
	int MaxExplosionFrames = 3;
};
