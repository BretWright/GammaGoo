// Copyright 2026 Bret Wright. All Rights Reserved.
// Cryo Spike — CONTAIN type. Freezes fluid in radius on a cycle with cooldown.

#pragma once

#include "CoreMinimal.h"
#include "Towers/FluidTowerBase.h"
#include "CryoSpike.generated.h"

UCLASS(BlueprintType, Blueprintable)
class GAMMAGOO_API ACryoSpike : public AFluidTowerBase
{
	GENERATED_BODY()

public:
	ACryoSpike();
	virtual void ExecuteEffect() override;

protected:
	/** How long the freeze persists before thawing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower|Cryo")
	float FreezeDuration = 30.f;

	/** Cooldown after thaw before re-freezing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower|Cryo")
	float CooldownDuration = 5.f;

private:
	bool bFreezeActive = false;
	bool bOnCooldown = false;
	FTimerHandle ThawTimerHandle;
	FTimerHandle CooldownTimerHandle;

	void Freeze();
	void Thaw();
	void CooldownComplete();
};
