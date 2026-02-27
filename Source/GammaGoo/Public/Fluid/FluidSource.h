// Copyright 2026 Bret Wright. All Rights Reserved.
// AFluidSource is a placeable actor that injects fluid into the subsystem grid.
// Activated and deactivated by AWaveManager to control wave flow.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FluidSource.generated.h"

class UFluidSubsystem;
class USceneComponent;

UCLASS(BlueprintType, Blueprintable)
class GAMMAGOO_API AFluidSource : public AActor
{
	GENERATED_BODY()

public:
	AFluidSource();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "Fluid|Source")
	void Activate();

	UFUNCTION(BlueprintCallable, Category = "Fluid|Source")
	void Deactivate();

protected:
	/** Volume of fluid added per SpawnInterval. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid|Source")
	float SpawnRate = 10.f;

	/** How often fluid is injected (seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid|Source",
		meta = (ClampMin = "0.016"))
	float SpawnInterval = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid|Source")
	bool bActiveOnBeginPlay = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fluid|Source")
	bool bActive = false;

private:
	void SpawnFluid();

	FTimerHandle SpawnTimerHandle;

	UPROPERTY()
	TObjectPtr<UFluidSubsystem> FluidSubsystem = nullptr;

	/** Cached grid coordinate. Computed once in BeginPlay — source must not move. */
	FIntPoint CachedCellCoord = FIntPoint(INDEX_NONE, INDEX_NONE);
};
