// Copyright 2026 Bret Wright. All Rights Reserved.

#include "Towers/RepulsorTower.h"
#include "Fluid/FluidSubsystem.h"

ARepulsorTower::ARepulsorTower()
{
	EffectRadius = 600.f;
	EffectInterval = 0.5f;
	BuildCost = 150.f;
	Health = 100.f;
	MaxHealth = 100.f;
}

void ARepulsorTower::ExecuteEffect()
{
	if (!FluidSubsystem || !IsAlive()) { return; }
	FluidSubsystem->ApplyRadialForceInRadius(GetActorLocation(), EffectRadius, PushStrength);
}
