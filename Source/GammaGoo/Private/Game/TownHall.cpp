// Copyright 2026 Bret Wright. All Rights Reserved.

#include "Game/TownHall.h"
#include "Fluid/FluidSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

ATownHall::ATownHall()
{
	PrimaryActorTick.bCanEverTick = false;

	BuildingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BuildingMesh"));
	RootComponent = BuildingMesh;
}

void ATownHall::BeginPlay()
{
	Super::BeginPlay();

	FluidSubsystem = GetWorld()->GetSubsystem<UFluidSubsystem>();
	BaseZ = GetActorLocation().Z;

	GetWorldTimerManager().SetTimer(
		DamageTimerHandle,
		FTimerDelegate::CreateUObject(this, &ATownHall::CheckFluidDamage),
		DamageCheckInterval,
		/*bLoop=*/true
	);
}

void ATownHall::CheckFluidDamage()
{
	if (bDestroyed || !FluidSubsystem) { return; }

	const float FluidHeight = FluidSubsystem->GetFluidHeightAtWorldPos(GetActorLocation());
	const float Depth = FluidHeight - BaseZ;

	if (Depth <= 0.f) { return; }

	const float Damage = Depth * DamagePerDepthPerSecond * DamageCheckInterval;
	Health = FMath::Max(0.f, Health - Damage);
	OnHealthChanged.Broadcast(Health, MaxHealth);

	if (Health <= 0.f)
	{
		bDestroyed = true;
		GetWorldTimerManager().ClearTimer(DamageTimerHandle);
		OnTownDestroyed.Broadcast();
	}
}
