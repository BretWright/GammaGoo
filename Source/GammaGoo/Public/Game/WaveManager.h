// Copyright 2026 Bret Wright. All Rights Reserved.
// AWaveManager drives a 5-wave escalation sequence with build phases, basin triggers,
// and spawn-rate multipliers. All source activation is routed through this actor.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WaveManager.generated.h"

class AFluidSource;
class ATownHall;
class UFluidSubsystem;

// ---------------------------------------------------------------------------
// Wave state machine
// ---------------------------------------------------------------------------
UENUM(BlueprintType)
enum class EWaveState : uint8
{
	PreGame		UMETA(DisplayName = "Pre-Game"),
	BuildPhase	UMETA(DisplayName = "Build Phase"),
	WaveActive	UMETA(DisplayName = "Wave Active"),
	Victory		UMETA(DisplayName = "Victory"),
	Defeat		UMETA(DisplayName = "Defeat")
};

// ---------------------------------------------------------------------------
// Per-wave config data
// ---------------------------------------------------------------------------
USTRUCT(BlueprintType)
struct FWaveConfig
{
	GENERATED_BODY()

	/** How many regular sources should be active during this wave. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	int32 SourceCount = 1;

	/** Multiplier applied to each source's base SpawnRate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	float SpawnRateMultiplier = 1.f;

	/** Seconds the wave runs before transitioning to the next build phase. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	float Duration = 60.f;

	/** If true, a basin source can trigger mid-wave when total fluid exceeds the threshold. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	bool bBasinTriggerEnabled = false;
};

// ---------------------------------------------------------------------------
// Delegates
// ---------------------------------------------------------------------------
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWaveStateChanged, EWaveState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWaveNumberChanged, int32, WaveNumber, int32, TotalWaves);

// ---------------------------------------------------------------------------
// AWaveManager
// ---------------------------------------------------------------------------
UCLASS(BlueprintType, Blueprintable)
class GAMMAGOO_API AWaveManager : public AActor
{
	GENERATED_BODY()

public:
	AWaveManager();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// --- Delegates --------------------------------------------------------
	UPROPERTY(BlueprintAssignable, Category = "Wave")
	FOnWaveStateChanged OnWaveStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Wave")
	FOnWaveNumberChanged OnWaveNumberChanged;

	// --- Public getters ---------------------------------------------------
	UFUNCTION(BlueprintPure, Category = "Wave")
	EWaveState GetWaveState() const { return WaveState; }

	UFUNCTION(BlueprintPure, Category = "Wave")
	int32 GetCurrentWaveNumber() const { return CurrentWaveIndex + 1; }

	UFUNCTION(BlueprintPure, Category = "Wave")
	int32 GetTotalWaves() const { return WaveConfigs.Num(); }

	UFUNCTION(BlueprintPure, Category = "Wave")
	float GetWaveTimeRemaining() const;

	UFUNCTION(BlueprintPure, Category = "Wave")
	float GetBuildPhaseTimeRemaining() const;

protected:
	// --- Configuration (editable in editor) --------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave|Config")
	TArray<FWaveConfig> WaveConfigs;

	/** Duration of the build phase between waves (seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave|Config")
	float BuildPhaseDuration = 30.f;

	/** Total fluid volume that triggers basin sources during qualifying waves. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave|Config")
	float BasinTriggerThreshold = 5000.f;

	/** How often the basin trigger volume check runs during a wave (seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave|Config")
	float BasinCheckInterval = 1.f;

private:
	// --- State -------------------------------------------------------------
	EWaveState WaveState = EWaveState::PreGame;
	int32 CurrentWaveIndex = -1;

	// --- Actor references --------------------------------------------------
	UPROPERTY()
	TArray<TObjectPtr<AFluidSource>> RegularSources;

	UPROPERTY()
	TArray<TObjectPtr<AFluidSource>> BasinSources;

	/** Base spawn rate stored per-source before any multiplier is applied. */
	TArray<float> BaseSpawnRates;

	/** Base spawn rates for basin sources (avoids cumulative multiplication). */
	TArray<float> BaseBasinSpawnRates;

	UPROPERTY()
	TObjectPtr<ATownHall> TownHall = nullptr;

	UPROPERTY()
	TObjectPtr<UFluidSubsystem> FluidSubsystem = nullptr;

	bool bBasinTriggeredThisWave = false;

	// --- Timers ------------------------------------------------------------
	FTimerHandle WaveDurationHandle;
	FTimerHandle BuildPhaseHandle;
	FTimerHandle BasinCheckHandle;

	// --- Internal flow -----------------------------------------------------
	void GatherWorldReferences();
	void SetWaveState(EWaveState NewState);
	void StartBuildPhase();
	void StartNextWave();
	void OnWaveDurationElapsed();
	void DeactivateAllSources();
	void ActivateSourcesForWave(const FWaveConfig& Config);
	void ApplySpawnRateMultiplier(float Multiplier);
	void CheckBasinTrigger();
	void TriggerVictory();

	UFUNCTION()
	void OnTownDestroyed();
};
