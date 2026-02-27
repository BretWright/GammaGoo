// Copyright 2026 Bret Wright. All Rights Reserved.

#include "Towers/CryoSpike.h"
#include "Fluid/FluidSubsystem.h"
#include "TimerManager.h"

ACryoSpike::ACryoSpike()
{
	EffectRadius = 450.f;
	EffectInterval = 1.0f; // Check interval for auto-cycle trigger
	BuildCost = 200.f;
	Health = 100.f;
	MaxHealth = 100.f;
}

void ACryoSpike::ExecuteEffect()
{
	// Auto-cycle: freeze if not active and not on cooldown
	if (!FluidSubsystem || !IsAlive()) { return; }
	if (!bFreezeActive && !bOnCooldown)
	{
		Freeze();
	}
}

void ACryoSpike::Freeze()
{
	if (!FluidSubsystem) { return; }
	bFreezeActive = true;
	FluidSubsystem->SetFrozenInRadius(GetActorLocation(), EffectRadius, true);

	GetWorldTimerManager().SetTimer(
		ThawTimerHandle,
		FTimerDelegate::CreateUObject(this, &ACryoSpike::Thaw),
		FreezeDuration,
		/*bLoop=*/false
	);
}

void ACryoSpike::Thaw()
{
	if (!FluidSubsystem) { return; }
	bFreezeActive = false;
	FluidSubsystem->SetFrozenInRadius(GetActorLocation(), EffectRadius, false);

	bOnCooldown = true;
	GetWorldTimerManager().SetTimer(
		CooldownTimerHandle,
		FTimerDelegate::CreateUObject(this, &ACryoSpike::CooldownComplete),
		CooldownDuration,
		/*bLoop=*/false
	);
}

void ACryoSpike::CooldownComplete()
{
	bOnCooldown = false;
}
