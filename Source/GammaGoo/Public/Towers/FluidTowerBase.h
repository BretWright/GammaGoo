// Copyright 2026 Bret Wright. All Rights Reserved.
// Abstract base class for all defensive towers. Timer-driven effects, never Tick.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FluidTowerBase.generated.h"

class UFluidSubsystem;
class UStaticMeshComponent;

UCLASS(Abstract, BlueprintType, Blueprintable)
class GAMMAGOO_API AFluidTowerBase : public AActor
{
	GENERATED_BODY()

public:
	AFluidTowerBase();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Override in subclasses to apply tower-specific effect. */
	UFUNCTION(BlueprintCallable, Category = "Tower")
	virtual void ExecuteEffect();

	UFUNCTION(BlueprintCallable, Category = "Tower")
	void ApplyDamage(float DamageAmount);

	UFUNCTION(BlueprintPure, Category = "Tower")
	float GetBuildCost() const { return BuildCost; }

	UFUNCTION(BlueprintPure, Category = "Tower")
	bool IsAlive() const { return Health > 0.f; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tower")
	TObjectPtr<UStaticMeshComponent> TowerMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower|Stats")
	float EffectRadius = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower|Stats")
	float EffectInterval = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower|Stats")
	float BuildCost = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower|Stats")
	float Health = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower|Stats")
	float MaxHealth = 100.f;

	UPROPERTY()
	TObjectPtr<UFluidSubsystem> FluidSubsystem = nullptr;

private:
	FTimerHandle EffectTimerHandle;

	void OnDestroyed_Internal();
};
