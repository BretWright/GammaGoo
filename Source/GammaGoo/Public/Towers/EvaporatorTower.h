// Copyright 2026 Bret Wright. All Rights Reserved.
// Evaporator Pylon — DESTROY type. Removes fluid volume in radius.

#pragma once

#include "CoreMinimal.h"
#include "Towers/FluidTowerBase.h"
#include "EvaporatorTower.generated.h"

UCLASS(BlueprintType, Blueprintable)
class GAMMAGOO_API AEvaporatorTower : public AFluidTowerBase
{
	GENERATED_BODY()

public:
	AEvaporatorTower();
	virtual void ExecuteEffect() override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower|Evaporator")
	float EvaporateAmount = 50.f;
};
