// Copyright 2026 Bret Wright. All Rights Reserved.

#include "Towers/EvaporatorTower.h"
#include "Fluid/FluidSubsystem.h"

AEvaporatorTower::AEvaporatorTower()
{
	EffectRadius = 500.f;
	EffectInterval = 0.5f;
	BuildCost = 100.f;
	Health = 100.f;
	MaxHealth = 100.f;
}

void AEvaporatorTower::ExecuteEffect()
{
	if (!FluidSubsystem || !IsAlive()) { return; }
	FluidSubsystem->RemoveFluidInRadius(GetActorLocation(), EffectRadius, EvaporateAmount);
}
