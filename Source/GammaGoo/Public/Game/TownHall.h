// Copyright 2026 Bret Wright. All Rights Reserved.
// ATownHall is the player's settlement. Takes damage from fluid depth, broadcasts lose condition.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TownHall.generated.h"

class UFluidSubsystem;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTownDestroyed);

UCLASS(BlueprintType, Blueprintable)
class GAMMAGOO_API ATownHall : public AActor
{
	GENERATED_BODY()

public:
	ATownHall();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintPure, Category = "TownHall")
	float GetHealthPercent() const { return MaxHealth > 0.f ? Health / MaxHealth : 0.f; }

	UPROPERTY(BlueprintAssignable, Category = "TownHall")
	FOnTownDestroyed OnTownDestroyed;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TownHall")
	TObjectPtr<UStaticMeshComponent> BuildingMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TownHall")
	float Health = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TownHall")
	float MaxHealth = 1000.f;

	/** Damage per second per cm of fluid depth above the base. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TownHall")
	float DamagePerDepthPerSecond = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TownHall")
	float DamageCheckInterval = 0.5f;

private:
	void CheckFluidDamage();

	FTimerHandle DamageTimerHandle;

	UPROPERTY()
	TObjectPtr<UFluidSubsystem> FluidSubsystem = nullptr;

	float BaseZ = 0.f;
	bool bDestroyed = false;
};
