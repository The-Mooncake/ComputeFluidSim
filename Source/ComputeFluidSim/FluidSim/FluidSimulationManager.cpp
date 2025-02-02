#include "FluidSimulationManager.h"


#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "UObject/ConstructorHelpers.h"
#include "FluidSimSubsystem.h"
#include "FluidSimulation.h"
#include "Materials/MaterialInstanceDynamic.h"

// Sets default values
AFluidSimulationManager::AFluidSimulationManager()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Ticking in pre physics so we can BeginPlay and Tick the simulation before other components, e.g: sources/components/etc...
	PrimaryActorTick.TickGroup = ETickingGroup::TG_PrePhysics;

	Solver = CreateDefaultSubobject<UFluidSimulation>(TEXT("Simulation Solver"));

	// Initialise debug objects.
	DebugMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Debug Plane"));
	DebugMeshComponent->SetVisibility(bVisualDebug);
}

// Called when the game starts or when spawned
void AFluidSimulationManager::BeginPlay()
{
	Super::BeginPlay();

	// Register simulation.
	const UWorld* World = GetWorld();
	if (World != nullptr)
	{
		FluidSimSubSystem = World->GetSubsystem<UFluidSimSubsystem>();
		FluidSimSubSystem->RegisterSimManager(this);
	}

	// Setup Solver
	if (IsValid(Solver))
	{
		FGridDescription Desc = FGridDescription(VoxelSize, GridResolution);
		FContentBrowserTextures Textures = FContentBrowserTextures(RT_Velocity_Vol, RT_Density_Vol, RT_Pressure_Vol, RT_Divergence_Vol);
		SolverCPUReady = Solver->Setup(Desc, Textures);
	}
	
}

// Called every frame
void AFluidSimulationManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const UWorld* World = GetWorld();
	if ((bDrawBounds || bDrawVoxel) && World != nullptr)
	{
		const FVector GridSize = FVector(GridResolution.X, GridResolution.Y, GridResolution.Z) * 50.0f;
		if (bDrawBounds)
		{
			DrawDebugBox(World, GetActorLocation(), GridSize, FColor::Red);
		}
		if (bDrawVoxel)
		{
			const FVector Voxel = FVector(VoxelSize, VoxelSize, VoxelSize) * 50.0f; 
			const FVector Corner = GetActorLocation() - GridSize + (Voxel);
			DrawDebugBox(World, Corner, Voxel, FColor::Cyan);
		}
	}
	
	if (IsValid(Solver) && SolverCPUReady)
	{
		Solver->SimulationStep(SolverSettings);
	}
}

void AFluidSimulationManager::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	const FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	const FName MemberPropertyName = PropertyChangedEvent.MemberProperty->GetFName();
	
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AFluidSimulationManager, bVisualDebug) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(AFluidSimulationManager, FieldToDebug) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(AFluidSimulationManager, DebugZPlaneHeight) ||
		MemberPropertyName == GET_MEMBER_NAME_CHECKED(AFluidSimulationManager, GridResolution) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(AFluidSimulationManager, SimStageDebug) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(AFluidSimulationManager, DebugValueMultiplier)
		)
	{
		UpdateDebug();
	}
}

void AFluidSimulationManager::PostLoad()
{
	Super::PostLoad();
	SetupDebug();
}

void AFluidSimulationManager::PostActorCreated()
{
	Super::PostActorCreated();
	SetupDebug();
}

void AFluidSimulationManager::Source(FFluidSimSourceData SourceData) const
{
	Solver->SourceSim(SourceData);
}

FGridDescription AFluidSimulationManager::GetSimGridDescription() const
{
	return Solver->GetGridDescription();
}

void AFluidSimulationManager::SetupDebug()
{
	if (DebugStaticMesh != nullptr && DebugMaterial != nullptr)
	{
		DebugMeshComponent->SetStaticMesh(DebugStaticMesh);
		DebugMaterialDynamic = UMaterialInstanceDynamic::Create(DebugMaterial, this);
		DebugMeshComponent->SetMaterial(0, DebugMaterialDynamic);
		UpdateDebug();
	}
}

void AFluidSimulationManager::UpdateDebug()
{
	DebugMeshComponent->SetVisibility(bVisualDebug);
	DebugMeshComponent->SetWorldScale3D(FVector(GridResolution));

	if (DebugMaterialDynamic)
	{
		switch (FieldToDebug)
		{
		case EFluidSimDebug::Velocity:
			DebugMaterialDynamic->SetScalarParameterValue(FName("ViewDensity"), 0.0f);
			DebugMaterialDynamic->SetScalarParameterValue(FName("ViewDivergence"), 0.0f);
			DebugMaterialDynamic->SetScalarParameterValue(FName("ViewPressure"), 0.0f);
			break;

		case EFluidSimDebug::Density:
			DebugMaterialDynamic->SetScalarParameterValue(FName("ViewDensity"), 1.0f);
			DebugMaterialDynamic->SetScalarParameterValue(FName("ViewDivergence"), 0.0f);
			DebugMaterialDynamic->SetScalarParameterValue(FName("ViewPressure"), 0.0f);
			break;
			
		case EFluidSimDebug::Divergence:
			DebugMaterialDynamic->SetScalarParameterValue(FName("ViewDensity"), 0.0f);
			DebugMaterialDynamic->SetScalarParameterValue(FName("ViewDivergence"), 1.0f);
			DebugMaterialDynamic->SetScalarParameterValue(FName("ViewPressure"), 0.0f);
			break;

		case EFluidSimDebug::Pressure:
			DebugMaterialDynamic->SetScalarParameterValue(FName("ViewDensity"), 0.0f);
			DebugMaterialDynamic->SetScalarParameterValue(FName("ViewDivergence"), 0.0f);
			DebugMaterialDynamic->SetScalarParameterValue(FName("ViewPressure"), 1.0f);
			break;
		}
		
		DebugMaterialDynamic->SetScalarParameterValue(FName("ZAxis"), DebugZPlaneHeight);
		DebugMaterialDynamic->SetScalarParameterValue(FName("DebugScale"), DebugValueMultiplier);
	}

	SolverSettings.Debug = SimStageDebug;
}

