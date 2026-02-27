# Week 4: Waves + HUD Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Implement the 5-wave sequence manager with escalating fluid pressure, build phases, basin trigger, and a gameplay HUD showing wave state, resources, Town Hall HP, and player energy — completing the full playable PoC loop.

**Architecture:** AWaveManager (AActor placed in level) owns wave state machine, references AFluidSource actors via level tags, activates/deactivates them per wave config. UFluidSiegeHUD (UUserWidget) is a C++ widget base class with bound text/progress bars; created and added to viewport by AFluidDefenseCharacter on BeginPlay. ATownHall gets an OnHealthChanged delegate for HUD binding.

**Tech Stack:** UE 5.7 C++, UMG (Slate/UMG widgets), Timer-driven state machine, Dynamic multicast delegates

---

## Dependency Order

```
Task 1: ATownHall OnHealthChanged delegate (modify existing — HUD needs it)
Task 2: AWaveManager (needs AFluidSource, UFluidSubsystem, ATownHall)
Task 3: UFluidSiegeHUD widget (needs AWaveManager, UResourceSubsystem, ATownHall, AFluidDefenseCharacter)
Task 4: Wire HUD into AFluidDefenseCharacter (needs UFluidSiegeHUD)
Task 5: Final compile + commit
```

Tasks 1–2 are independent. Task 3 depends on 1–2. Task 4 depends on 3. Task 5 depends on all.

---

### Task 1: ATownHall — Add OnHealthChanged Delegate

**Files:**
- Modify: `Source/GammaGoo/Public/Game/TownHall.h`
- Modify: `Source/GammaGoo/Private/Game/TownHall.cpp`

**Context:** The HUD needs to update the Town Hall health bar whenever damage is taken. Currently ATownHall only broadcasts OnTownDestroyed at 0 HP. We need an OnHealthChanged delegate that fires every damage tick.

**Step 1: Add delegate to header**

In `TownHall.h`, add after the existing `FOnTownDestroyed` delegate declaration:

```cpp
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTownHealthChanged, float, CurrentHealth, float, MaxHealth);
```

Add as a public UPROPERTY after `OnTownDestroyed`:

```cpp
UPROPERTY(BlueprintAssignable, Category = "TownHall")
FOnTownHealthChanged OnHealthChanged;
```

**Step 2: Broadcast in CheckFluidDamage**

In `TownHall.cpp`, in `CheckFluidDamage()`, add after the line `Health = FMath::Max(0.f, Health - Damage);`:

```cpp
OnHealthChanged.Broadcast(Health, MaxHealth);
```

**Step 3: Commit**

```bash
git add Source/GammaGoo/Public/Game/TownHall.h Source/GammaGoo/Private/Game/TownHall.cpp
git commit -m "feat: add OnHealthChanged delegate to ATownHall for HUD binding"
```

---

### Task 2: AWaveManager — 5-Wave Sequence

**Files:**
- Create: `Source/GammaGoo/Public/Game/WaveManager.h`
- Create: `Source/GammaGoo/Private/Game/WaveManager.cpp`

**Context:** Placed actor that manages a 5-wave sequence. Each wave activates a set of AFluidSource actors (found by tag), escalates spawn rates, and runs until a duration expires. Between waves: 30s build phase where sources are deactivated but fluid persists and towers still work. Basin trigger: when GetTotalFluidVolume() > 5000, a bonus source activates mid-wave.

**Wave Configuration:**

| Wave | Sources Active | SpawnRate Multiplier | Duration (s) | Basin Trigger |
|------|---------------|---------------------|---------------|---------------|
| 1 | 1 | 1.0x | 60 | No |
| 2 | 2 | 1.2x | 75 | No |
| 3 | 2 | 1.5x | 90 | Yes (source 3) |
| 4 | 3 | 1.8x | 105 | Yes |
| 5 | 3 | 2.5x | 120 | Yes |

**Step 1: Create header**

```cpp
// Source/GammaGoo/Public/Game/WaveManager.h
// Copyright 2026 Bret Wright. All Rights Reserved.
// AWaveManager drives the 5-wave sequence, build phases, and basin trigger.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WaveManager.generated.h"

class AFluidSource;
class UFluidSubsystem;
class ATownHall;

UENUM(BlueprintType)
enum class EWaveState : uint8
{
	PreGame,      // Before first wave
	BuildPhase,   // Between waves — sources off, player builds
	WaveActive,   // Sources on, fluid rising
	Victory,      // All 5 waves survived
	Defeat        // Town Hall destroyed
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWaveStateChanged, EWaveState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWaveNumberChanged, int32, WaveNumber, int32, TotalWaves);

USTRUCT(BlueprintType)
struct FWaveConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 SourceCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SpawnRateMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Duration = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bBasinTriggerEnabled = false;
};

UCLASS(BlueprintType, Blueprintable)
class GAMMAGOO_API AWaveManager : public AActor
{
	GENERATED_BODY()

public:
	AWaveManager();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "Wave")
	void StartGame();

	UFUNCTION(BlueprintPure, Category = "Wave")
	EWaveState GetWaveState() const { return CurrentState; }

	UFUNCTION(BlueprintPure, Category = "Wave")
	int32 GetCurrentWaveNumber() const { return CurrentWaveIndex + 1; }

	UFUNCTION(BlueprintPure, Category = "Wave")
	int32 GetTotalWaves() const { return WaveConfigs.Num(); }

	UFUNCTION(BlueprintPure, Category = "Wave")
	float GetWaveTimeRemaining() const;

	UFUNCTION(BlueprintPure, Category = "Wave")
	float GetBuildPhaseTimeRemaining() const;

	// --- Delegates ---
	UPROPERTY(BlueprintAssignable, Category = "Wave")
	FOnWaveStateChanged OnWaveStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Wave")
	FOnWaveNumberChanged OnWaveNumberChanged;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave|Config")
	TArray<FWaveConfig> WaveConfigs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave|Config")
	float BuildPhaseDuration = 30.f;

	/** Volume threshold that triggers the basin source. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave|Config")
	float BasinTriggerVolume = 5000.f;

	/** Tag used to find AFluidSource actors in the level. Sources should be tagged "FluidSource". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave|Config")
	FName SourceTag = TEXT("FluidSource");

	/** Tag for the basin trigger source. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave|Config")
	FName BasinSourceTag = TEXT("BasinSource");

private:
	void SetState(EWaveState NewState);

	void StartBuildPhase();
	void StartWave();
	void EndWave();
	void OnWaveTimerExpired();
	void CheckBasinTrigger();

	void ActivateSources(int32 Count, float SpawnRateMultiplier);
	void DeactivateAllSources();

	UFUNCTION()
	void OnTownDestroyed();

	void GatherSources();

	// --- State ---
	EWaveState CurrentState = EWaveState::PreGame;
	int32 CurrentWaveIndex = -1;

	FTimerHandle WaveTimerHandle;
	FTimerHandle BuildPhaseTimerHandle;
	FTimerHandle BasinCheckTimerHandle;
	bool bBasinTriggered = false;

	// --- Cached references ---
	UPROPERTY()
	TArray<TObjectPtr<AFluidSource>> FluidSources;

	UPROPERTY()
	TObjectPtr<AFluidSource> BasinSource = nullptr;

	UPROPERTY()
	TObjectPtr<UFluidSubsystem> FluidSubsystem = nullptr;

	UPROPERTY()
	TObjectPtr<ATownHall> TownHallRef = nullptr;

	static constexpr int32 MaxWaves = 5;
};
```

**Step 2: Create implementation**

```cpp
// Source/GammaGoo/Private/Game/WaveManager.cpp
// Copyright 2026 Bret Wright. All Rights Reserved.

#include "Game/WaveManager.h"
#include "Fluid/FluidSource.h"
#include "Fluid/FluidSubsystem.h"
#include "Game/TownHall.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"

AWaveManager::AWaveManager()
{
	PrimaryActorTick.bCanEverTick = false;

	// Default 5-wave config
	WaveConfigs.SetNum(MaxWaves);
	WaveConfigs[0] = { 1, 1.0f, 60.f, false };
	WaveConfigs[1] = { 2, 1.2f, 75.f, false };
	WaveConfigs[2] = { 2, 1.5f, 90.f, true };
	WaveConfigs[3] = { 3, 1.8f, 105.f, true };
	WaveConfigs[4] = { 3, 2.5f, 120.f, true };
}

void AWaveManager::BeginPlay()
{
	Super::BeginPlay();

	FluidSubsystem = GetWorld()->GetSubsystem<UFluidSubsystem>();
	GatherSources();

	// Find TownHall
	for (TActorIterator<ATownHall> It(GetWorld()); It; ++It)
	{
		TownHallRef = *It;
		TownHallRef->OnTownDestroyed.AddDynamic(this, &AWaveManager::OnTownDestroyed);
		break;
	}

	// Deactivate all sources at start — wave manager controls them
	DeactivateAllSources();

	// Auto-start with a build phase
	StartGame();
}

void AWaveManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(WaveTimerHandle);
	GetWorldTimerManager().ClearTimer(BuildPhaseTimerHandle);
	GetWorldTimerManager().ClearTimer(BasinCheckTimerHandle);
	Super::EndPlay(EndPlayReason);
}

void AWaveManager::GatherSources()
{
	FluidSources.Empty();
	BasinSource = nullptr;

	for (TActorIterator<AFluidSource> It(GetWorld()); It; ++It)
	{
		AFluidSource* Source = *It;
		if (Source->ActorHasTag(BasinSourceTag))
		{
			BasinSource = Source;
		}
		else if (Source->ActorHasTag(SourceTag))
		{
			FluidSources.Add(Source);
		}
	}
}

void AWaveManager::StartGame()
{
	CurrentWaveIndex = -1;
	bBasinTriggered = false;
	StartBuildPhase();
}

void AWaveManager::SetState(EWaveState NewState)
{
	CurrentState = NewState;
	OnWaveStateChanged.Broadcast(NewState);
}

void AWaveManager::StartBuildPhase()
{
	SetState(EWaveState::BuildPhase);
	DeactivateAllSources();

	GetWorldTimerManager().SetTimer(
		BuildPhaseTimerHandle,
		FTimerDelegate::CreateUObject(this, &AWaveManager::StartWave),
		BuildPhaseDuration,
		/*bLoop=*/false
	);
}

void AWaveManager::StartWave()
{
	CurrentWaveIndex++;

	if (CurrentWaveIndex >= WaveConfigs.Num())
	{
		// All waves survived
		SetState(EWaveState::Victory);
		return;
	}

	SetState(EWaveState::WaveActive);
	OnWaveNumberChanged.Broadcast(CurrentWaveIndex + 1, WaveConfigs.Num());

	const FWaveConfig& Config = WaveConfigs[CurrentWaveIndex];
	bBasinTriggered = false;

	// Activate sources for this wave
	ActivateSources(Config.SourceCount, Config.SpawnRateMultiplier);

	// Wave duration timer
	GetWorldTimerManager().SetTimer(
		WaveTimerHandle,
		FTimerDelegate::CreateUObject(this, &AWaveManager::OnWaveTimerExpired),
		Config.Duration,
		/*bLoop=*/false
	);

	// Basin trigger check (1s interval)
	if (Config.bBasinTriggerEnabled)
	{
		GetWorldTimerManager().SetTimer(
			BasinCheckTimerHandle,
			FTimerDelegate::CreateUObject(this, &AWaveManager::CheckBasinTrigger),
			1.f,
			/*bLoop=*/true
		);
	}
}

void AWaveManager::OnWaveTimerExpired()
{
	EndWave();
}

void AWaveManager::EndWave()
{
	GetWorldTimerManager().ClearTimer(WaveTimerHandle);
	GetWorldTimerManager().ClearTimer(BasinCheckTimerHandle);

	if (CurrentWaveIndex + 1 >= WaveConfigs.Num())
	{
		// Last wave complete — victory
		DeactivateAllSources();
		SetState(EWaveState::Victory);
	}
	else
	{
		StartBuildPhase();
	}
}

void AWaveManager::CheckBasinTrigger()
{
	if (bBasinTriggered || !FluidSubsystem || !BasinSource) { return; }

	if (FluidSubsystem->GetTotalFluidVolume() > BasinTriggerVolume)
	{
		bBasinTriggered = true;
		BasinSource->Activate();
		GetWorldTimerManager().ClearTimer(BasinCheckTimerHandle);
	}
}

void AWaveManager::ActivateSources(int32 Count, float SpawnRateMultiplier)
{
	const int32 NumToActivate = FMath::Min(Count, FluidSources.Num());
	for (int32 I = 0; I < NumToActivate; ++I)
	{
		if (AFluidSource* Source = FluidSources[I])
		{
			Source->SpawnRate = Source->SpawnRate * SpawnRateMultiplier;
			Source->Activate();
		}
	}
}

void AWaveManager::DeactivateAllSources()
{
	for (AFluidSource* Source : FluidSources)
	{
		if (Source) { Source->Deactivate(); }
	}
	if (BasinSource) { BasinSource->Deactivate(); }
}

void AWaveManager::OnTownDestroyed()
{
	GetWorldTimerManager().ClearTimer(WaveTimerHandle);
	GetWorldTimerManager().ClearTimer(BuildPhaseTimerHandle);
	GetWorldTimerManager().ClearTimer(BasinCheckTimerHandle);
	DeactivateAllSources();
	SetState(EWaveState::Defeat);
}

float AWaveManager::GetWaveTimeRemaining() const
{
	if (CurrentState != EWaveState::WaveActive) { return 0.f; }
	return GetWorldTimerManager().GetTimerRemaining(WaveTimerHandle);
}

float AWaveManager::GetBuildPhaseTimeRemaining() const
{
	if (CurrentState != EWaveState::BuildPhase) { return 0.f; }
	return GetWorldTimerManager().GetTimerRemaining(BuildPhaseTimerHandle);
}
```

**Known issue with ActivateSources:** SpawnRate is being multiplied each wave cumulatively. We need to store the base rate. Fix: read SpawnRate from the CDO default, or store base rates at GatherSources time.

**Step 2b: Fix SpawnRate accumulation — store base rates**

Add to the private section of WaveManager.h:
```cpp
/** Base spawn rates cached at BeginPlay, before wave multipliers. */
TArray<float> BaseSpawnRates;
```

In GatherSources(), after adding each source to FluidSources:
```cpp
BaseSpawnRates.Add(Source->SpawnRate);
```

Replace ActivateSources with:
```cpp
void AWaveManager::ActivateSources(int32 Count, float SpawnRateMultiplier)
{
	const int32 NumToActivate = FMath::Min(Count, FluidSources.Num());
	for (int32 I = 0; I < NumToActivate; ++I)
	{
		if (AFluidSource* Source = FluidSources[I])
		{
			Source->SpawnRate = BaseSpawnRates[I] * SpawnRateMultiplier;
			Source->Activate();
		}
	}
}
```

**Step 3: Commit**

```bash
git add Source/GammaGoo/Public/Game/WaveManager.h Source/GammaGoo/Private/Game/WaveManager.cpp
git commit -m "feat: add AWaveManager with 5-wave sequence, build phases, basin trigger"
```

---

### Task 3: UFluidSiegeHUD — Gameplay HUD Widget

**Files:**
- Create: `Source/GammaGoo/Public/UI/FluidSiegeHUD.h`
- Create: `Source/GammaGoo/Private/UI/FluidSiegeHUD.cpp`

**Context:** A single UUserWidget subclass that displays all gameplay info: wave state, wave number, timer, resources, Town Hall HP bar, player energy bar. Uses C++ text/progress bar bindings. The widget is created by AFluidDefenseCharacter and added to viewport.

The HUD polls data from subsystems rather than delegate-binding every field, using a 0.1s NativeTick for simplicity at PoC scale. Key bindings:
- Wave number and state from AWaveManager (found via TActorIterator)
- Currency from UResourceSubsystem (GameInstance subsystem)
- Town Hall HP from ATownHall (found via TActorIterator)
- Player energy from owning AFluidDefenseCharacter

**Step 1: Create header**

```cpp
// Source/GammaGoo/Public/UI/FluidSiegeHUD.h
// Copyright 2026 Bret Wright. All Rights Reserved.
// UFluidSiegeHUD displays wave state, resources, Town Hall HP, and player energy.

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

UCLASS()
class GAMMAGOO_API UFluidSiegeHUD : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Call after creating the widget to cache references. */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void InitHUD(AFluidDefenseCharacter* InPlayer);

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// --- Widget bindings (set in Blueprint or via BindWidget) ---

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WaveText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TimerText;

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

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StateText;

private:
	UPROPERTY()
	TObjectPtr<AWaveManager> WaveManager = nullptr;

	UPROPERTY()
	TObjectPtr<UResourceSubsystem> ResourceSubsystem = nullptr;

	UPROPERTY()
	TObjectPtr<ATownHall> TownHall = nullptr;

	UPROPERTY()
	TObjectPtr<AFluidDefenseCharacter> PlayerCharacter = nullptr;

	void CacheReferences();
};
```

**Step 2: Create implementation**

```cpp
// Source/GammaGoo/Private/UI/FluidSiegeHUD.cpp
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
#include "Kismet/GameplayStatics.h"

void UFluidSiegeHUD::InitHUD(AFluidDefenseCharacter* InPlayer)
{
	PlayerCharacter = InPlayer;
	CacheReferences();
}

void UFluidSiegeHUD::CacheReferences()
{
	const UWorld* World = GetWorld();
	if (!World) { return; }

	// Find WaveManager
	for (TActorIterator<AWaveManager> It(World); It; ++It)
	{
		WaveManager = *It;
		break;
	}

	// Find TownHall
	for (TActorIterator<ATownHall> It(World); It; ++It)
	{
		TownHall = *It;
		break;
	}

	// Get ResourceSubsystem
	if (const UGameInstance* GI = World->GetGameInstance())
	{
		ResourceSubsystem = GI->GetSubsystem<UResourceSubsystem>();
	}
}

void UFluidSiegeHUD::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// --- Wave Info ---
	if (WaveManager)
	{
		if (WaveText)
		{
			WaveText->SetText(FText::FromString(
				FString::Printf(TEXT("Wave %d / %d"), WaveManager->GetCurrentWaveNumber(), WaveManager->GetTotalWaves())
			));
		}

		if (TimerText)
		{
			float TimeRemaining = 0.f;
			FString TimerLabel;
			switch (WaveManager->GetWaveState())
			{
			case EWaveState::BuildPhase:
				TimeRemaining = WaveManager->GetBuildPhaseTimeRemaining();
				TimerLabel = TEXT("BUILD");
				break;
			case EWaveState::WaveActive:
				TimeRemaining = WaveManager->GetWaveTimeRemaining();
				TimerLabel = TEXT("WAVE");
				break;
			case EWaveState::Victory:
				TimerLabel = TEXT("VICTORY");
				break;
			case EWaveState::Defeat:
				TimerLabel = TEXT("DEFEAT");
				break;
			default:
				TimerLabel = TEXT("READY");
				break;
			}

			if (TimeRemaining > 0.f)
			{
				TimerText->SetText(FText::FromString(
					FString::Printf(TEXT("%s  %d s"), *TimerLabel, FMath::CeilToInt(TimeRemaining))
				));
			}
			else
			{
				TimerText->SetText(FText::FromString(TimerLabel));
			}
		}

		if (StateText)
		{
			switch (WaveManager->GetWaveState())
			{
			case EWaveState::BuildPhase:
				StateText->SetText(FText::FromString(TEXT("Build Phase")));
				break;
			case EWaveState::WaveActive:
				StateText->SetText(FText::FromString(TEXT("Wave Active")));
				break;
			case EWaveState::Victory:
				StateText->SetText(FText::FromString(TEXT("VICTORY!")));
				break;
			case EWaveState::Defeat:
				StateText->SetText(FText::FromString(TEXT("DEFEATED")));
				break;
			default:
				StateText->SetText(FText::FromString(TEXT("Preparing...")));
				break;
			}
		}
	}

	// --- Resources ---
	if (ResourceSubsystem && ResourceText)
	{
		ResourceText->SetText(FText::FromString(
			FString::Printf(TEXT("%.0f"), ResourceSubsystem->GetCurrency())
		));
	}

	// --- Town Hall HP ---
	if (TownHall)
	{
		const float HPPercent = TownHall->GetHealthPercent();
		if (TownHallHealthBar)
		{
			TownHallHealthBar->SetPercent(HPPercent);
			// Red when critical (<25%)
			const FLinearColor BarColor = HPPercent < 0.25f ? FLinearColor::Red : FLinearColor::Green;
			TownHallHealthBar->SetFillColorAndOpacity(BarColor);
		}
		if (TownHallHealthText)
		{
			TownHallHealthText->SetText(FText::FromString(
				FString::Printf(TEXT("Town Hall  %d%%"), FMath::RoundToInt(HPPercent * 100.f))
			));
		}
	}

	// --- Player Energy ---
	if (PlayerCharacter)
	{
		const float EnergyPct = PlayerCharacter->GetEnergyPercent();
		if (EnergyBar)
		{
			EnergyBar->SetPercent(EnergyPct);
		}
		if (EnergyText)
		{
			EnergyText->SetText(FText::FromString(
				FString::Printf(TEXT("Energy  %d%%"), FMath::RoundToInt(EnergyPct * 100.f))
			));
		}
	}
}
```

**Step 3: Commit**

```bash
git add Source/GammaGoo/Public/UI/FluidSiegeHUD.h Source/GammaGoo/Private/UI/FluidSiegeHUD.cpp
git commit -m "feat: add UFluidSiegeHUD with wave, resource, health, and energy display"
```

---

### Task 4: Wire HUD into AFluidDefenseCharacter

**Files:**
- Modify: `Source/GammaGoo/Public/Player/FluidDefenseCharacter.h`
- Modify: `Source/GammaGoo/Private/Player/FluidDefenseCharacter.cpp`

**Context:** The player character creates the HUD widget on BeginPlay and adds it to viewport. The HUD widget class is set via EditDefaultsOnly (assigned in Blueprint).

**Step 1: Add HUD properties to header**

In `FluidDefenseCharacter.h`, add forward declaration:
```cpp
class UFluidSiegeHUD;
```

Add to the protected section, after the Build Component:
```cpp
// --- HUD ---
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|UI")
TSubclassOf<UFluidSiegeHUD> HUDWidgetClass;

UPROPERTY(BlueprintReadOnly, Category = "Player|UI")
TObjectPtr<UFluidSiegeHUD> HUDWidget;
```

**Step 2: Create and add widget in BeginPlay**

In `FluidDefenseCharacter.cpp`, add include:
```cpp
#include "UI/FluidSiegeHUD.h"
```

At the end of `BeginPlay()`, after the fluid check timer setup, add:
```cpp
// Create HUD widget
if (HUDWidgetClass && IsLocallyControlled())
{
	HUDWidget = CreateWidget<UFluidSiegeHUD>(Cast<APlayerController>(GetController()), HUDWidgetClass);
	if (HUDWidget)
	{
		HUDWidget->InitHUD(this);
		HUDWidget->AddToViewport();
	}
}
```

**Step 3: Commit**

```bash
git add Source/GammaGoo/Public/Player/FluidDefenseCharacter.h Source/GammaGoo/Private/Player/FluidDefenseCharacter.cpp
git commit -m "feat: wire UFluidSiegeHUD into player character BeginPlay"
```

---

### Task 5: Final Compile Verification + Commit

**Step 1: Verify Build.cs**

Check that `Source/GammaGoo/GammaGoo.Build.cs` already contains `"UMG"` in PrivateDependencyModuleNames. It does (confirmed in research). No changes needed.

**Step 2: Full compile**

Build the project and verify zero errors across all new and modified files.

**Step 3: Commit (if any remaining changes)**

```bash
git add -A
git commit -m "feat: Week 4 complete — wave manager, HUD, health delegate"
```

---

## Summary of All New/Modified Files

| File | Action | Purpose |
|------|--------|---------|
| `Public/Game/TownHall.h` | MODIFY | Add FOnTownHealthChanged delegate |
| `Private/Game/TownHall.cpp` | MODIFY | Broadcast OnHealthChanged in CheckFluidDamage |
| `Public/Game/WaveManager.h` | CREATE | 5-wave state machine, FWaveConfig, EWaveState |
| `Private/Game/WaveManager.cpp` | CREATE | Wave sequence, build phases, basin trigger, source management |
| `Public/UI/FluidSiegeHUD.h` | CREATE | UUserWidget with BindWidgetOptional text/progress bars |
| `Private/UI/FluidSiegeHUD.cpp` | CREATE | NativeTick polling of wave, resource, health, energy data |
| `Public/Player/FluidDefenseCharacter.h` | MODIFY | Add HUDWidgetClass and HUDWidget properties |
| `Private/Player/FluidDefenseCharacter.cpp` | MODIFY | Create and add HUD widget in BeginPlay |

## Post-Implementation: In-Editor Setup Required

After C++ compiles, these must be done manually in the UE editor:

1. **Create WBP_FluidSiegeHUD** — Widget Blueprint parented to UFluidSiegeHUD, layout text blocks and progress bars with matching names (WaveText, TimerText, ResourceText, etc.)
2. **Create BP_WaveManager** — Blueprint of AWaveManager, place in Lvl_MillbrookValley
3. **Tag fluid sources** — Add "FluidSource" tag to main sources, "BasinSource" tag to the basin trigger source
4. **Set HUDWidgetClass** — On BP_FluidDefenseCharacter, set HUDWidgetClass to WBP_FluidSiegeHUD
5. **Create Enhanced Input assets** — IA_Fire, IA_Build, IMC_FluidSiege (data assets in Content/Input/)
