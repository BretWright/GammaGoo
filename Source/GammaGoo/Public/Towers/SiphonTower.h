// Copyright 2026 Bret Wright. All Rights Reserved.
// Siphon Well — EXPLOIT type. Drains fluid and converts to player currency.

#pragma once

#include "CoreMinimal.h"
#include "Towers/FluidTowerBase.h"
#include "SiphonTower.generated.h"

UCLASS(BlueprintType, Blueprintable)
class GAMMAGOO_API ASiphonTower : public AFluidTowerBase
{
	GENERATED_BODY()

public:
	ASiphonTower();
	virtual void ExecuteEffect() override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower|Siphon")
	float DrainAmount = 30.f;

	/** Resource gained per 1.0 volume drained. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower|Siphon")
	float ConversionRatio = 0.5f;
};
