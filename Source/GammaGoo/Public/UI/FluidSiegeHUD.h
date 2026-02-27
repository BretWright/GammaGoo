// Copyright 2026 Bret Wright. All Rights Reserved.
// UFluidSiegeHUD displays wave state, resources, Town Hall health, and player energy via NativeTick polling.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FluidSiegeHUD.generated.h"

class AWaveManager;
class UResourceSubsystem;
class ATownHall;
class AFluidDefenseCharacter;
class UTextBlock;
class UProgressBar;

UCLASS(BlueprintType, Blueprintable)
class GAMMAGOO_API UFluidSiegeHUD : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Cache the owning player character and resolve world references. */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void InitHUD(AFluidDefenseCharacter* InPlayer);

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// --- Bound widgets (all optional — HUD works even without a Blueprint layout) ---

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WaveText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TimerText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StateText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ResourceText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TownHallHealthText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> TownHallHealthBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EnergyText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> EnergyBar;

private:
	void CacheReferences();

	UPROPERTY()
	TObjectPtr<AWaveManager> CachedWaveManager = nullptr;

	UPROPERTY()
	TObjectPtr<UResourceSubsystem> CachedResourceSubsystem = nullptr;

	UPROPERTY()
	TObjectPtr<ATownHall> CachedTownHall = nullptr;

	UPROPERTY()
	TObjectPtr<AFluidDefenseCharacter> CachedPlayer = nullptr;
};
