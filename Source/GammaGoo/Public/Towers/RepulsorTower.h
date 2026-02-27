// Copyright 2026 Bret Wright. All Rights Reserved.
// Repulsor Tower — DISPLACE type. Pushes fluid radially outward.

#pragma once

#include "CoreMinimal.h"
#include "Towers/FluidTowerBase.h"
#include "RepulsorTower.generated.h"

UCLASS(BlueprintType, Blueprintable)
class GAMMAGOO_API ARepulsorTower : public AFluidTowerBase
{
	GENERATED_BODY()

public:
	ARepulsorTower();
	virtual void ExecuteEffect() override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower|Repulsor")
	float PushStrength = 200.f;
};
