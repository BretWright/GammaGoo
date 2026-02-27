// Copyright 2026 Bret Wright. All Rights Reserved.
// Levee Wall — CONTAIN type. Blocks fluid flow, takes pressure damage.

#pragma once

#include "CoreMinimal.h"
#include "Towers/FluidTowerBase.h"
#include "LeveeWall.generated.h"

UCLASS(BlueprintType, Blueprintable)
class GAMMAGOO_API ALeveeWall : public AFluidTowerBase
{
	GENERATED_BODY()

public:
	ALeveeWall();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void ExecuteEffect() override;

protected:
	/** Number of cells this wall spans along its forward axis. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower|Levee")
	int32 WallLength = 3;

private:
	/** Grid cells this wall occupies. Blocked on spawn, unblocked on destroy. */
	TArray<FIntPoint> OccupiedCells;

	void BlockCells(bool bBlock);
};
