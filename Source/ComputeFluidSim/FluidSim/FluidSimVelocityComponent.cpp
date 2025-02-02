// Fill out your copyright notice in the Description page of Project Settings.


#include "FluidSimVelocityComponent.h"

#include "FluidSimSubsystem.h"

UFluidSimVelocityComponent::UFluidSimVelocityComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetTickGroup(TG_PrePhysics);
}

// Called when the game starts
void UFluidSimVelocityComponent::BeginPlay()
{
	Super::BeginPlay();
	CacheSimulation();
}

// Called every frame
void UFluidSimVelocityComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ResolveVelocity();
}

void UFluidSimVelocityComponent::CacheSimulation()
{
	const UWorld* World = GetWorld();
	if (!World) return;

	FluidSimSubsystem = World->GetSubsystem<UFluidSimSubsystem>();
}

void UFluidSimVelocityComponent::ResolveVelocity()
{
	if (FluidSimSubsystem == nullptr) { CacheSimulation(); return; }
	
	const AActor* OwningActor = this->GetOwner();
	const FVector Location = OwningActor->GetActorLocation();
	const FVector Vel = FluidSimSubsystem.Get()->GetVelocity(Location) * VelocityScale;
	
	UPrimitiveComponent* RootComponent = Cast<UPrimitiveComponent>(OwningActor->GetRootComponent());
	if (RootComponent != nullptr)
	{
		if (FBodyInstance* BI = RootComponent->GetBodyInstance())
		{
			BI->SetLinearVelocity(Vel, true);	
		}
	}
}

