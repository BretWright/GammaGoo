// Copyright 2026 Bret Wright. All Rights Reserved.

#include "Towers/FluidTowerBase.h"
#include "Fluid/FluidSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

AFluidTowerBase::AFluidTowerBase()
{
	PrimaryActorTick.bCanEverTick = false;

	TowerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TowerMesh"));
	RootComponent = TowerMesh;
}

void AFluidTowerBase::BeginPlay()
{
	Super::BeginPlay();

	FluidSubsystem = GetWorld()->GetSubsystem<UFluidSubsystem>();

	GetWorldTimerManager().SetTimer(
		EffectTimerHandle,
		FTimerDelegate::CreateUObject(this, &AFluidTowerBase::ExecuteEffect),
		EffectInterval,
		/*bLoop=*/true
	);
}

void AFluidTowerBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(EffectTimerHandle);
	Super::EndPlay(EndPlayReason);
}

void AFluidTowerBase::ExecuteEffect()
{
	// Override in subclasses
}

void AFluidTowerBase::ApplyDamage(float DamageAmount)
{
	if (DamageAmount <= 0.f || Health <= 0.f) { return; }
	Health = FMath::Max(0.f, Health - DamageAmount);

	if (Health <= 0.f)
	{
		OnDestroyed_Internal();
	}
}

void AFluidTowerBase::OnDestroyed_Internal()
{
	GetWorldTimerManager().ClearTimer(EffectTimerHandle);
	Destroy();
}
