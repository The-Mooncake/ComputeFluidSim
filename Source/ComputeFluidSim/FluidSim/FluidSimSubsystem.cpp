
#include "FluidSimSubsystem.h"
#include "FluidSimulationManager.h"
#include "FluidSimulationSource.h" 
#include "Engine/TextureRenderTargetVolume.h"

void UFluidSimSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UFluidSimSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

TStatId UFluidSimSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UFluidSimSubsystem, STATGROUP_Tickables);
}

void UFluidSimSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// See notes in FetchVelocityData()
	// ENQUEUE_RENDER_COMMAND(GPUFluidSimSubsystem)(
	// 		[this, VelField=VelocityField, Data=&VelocityData](FRHICommandListImmediate& RHICmdList)
	// 		{
	// 			FetchVelocityData(VelField, *Data);
	// 		});

	FetchVelocityData(VelocityField, VelocityData);
}

void UFluidSimSubsystem::RegisterSimManager(AFluidSimulationManager* Manager)
{
	if (IsValid(Manager))
	{
		SimManager = Manager;

		VelocityField = SimManager->RT_Velocity_Vol;
		checkf(VelocityField, TEXT("SimManager Velocity Field is nullptr!"));
		VelocityData.Reserve(VelocityField->SizeX * VelocityField->SizeY * VelocityField->SizeZ);
	}
}

bool UFluidSimSubsystem::RegisterSource(AFluidSimulationSource* SimSource)
{
	if (!IsValid(SimSource)) return false;

	SourceArray.Add(SimSource);

	return true;
}

FVector UFluidSimSubsystem::GetFieldUVs(const AActor& InActor) const
{
	if (SimManager == nullptr || SimManager == nullptr) return FVector::Zero();

	const FVector SimLocation = SimManager->GetActorLocation();
	const FVector SimResolution = FVector(SimManager->GridResolution.X, SimManager->GridResolution.Y, SimManager->GridResolution.Z);
	const FVector SimSize = SimResolution * SimManager->VoxelSize * 100.0f; // VoxelSize in M.
	const FVector ActorPosition = InActor.GetActorLocation();

	FVector UVs = ActorPosition - SimLocation;
	UVs += SimSize / 2.0f;
	UVs /= SimSize;
	
	return UVs;
}

FVector UFluidSimSubsystem::GetVelocity(const FVector& InLocation) const
{
	const FVector SimLocation = SimManager->GetActorLocation();
	const FVector SimResolution = FVector(SimManager->GridResolution.X, SimManager->GridResolution.Y, SimManager->GridResolution.Z);
	const FVector SimSize = SimResolution * SimManager->VoxelSize * 100.0f; // VoxelSize in M;

	FVector UVs = InLocation - SimLocation;
	UVs += SimSize / 2.0f;
	UVs /= SimManager->VoxelSize * 100.0f;
	const FIntVector Voxel = FIntVector(UVs.X, UVs.Y, UVs.Z);
	const int Idx = Voxel.Z * SimResolution.Z + Voxel.Y * SimResolution.Y + Voxel.X;

	if (Idx >= 0 && Idx < VelocityData.Num())
	{
		const FLinearColor Color = VelocityData[Idx].GetFloats() * 100.0f;
		return FVector(Color.R, Color.G, Color.B);
	}
	return FVector::ZeroVector;
}

void UFluidSimSubsystem::FetchVelocityData(UTextureRenderTargetVolume* InVel, TArray<FFloat16Color>& Data)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR("UFluidSimSubsystem::FetchVelocityData");
	
	// Note slow when run on game thread. Takes about 32ms on game thread.
	// Move to async render thread task... Can do GetRenderTargetResource(), in rendering thread.
	//VelocityField->GameThread_GetRenderTargetResource()->ReadFloat16Pixels(VelocityData);
	// N.B: Potential for dual access or data race if two objects access this.

	// This doesn't work as a resource isn't initialised properly. Try the game thread version on an async game thread.
	//FTextureRenderTargetResource* RTResource = InVel->GetRenderTargetResource();
	//if (RTResource->IsInitialized() && RTResource->IsTextureRHIPartiallyResident())
	//{
		//RTResource->ReadFloat16Pixels(Data);	
	//}
}
