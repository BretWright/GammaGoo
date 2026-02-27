// Copyright 2026 Bret Wright. All Rights Reserved.

#include "Game/WaveManager.h"
#include "Fluid/FluidSource.h"
#include "Fluid/FluidSubsystem.h"
#include "Game/TownHall.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "EngineUtils.h"

AWaveManager::AWaveManager()
{
	PrimaryActorTick.bCanEverTick = false;

	// ---- Default 5-wave configuration ----
	WaveConfigs.SetNum(5);

	// Wave 1: 1 source, 1.0x rate, 60s
	WaveConfigs[0].SourceCount = 1;
	WaveConfigs[0].SpawnRateMultiplier = 1.0f;
	WaveConfigs[0].Duration = 60.f;
	WaveConfigs[0].bBasinTriggerEnabled = false;

	// Wave 2: 2 sources, 1.2x rate, 75s
	WaveConfigs[1].SourceCount = 2;
	WaveConfigs[1].SpawnRateMultiplier = 1.2f;
	WaveConfigs[1].Duration = 75.f;
	WaveConfigs[1].bBasinTriggerEnabled = false;

	// Wave 3: 2 sources, 1.5x rate, 90s, basin trigger
	WaveConfigs[2].SourceCount = 2;
	WaveConfigs[2].SpawnRateMultiplier = 1.5f;
	WaveConfigs[2].Duration = 90.f;
	WaveConfigs[2].bBasinTriggerEnabled = true;

	// Wave 4: 3 sources, 1.8x rate, 105s, basin trigger
	WaveConfigs[3].SourceCount = 3;
	WaveConfigs[3].SpawnRateMultiplier = 1.8f;
	WaveConfigs[3].Duration = 105.f;
	WaveConfigs[3].bBasinTriggerEnabled = true;

	// Wave 5: 3 sources, 2.5x rate, 120s, basin trigger
	WaveConfigs[4].SourceCount = 3;
	WaveConfigs[4].SpawnRateMultiplier = 2.5f;
	WaveConfigs[4].Duration = 120.f;
	WaveConfigs[4].bBasinTriggerEnabled = true;
}

// ---------------------------------------------------------------------------
// BeginPlay / EndPlay
// ---------------------------------------------------------------------------

void AWaveManager::BeginPlay()
{
	Super::BeginPlay();

	GatherWorldReferences();

	// Safety: deactivate all sources regardless of bActiveOnBeginPlay
	DeactivateAllSources();

	// Bind the Town Hall lose condition
	if (TownHall)
	{
		TownHall->OnTownDestroyed.AddDynamic(this, &AWaveManager::OnTownDestroyed);
	}

	// Auto-start: transition to build phase for wave 1
	StartBuildPhase();
}

void AWaveManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(WaveDurationHandle);
	GetWorldTimerManager().ClearTimer(BuildPhaseHandle);
	GetWorldTimerManager().ClearTimer(BasinCheckHandle);

	Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------------------
// World references
// ---------------------------------------------------------------------------

void AWaveManager::GatherWorldReferences()
{
	UWorld* World = GetWorld();
	if (!World) { return; }

	FluidSubsystem = World->GetSubsystem<UFluidSubsystem>();

	// Gather fluid sources by actor tag
	for (TActorIterator<AFluidSource> It(World); It; ++It)
	{
		AFluidSource* Source = *It;
		if (!Source) { continue; }

		if (Source->ActorHasTag(FName(TEXT("BasinSource"))))
		{
			BasinSources.Add(Source);
		}
		else if (Source->ActorHasTag(FName(TEXT("FluidSource"))))
		{
			RegularSources.Add(Source);
		}
	}

	// Store base spawn rates before any multipliers are applied
	BaseSpawnRates.SetNum(RegularSources.Num());
	for (int32 i = 0; i < RegularSources.Num(); ++i)
	{
		if (RegularSources[i])
		{
			BaseSpawnRates[i] = RegularSources[i]->GetSpawnRate();
		}
	}

	BaseBasinSpawnRates.SetNum(BasinSources.Num());
	for (int32 i = 0; i < BasinSources.Num(); ++i)
	{
		if (BasinSources[i])
		{
			BaseBasinSpawnRates[i] = BasinSources[i]->GetSpawnRate();
		}
	}

	// Find the Town Hall
	for (TActorIterator<ATownHall> It(World); It; ++It)
	{
		TownHall = *It;
		break; // Only one town hall expected
	}

	UE_LOG(LogTemp, Log,
		TEXT("AWaveManager: Found %d regular sources, %d basin sources, TownHall=%s"),
		RegularSources.Num(), BasinSources.Num(),
		TownHall ? *TownHall->GetName() : TEXT("NONE"));
}

// ---------------------------------------------------------------------------
// State machine
// ---------------------------------------------------------------------------

void AWaveManager::SetWaveState(EWaveState NewState)
{
	if (WaveState == NewState) { return; }
	WaveState = NewState;
	OnWaveStateChanged.Broadcast(NewState);
}

// ---------------------------------------------------------------------------
// Build phase
// ---------------------------------------------------------------------------

void AWaveManager::StartBuildPhase()
{
	SetWaveState(EWaveState::BuildPhase);

	DeactivateAllSources();

	// Schedule transition to the next wave after build phase duration
	GetWorldTimerManager().SetTimer(
		BuildPhaseHandle,
		FTimerDelegate::CreateUObject(this, &AWaveManager::StartNextWave),
		BuildPhaseDuration,
		/*bLoop=*/false
	);
}

// ---------------------------------------------------------------------------
// Wave activation
// ---------------------------------------------------------------------------

void AWaveManager::StartNextWave()
{
	CurrentWaveIndex++;

	if (CurrentWaveIndex >= WaveConfigs.Num())
	{
		// All waves cleared
		TriggerVictory();
		return;
	}

	const FWaveConfig& Config = WaveConfigs[CurrentWaveIndex];

	SetWaveState(EWaveState::WaveActive);
	OnWaveNumberChanged.Broadcast(CurrentWaveIndex + 1, WaveConfigs.Num());

	// Reset basin trigger tracking for this wave
	bBasinTriggeredThisWave = false;

	// Apply spawn rate multiplier before activating
	ApplySpawnRateMultiplier(Config.SpawnRateMultiplier);

	// Activate the required number of regular sources
	ActivateSourcesForWave(Config);

	// Schedule wave end
	GetWorldTimerManager().SetTimer(
		WaveDurationHandle,
		FTimerDelegate::CreateUObject(this, &AWaveManager::OnWaveDurationElapsed),
		Config.Duration,
		/*bLoop=*/false
	);

	// Start basin volume check if enabled for this wave
	if (Config.bBasinTriggerEnabled)
	{
		GetWorldTimerManager().SetTimer(
			BasinCheckHandle,
			FTimerDelegate::CreateUObject(this, &AWaveManager::CheckBasinTrigger),
			BasinCheckInterval,
			/*bLoop=*/true
		);
	}

	UE_LOG(LogTemp, Log,
		TEXT("AWaveManager: Wave %d started — %d sources, %.1fx rate, %.0fs duration, basin=%s"),
		CurrentWaveIndex + 1, Config.SourceCount, Config.SpawnRateMultiplier,
		Config.Duration, Config.bBasinTriggerEnabled ? TEXT("ON") : TEXT("OFF"));
}

// ---------------------------------------------------------------------------
// Wave duration elapsed
// ---------------------------------------------------------------------------

void AWaveManager::OnWaveDurationElapsed()
{
	GetWorldTimerManager().ClearTimer(BasinCheckHandle);

	if (CurrentWaveIndex + 1 >= WaveConfigs.Num())
	{
		// Last wave completed
		TriggerVictory();
	}
	else
	{
		StartBuildPhase();
	}
}

// ---------------------------------------------------------------------------
// Source management
// ---------------------------------------------------------------------------

void AWaveManager::DeactivateAllSources()
{
	for (AFluidSource* Source : RegularSources)
	{
		if (Source) { Source->Deactivate(); }
	}
	for (AFluidSource* Source : BasinSources)
	{
		if (Source) { Source->Deactivate(); }
	}
}

void AWaveManager::ActivateSourcesForWave(const FWaveConfig& Config)
{
	const int32 Count = FMath::Min(Config.SourceCount, RegularSources.Num());
	for (int32 i = 0; i < Count; ++i)
	{
		if (RegularSources[i])
		{
			RegularSources[i]->Activate();
		}
	}
}

void AWaveManager::ApplySpawnRateMultiplier(float Multiplier)
{
	for (int32 i = 0; i < RegularSources.Num(); ++i)
	{
		if (RegularSources[i] && BaseSpawnRates.IsValidIndex(i))
		{
			RegularSources[i]->SetSpawnRate(BaseSpawnRates[i] * Multiplier);
		}
	}
}

// ---------------------------------------------------------------------------
// Basin trigger
// ---------------------------------------------------------------------------

void AWaveManager::CheckBasinTrigger()
{
	if (bBasinTriggeredThisWave || !FluidSubsystem) { return; }

	const float TotalVolume = FluidSubsystem->GetTotalFluidVolume();
	if (TotalVolume > BasinTriggerThreshold)
	{
		bBasinTriggeredThisWave = true;

		// Activate all basin sources with the current wave's multiplier applied to base rate
		const FWaveConfig& Config = WaveConfigs[CurrentWaveIndex];
		for (int32 i = 0; i < BasinSources.Num(); ++i)
		{
			if (BasinSources[i] && BaseBasinSpawnRates.IsValidIndex(i))
			{
				BasinSources[i]->SetSpawnRate(BaseBasinSpawnRates[i] * Config.SpawnRateMultiplier);
				BasinSources[i]->Activate();
			}
		}

		UE_LOG(LogTemp, Log,
			TEXT("AWaveManager: Basin trigger activated! Total volume %.0f > threshold %.0f"),
			TotalVolume, BasinTriggerThreshold);
	}
}

// ---------------------------------------------------------------------------
// End conditions
// ---------------------------------------------------------------------------

void AWaveManager::TriggerVictory()
{
	DeactivateAllSources();
	GetWorldTimerManager().ClearTimer(WaveDurationHandle);
	GetWorldTimerManager().ClearTimer(BuildPhaseHandle);
	GetWorldTimerManager().ClearTimer(BasinCheckHandle);
	SetWaveState(EWaveState::Victory);

	UE_LOG(LogTemp, Log, TEXT("AWaveManager: VICTORY — All %d waves survived!"), WaveConfigs.Num());
}

void AWaveManager::OnTownDestroyed()
{
	DeactivateAllSources();
	GetWorldTimerManager().ClearTimer(WaveDurationHandle);
	GetWorldTimerManager().ClearTimer(BuildPhaseHandle);
	GetWorldTimerManager().ClearTimer(BasinCheckHandle);
	SetWaveState(EWaveState::Defeat);

	UE_LOG(LogTemp, Warning, TEXT("AWaveManager: DEFEAT — Town Hall destroyed on wave %d."),
		CurrentWaveIndex + 1);
}

// ---------------------------------------------------------------------------
// Time remaining queries
// ---------------------------------------------------------------------------

float AWaveManager::GetWaveTimeRemaining() const
{
	if (WaveState != EWaveState::WaveActive) { return 0.f; }
	return GetWorldTimerManager().GetTimerRemaining(WaveDurationHandle);
}

float AWaveManager::GetBuildPhaseTimeRemaining() const
{
	if (WaveState != EWaveState::BuildPhase) { return 0.f; }
	return GetWorldTimerManager().GetTimerRemaining(BuildPhaseHandle);
}
