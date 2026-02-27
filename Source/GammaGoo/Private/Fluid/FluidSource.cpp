// Copyright 2026 Bret Wright. All Rights Reserved.

#include "Fluid/FluidSource.h"
#include "Fluid/FluidSubsystem.h"
#include "Engine/World.h"
#include "TimerManager.h"

AFluidSource::AFluidSource()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AFluidSource::BeginPlay()
{
	Super::BeginPlay();

	FluidSubsystem = GetWorld()->GetSubsystem<UFluidSubsystem>();
	if (!ensure(FluidSubsystem))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("AFluidSource '%s': UFluidSubsystem not found. Source disabled."),
			*GetName());
		return;
	}

	CachedCellCoord = FluidSubsystem->WorldToCell(GetActorLocation());

	if (bActiveOnBeginPlay)
	{
		Activate();
	}
}

void AFluidSource::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Deactivate();
	Super::EndPlay(EndPlayReason);
}

void AFluidSource::Activate()
{
	if (bActive || !FluidSubsystem) { return; }
	bActive = true;

	GetWorldTimerManager().SetTimer(
		SpawnTimerHandle,
		FTimerDelegate::CreateUObject(this, &AFluidSource::SpawnFluid),
		SpawnInterval,
		/*bLoop=*/true
	);
}

void AFluidSource::Deactivate()
{
	bActive = false;
	GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
}

void AFluidSource::SpawnFluid()
{
	if (!FluidSubsystem || !FluidSubsystem->IsValidCell(CachedCellCoord.X, CachedCellCoord.Y))
	{
		return;
	}
	FluidSubsystem->AddFluidAtCell(CachedCellCoord.X, CachedCellCoord.Y, SpawnRate);
}
