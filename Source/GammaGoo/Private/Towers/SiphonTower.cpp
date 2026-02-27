// Copyright 2026 Bret Wright. All Rights Reserved.

#include "Towers/SiphonTower.h"
#include "Fluid/FluidSubsystem.h"
#include "Game/ResourceSubsystem.h"
#include "Engine/World.h"

ASiphonTower::ASiphonTower()
{
	EffectRadius = 400.f;
	EffectInterval = 0.5f;
	BuildCost = 75.f;
	Health = 100.f;
	MaxHealth = 100.f;
}

void ASiphonTower::ExecuteEffect()
{
	if (!FluidSubsystem || !IsAlive()) { return; }

	// Measure fluid before and after to compute actual removal
	const float VolumeBefore = FluidSubsystem->GetTotalFluidVolume();
	FluidSubsystem->RemoveFluidInRadius(GetActorLocation(), EffectRadius, DrainAmount);
	const float VolumeAfter = FluidSubsystem->GetTotalFluidVolume();

	const float Removed = VolumeBefore - VolumeAfter;
	if (Removed > 0.f)
	{
		if (UResourceSubsystem* Res = GetGameInstance()->GetSubsystem<UResourceSubsystem>())
		{
			Res->AddCurrency(Removed * ConversionRatio);
		}
	}
}
