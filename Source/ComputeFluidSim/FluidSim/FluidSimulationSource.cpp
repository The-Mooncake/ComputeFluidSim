
#include "FluidSimulationSource.h"

#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Engine/LocalPlayer.h"
#include "FluidSimulationManager.h"
#include "FluidSimSubsystem.h"


// Sets default values
AFluidSimulationSource::AFluidSimulationSource()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AFluidSimulationSource::BeginPlay()
{
	Super::BeginPlay();

	RegisterWithSim(); // Try registering now, however other subsystems may not have ticked.
	
	LastFramePosition = GetActorLocation();
	SourceVelocity = FVector3f::Zero();
}

// Called every frame
void AFluidSimulationSource::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (!IsActive)
	{
		// Reset source.
		ExplosionFrameCount = 0;
		return;
	};

	// Trigger source.
	switch (SourceType) {
	case EFluidSourceType::EXPLOSION: 
		if (ExplosionFrameCount < MaxExplosionFrames)
		{
			Trigger();
			ExplosionFrameCount++;
		}
		break;

	default: // Triggered every frame.
		Trigger();
		break;
	}
}

void AFluidSimulationSource::RegisterWithSim()
{
	// Get player simulation subsystem.
	UWorld* World = GetWorld();
	if (World == nullptr) { return; }

	// Register this source actor with the fluid subsystem.
	FluidSimSubSystem = World->GetSubsystem<UFluidSimSubsystem>();
	SimManager = FluidSimSubSystem ? FluidSimSubSystem->GetSimManager() : nullptr;
	if (FluidSimSubSystem && SimManager)
	{
		FluidSimSubSystem->RegisterSource(this);
		SourceReady = true;
	}
}

void AFluidSimulationSource::Trigger()
{
	if (SourceReady)
	{
		FFluidSimSourceData SourceData = CreateShaderSourceData();
		SimManager->Source(SourceData);
	}
	else
	{
		RegisterWithSim();
		return; // Try again next frame.
	}
}

FFluidSimSourceData AFluidSimulationSource::CreateShaderSourceData()
{
	FFluidSimSourceData SourceData = FFluidSimSourceData();
	SourceData.SourceType = SourceType;
	SourceData.PositionWS = static_cast<FVector3f>(GetActorLocation() );
	SourceData.ShaderData.PositionIdx = GetSimPositionIdx();
	SourceData.ShaderData.DirectionVectorWS = GetSourceDirectionVector() ;
	SourceData.ShaderData.Strength = Strength;
	SourceData.ShaderData.Size = Size;
	SourceData.ShaderData.Hardness = Hardness;
	
	return SourceData;
}

FIntVector AFluidSimulationSource::GetSimPositionIdx() const 
{
	const FVector3f SimLocation = static_cast<FVector3f>(SimManager->GetActorLocation() );
	const FGridDescription Desc = SimManager->GetSimGridDescription();
	const FVector3f ActorLocation = static_cast<FVector3f>(GetActorLocation() );

	FVector3f SampleLocation = ActorLocation - SimLocation ;
	SampleLocation += (Desc.GridSizeWS * 100) / 2;
	SampleLocation /= (Desc.GridSizeWS * 100) ;
	SampleLocation *= static_cast<FVector3f>(Desc.GridResolution);

	return FIntVector(floor(SampleLocation.X), floor(SampleLocation.Y), floor(SampleLocation.Z) );
}

void AFluidSimulationSource::CalculateSourceDirectionVector()
{
	const FVector CurrentPosition = GetActorLocation();
	SourceVelocity = static_cast<FVector3f>(CurrentPosition - LastFramePosition);
	LastFramePosition = CurrentPosition;
}

FVector3f AFluidSimulationSource::GetSourceDirectionVector()
{
	switch (SourceType)
	{
	case EFluidSourceType::FAN:
		return static_cast<FVector3f>(GetActorRotation().Vector() );
		
	case EFluidSourceType::WAKE:
		CalculateSourceDirectionVector();
		return SourceVelocity / 100.0; // Parameterize magic variable, CVAR?
		
	default:
		return FVector3f::Zero();
	}
}
