// Copyright 2026 Bret Wright. All Rights Reserved.

#include "UI/FluidSiegeHUD.h"
#include "Game/WaveManager.h"
#include "Game/ResourceSubsystem.h"
#include "Game/TownHall.h"
#include "Player/FluidDefenseCharacter.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Engine/World.h"
#include "EngineUtils.h"

// ---------------------------------------------------------------------------
// InitHUD
// ---------------------------------------------------------------------------

void UFluidSiegeHUD::InitHUD(AFluidDefenseCharacter* InPlayer)
{
	CachedPlayer = InPlayer;
	CacheReferences();
}

// ---------------------------------------------------------------------------
// CacheReferences — find world actors and subsystems
// ---------------------------------------------------------------------------

void UFluidSiegeHUD::CacheReferences()
{
	UWorld* World = GetWorld();
	if (!World) { return; }

	// Wave Manager — single instance in the level
	if (!CachedWaveManager)
	{
		for (TActorIterator<AWaveManager> It(World); It; ++It)
		{
			CachedWaveManager = *It;
			break;
		}
	}

	// Town Hall — single instance in the level
	if (!CachedTownHall)
	{
		for (TActorIterator<ATownHall> It(World); It; ++It)
		{
			CachedTownHall = *It;
			break;
		}
	}

	// Resource Subsystem — GameInstance subsystem
	if (!CachedResourceSubsystem)
	{
		if (UGameInstance* GI = World->GetGameInstance())
		{
			CachedResourceSubsystem = GI->GetSubsystem<UResourceSubsystem>();
		}
	}
}

// ---------------------------------------------------------------------------
// NativeTick — poll gameplay state and update widgets
// ---------------------------------------------------------------------------

void UFluidSiegeHUD::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Re-cache if any reference is lost (e.g., late spawn)
	if (!CachedWaveManager || !CachedTownHall || !CachedResourceSubsystem)
	{
		CacheReferences();
	}

	// --- Wave text: "Wave 3 / 5" ---
	if (WaveText && CachedWaveManager)
	{
		WaveText->SetText(FText::FromString(
			FString::Printf(TEXT("Wave %d / %d"),
				CachedWaveManager->GetCurrentWaveNumber(),
				CachedWaveManager->GetTotalWaves())));
	}

	// --- Timer text & State text ---
	if (CachedWaveManager)
	{
		const EWaveState State = CachedWaveManager->GetWaveState();
		FString TimerStr;
		FString StateStr;

		switch (State)
		{
		case EWaveState::PreGame:
			TimerStr = TEXT("WAITING");
			StateStr = TEXT("Pre-Game");
			break;

		case EWaveState::BuildPhase:
		{
			const float Remaining = CachedWaveManager->GetBuildPhaseTimeRemaining();
			TimerStr = FString::Printf(TEXT("BUILD %ds"), FMath::CeilToInt(Remaining));
			StateStr = TEXT("Build Phase");
			break;
		}

		case EWaveState::WaveActive:
		{
			const float Remaining = CachedWaveManager->GetWaveTimeRemaining();
			TimerStr = FString::Printf(TEXT("WAVE %ds"), FMath::CeilToInt(Remaining));
			StateStr = TEXT("Wave Active");
			break;
		}

		case EWaveState::Victory:
			TimerStr = TEXT("VICTORY");
			StateStr = TEXT("VICTORY!");
			break;

		case EWaveState::Defeat:
			TimerStr = TEXT("DEFEAT");
			StateStr = TEXT("DEFEATED");
			break;
		}

		if (TimerText)
		{
			TimerText->SetText(FText::FromString(TimerStr));
		}
		if (StateText)
		{
			StateText->SetText(FText::FromString(StateStr));
		}
	}

	// --- Resource text: "500" ---
	if (ResourceText && CachedResourceSubsystem)
	{
		ResourceText->SetText(FText::FromString(
			FString::Printf(TEXT("%.0f"), CachedResourceSubsystem->GetCurrency())));
	}

	// --- Town Hall health bar and text ---
	if (CachedTownHall)
	{
		const float HealthPct = CachedTownHall->GetHealthPercent();

		if (TownHallHealthBar)
		{
			TownHallHealthBar->SetPercent(HealthPct);

			// Red when below 25%, green otherwise
			const FLinearColor BarColor = (HealthPct < 0.25f)
				? FLinearColor::Red
				: FLinearColor::Green;
			TownHallHealthBar->SetFillColorAndOpacity(BarColor);
		}

		if (TownHallHealthText)
		{
			TownHallHealthText->SetText(FText::FromString(
				FString::Printf(TEXT("Town Hall %d%%"), FMath::RoundToInt(HealthPct * 100.f))));
		}
	}

	// --- Energy bar and text ---
	if (CachedPlayer)
	{
		const float EnergyPct = CachedPlayer->GetEnergyPercent();

		if (EnergyBar)
		{
			EnergyBar->SetPercent(EnergyPct);
		}

		if (EnergyText)
		{
			EnergyText->SetText(FText::FromString(
				FString::Printf(TEXT("Energy %d%%"), FMath::RoundToInt(EnergyPct * 100.f))));
		}
	}
}
