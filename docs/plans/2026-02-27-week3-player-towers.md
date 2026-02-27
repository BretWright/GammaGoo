# Week 3: Player + Towers Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Implement the player character (fluid interaction, heat lance), build system, all 5 tower types, resource subsystem, and town hall lose condition — completing the core gameplay loop.

**Architecture:** AFluidDefenseCharacter extends ACharacter with Mover 2.0 movement, timer-driven fluid depth queries, and an attached UBuildComponent for tower placement. All 5 towers inherit AFluidTowerBase (timer-driven effects, never Tick). UResourceSubsystem (UGameInstanceSubsystem) tracks currency. ATownHall takes damage from fluid depth.

**Tech Stack:** UE 5.7 C++, Enhanced Input, Mover 2.0, UFluidSubsystem API, GameplayTags

---

## Dependency Order

```
Task 1: UResourceSubsystem (no deps — towers and siphon need it)
Task 2: AFluidTowerBase (needs UFluidSubsystem — already exists)
Task 3: Five tower subclasses (need AFluidTowerBase + UResourceSubsystem)
Task 4: ATownHall (needs UFluidSubsystem)
Task 5: AFluidDefenseCharacter (needs UFluidSubsystem)
Task 6: UBuildComponent (needs AFluidDefenseCharacter + AFluidTowerBase + UResourceSubsystem)
```

Tasks 1–2 are independent. Tasks 3–4 depend on 1–2. Task 5 is independent. Task 6 depends on everything.

---

### Task 1: UResourceSubsystem — Currency Tracking

**Files:**
- Create: `Source/GammaGoo/Public/Game/ResourceSubsystem.h`
- Create: `Source/GammaGoo/Private/Game/ResourceSubsystem.cpp`

**Context:** UGameInstanceSubsystem that persists across level loads. Towers deduct currency on placement, siphon wells add currency on drain. Starting balance: 500.

**Step 1: Create header**

```cpp
// Source/GammaGoo/Public/Game/ResourceSubsystem.h
// Copyright 2026 Bret Wright. All Rights Reserved.
// UResourceSubsystem tracks player currency. All spending and earning goes through this class.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ResourceSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnResourceChanged, float, NewAmount);

UCLASS()
class GAMMAGOO_API UResourceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category = "Resource")
	void AddCurrency(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Resource")
	bool SpendCurrency(float Amount);

	UFUNCTION(BlueprintPure, Category = "Resource")
	float GetCurrency() const { return Currency; }

	UFUNCTION(BlueprintPure, Category = "Resource")
	bool CanAfford(float Amount) const { return Currency >= Amount; }

	UPROPERTY(BlueprintAssignable, Category = "Resource")
	FOnResourceChanged OnResourceChanged;

private:
	UPROPERTY()
	float Currency = 0.f;

	static constexpr float StartingCurrency = 500.f;
};
```

**Step 2: Create implementation**

```cpp
// Source/GammaGoo/Private/Game/ResourceSubsystem.cpp
// Copyright 2026 Bret Wright. All Rights Reserved.

#include "Game/ResourceSubsystem.h"

void UResourceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Currency = StartingCurrency;
}

void UResourceSubsystem::AddCurrency(float Amount)
{
	if (Amount <= 0.f) { return; }
	Currency += Amount;
	OnResourceChanged.Broadcast(Currency);
}

bool UResourceSubsystem::SpendCurrency(float Amount)
{
	if (Amount <= 0.f || Currency < Amount) { return false; }
	Currency -= Amount;
	OnResourceChanged.Broadcast(Currency);
	return true;
}
```

**Step 3: Compile**

Run: `UnrealEditor-Cmd.exe` build or use editor hot-reload
Expected: Clean compile

**Step 4: Commit**

```bash
git add Source/GammaGoo/Public/Game/ResourceSubsystem.h Source/GammaGoo/Private/Game/ResourceSubsystem.cpp
git commit -m "feat: add UResourceSubsystem for player currency tracking"
```

---

### Task 2: AFluidTowerBase — Abstract Tower Base Class

**Files:**
- Create: `Source/GammaGoo/Public/Towers/FluidTowerBase.h`
- Create: `Source/GammaGoo/Private/Towers/FluidTowerBase.cpp`

**Context:** Abstract base for all 5 tower types. Timer-driven effect loop (never Tick). Provides EffectRadius, EffectInterval, BuildCost, Health. Subclasses override `ExecuteEffect()`.

**Step 1: Create header**

```cpp
// Source/GammaGoo/Public/Towers/FluidTowerBase.h
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
```

**Step 2: Create implementation**

```cpp
// Source/GammaGoo/Private/Towers/FluidTowerBase.cpp
// Copyright 2026 Bret Wright. All Rights Reserved.

#include "Towers/FluidTowerBase.h"
#include "Fluid/FluidSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

AFluidTowerBase::AFluidTowerBase()
{
	PrimaryActorTick.bCanEverTick = false;

	TowerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TowerMesh"));
	RootComponent = TowerMesh;
}

void AFluidTowerBase::BeginPlay()
{
	Super::BeginPlay();

	FluidSubsystem = GetWorld()->GetSubsystem<UFluidSubsystem>();

	GetWorldTimerManager().SetTimer(
		EffectTimerHandle,
		FTimerDelegate::CreateUObject(this, &AFluidTowerBase::ExecuteEffect),
		EffectInterval,
		/*bLoop=*/true
	);
}

void AFluidTowerBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(EffectTimerHandle);
	Super::EndPlay(EndPlayReason);
}

void AFluidTowerBase::ExecuteEffect()
{
	// Override in subclasses
}

void AFluidTowerBase::ApplyDamage(float DamageAmount)
{
	if (DamageAmount <= 0.f || Health <= 0.f) { return; }
	Health = FMath::Max(0.f, Health - DamageAmount);

	if (Health <= 0.f)
	{
		OnDestroyed_Internal();
	}
}

void AFluidTowerBase::OnDestroyed_Internal()
{
	GetWorldTimerManager().ClearTimer(EffectTimerHandle);
	Destroy();
}
```

**Step 3: Compile and commit**

```bash
git add Source/GammaGoo/Public/Towers/FluidTowerBase.h Source/GammaGoo/Private/Towers/FluidTowerBase.cpp
git commit -m "feat: add AFluidTowerBase abstract tower with timer-driven effects"
```

---

### Task 3: Five Tower Subclasses

**Files:**
- Create: `Source/GammaGoo/Public/Towers/EvaporatorTower.h`
- Create: `Source/GammaGoo/Private/Towers/EvaporatorTower.cpp`
- Create: `Source/GammaGoo/Public/Towers/RepulsorTower.h`
- Create: `Source/GammaGoo/Private/Towers/RepulsorTower.cpp`
- Create: `Source/GammaGoo/Public/Towers/SiphonTower.h`
- Create: `Source/GammaGoo/Private/Towers/SiphonTower.cpp`
- Create: `Source/GammaGoo/Public/Towers/LeveeWall.h`
- Create: `Source/GammaGoo/Private/Towers/LeveeWall.cpp`
- Create: `Source/GammaGoo/Public/Towers/CryoSpike.h`
- Create: `Source/GammaGoo/Private/Towers/CryoSpike.cpp`

**Context:** Each tower overrides `ExecuteEffect()` with one UFluidSubsystem API call per the implementation plan. Build costs: Evaporator=100, Repulsor=150, Siphon=75, Levee=25, Cryo=200.

#### 3A: AEvaporatorTower

**Step 1: Create header + implementation**

```cpp
// Source/GammaGoo/Public/Towers/EvaporatorTower.h
#pragma once

#include "CoreMinimal.h"
#include "Towers/FluidTowerBase.h"
#include "EvaporatorTower.generated.h"

UCLASS(BlueprintType, Blueprintable)
class GAMMAGOO_API AEvaporatorTower : public AFluidTowerBase
{
	GENERATED_BODY()

public:
	AEvaporatorTower();
	virtual void ExecuteEffect() override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower|Evaporator")
	float EvaporateAmount = 50.f;
};
```

```cpp
// Source/GammaGoo/Private/Towers/EvaporatorTower.cpp
#include "Towers/EvaporatorTower.h"
#include "Fluid/FluidSubsystem.h"

AEvaporatorTower::AEvaporatorTower()
{
	EffectRadius = 500.f;
	EffectInterval = 0.5f;
	BuildCost = 100.f;
	Health = 100.f;
	MaxHealth = 100.f;
}

void AEvaporatorTower::ExecuteEffect()
{
	if (!FluidSubsystem || !IsAlive()) { return; }
	FluidSubsystem->RemoveFluidInRadius(GetActorLocation(), EffectRadius, EvaporateAmount);
}
```

#### 3B: ARepulsorTower

```cpp
// Source/GammaGoo/Public/Towers/RepulsorTower.h
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
```

```cpp
// Source/GammaGoo/Private/Towers/RepulsorTower.cpp
#include "Towers/RepulsorTower.h"
#include "Fluid/FluidSubsystem.h"

ARepulsorTower::ARepulsorTower()
{
	EffectRadius = 600.f;
	EffectInterval = 0.5f;
	BuildCost = 150.f;
	Health = 100.f;
	MaxHealth = 100.f;
}

void ARepulsorTower::ExecuteEffect()
{
	if (!FluidSubsystem || !IsAlive()) { return; }
	// Radial push: each cell gets pushed away from tower center.
	// ApplyForceInRadius handles the radial direction internally via per-cell offset.
	// We pass a dummy direction; the subsystem uses radial falloff.
	// Actually, the API takes a uniform force direction. For radial push,
	// we need to call per-cell. Instead, we iterate affected cells manually.
	//
	// Simpler approach: call ApplyForceInRadius 4 times with cardinal directions
	// to approximate radial push. This matches the grid's Von Neumann flow model.
	const FVector Loc = GetActorLocation();
	const float HalfStrength = PushStrength * 0.5f;
	FluidSubsystem->ApplyForceInRadius(Loc, EffectRadius, FVector2D(HalfStrength, 0.f));
	FluidSubsystem->ApplyForceInRadius(Loc, EffectRadius, FVector2D(-HalfStrength, 0.f));
	FluidSubsystem->ApplyForceInRadius(Loc, EffectRadius, FVector2D(0.f, HalfStrength));
	FluidSubsystem->ApplyForceInRadius(Loc, EffectRadius, FVector2D(0.f, -HalfStrength));
}
```

Wait — the implementation plan says `ApplyForceInRadius(Location, 600, RadialDirection * 200)`. The existing API takes a uniform FVector2D Force, but the Repulsor needs radial (outward from center). The current API applies the *same* force vector to all cells. We need to add a radial force API or modify the tower to compute per-cell direction.

**Better approach:** Add `ApplyRadialForceInRadius()` to UFluidSubsystem that computes per-cell outward direction. This is cleaner than the 4-call hack above.

**Step 1a: Add to FluidSubsystem.h** (modify existing file)

Add after `ApplyForceInRadius`:
```cpp
/** Applies radial outward force in a radius. Each cell gets pushed away from center. Used by repulsor towers. */
UFUNCTION(BlueprintCallable, Category = "Fluid")
void ApplyRadialForceInRadius(FVector Center, float Radius, float Strength);
```

**Step 1b: Add to FluidSubsystem.cpp**

```cpp
void UFluidSubsystem::ApplyRadialForceInRadius(FVector Center, float Radius, float Strength)
{
	const FIntPoint CenterCell = WorldToCell(Center);
	const int32 CellRadius = FMath::CeilToInt(Radius / CellWorldSize);

	for (int32 DY = -CellRadius; DY <= CellRadius; ++DY)
	{
		for (int32 DX = -CellRadius; DX <= CellRadius; ++DX)
		{
			const int32 X = CenterCell.X + DX;
			const int32 Y = CenterCell.Y + DY;
			if (!IsValidCell(X, Y)) { continue; }

			const FVector CellPos = CellToWorld(X, Y);
			const FVector2D Offset(CellPos.X - Center.X, CellPos.Y - Center.Y);
			const float Dist = Offset.Size();
			if (Dist > Radius || Dist < KINDA_SMALL_NUMBER) { continue; }

			const int32 Idx = GetCellIndex(X, Y);
			if (Grid[Idx].FluidVolume <= KINDA_SMALL_NUMBER) { continue; }

			const FVector2D Dir = Offset / Dist;
			const float Falloff = 1.f - (Dist / Radius);
			Grid[Idx].FlowVelocity += Dir * Strength * Falloff;
		}
	}
}
```

Then RepulsorTower simplifies to:
```cpp
void ARepulsorTower::ExecuteEffect()
{
	if (!FluidSubsystem || !IsAlive()) { return; }
	FluidSubsystem->ApplyRadialForceInRadius(GetActorLocation(), EffectRadius, PushStrength);
}
```

#### 3C: ASiphonTower

```cpp
// Source/GammaGoo/Public/Towers/SiphonTower.h
#pragma once

#include "CoreMinimal.h"
#include "Towers/FluidTowerBase.h"
#include "SiphonTower.generated.h"

UCLASS(BlueprintType, Blueprintable)
class GAMMAGOO_API ASiphonTower : public AFluidTowerBase
{
	GENERATED_BODY()

public:
	ASiphonTower();
	virtual void ExecuteEffect() override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower|Siphon")
	float DrainAmount = 30.f;

	/** Resource gained per 1.0 volume drained. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower|Siphon")
	float ConversionRatio = 0.5f;
};
```

```cpp
// Source/GammaGoo/Private/Towers/SiphonTower.cpp
#include "Towers/SiphonTower.h"
#include "Fluid/FluidSubsystem.h"
#include "Game/ResourceSubsystem.h"
#include "Engine/World.h"

ASiphonTower::ASiphonTower()
{
	EffectRadius = 400.f;
	EffectInterval = 0.5f;
	BuildCost = 75.f;
	Health = 100.f;
	MaxHealth = 100.f;
}

void ASiphonTower::ExecuteEffect()
{
	if (!FluidSubsystem || !IsAlive()) { return; }

	// Measure fluid before and after to compute actual removal
	const float VolumeBefore = FluidSubsystem->GetTotalFluidVolume();
	FluidSubsystem->RemoveFluidInRadius(GetActorLocation(), EffectRadius, DrainAmount);
	const float VolumeAfter = FluidSubsystem->GetTotalFluidVolume();

	const float Removed = VolumeBefore - VolumeAfter;
	if (Removed > 0.f)
	{
		if (UResourceSubsystem* Res = GetGameInstance()->GetSubsystem<UResourceSubsystem>())
		{
			Res->AddCurrency(Removed * ConversionRatio);
		}
	}
}
```

**Note:** Using GetTotalFluidVolume() diff is a full-grid sum — fine for 16K cells at PoC scale. If this becomes a bottleneck, we'll add a `RemoveFluidInRadius` overload that returns the amount removed.

#### 3D: ALeveeWall

```cpp
// Source/GammaGoo/Public/Towers/LeveeWall.h
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
```

```cpp
// Source/GammaGoo/Private/Towers/LeveeWall.cpp
#include "Towers/LeveeWall.h"
#include "Fluid/FluidSubsystem.h"

ALeveeWall::ALeveeWall()
{
	EffectRadius = 0.f; // Not radius-based
	EffectInterval = 1.0f; // Pressure damage check interval
	BuildCost = 25.f;
	Health = 500.f;
	MaxHealth = 500.f;
}

void ALeveeWall::BeginPlay()
{
	Super::BeginPlay();

	if (!FluidSubsystem) { return; }

	// Compute occupied cells along the wall's forward (Y) axis
	const FVector Location = GetActorLocation();
	const FVector Forward = GetActorForwardVector();
	const float CellSize = 100.f; // FluidConstants::DefaultCellWorldSize

	for (int32 I = 0; I < WallLength; ++I)
	{
		const FVector CellWorldPos = Location + Forward * (I - WallLength / 2) * CellSize;
		const FIntPoint Cell = FluidSubsystem->WorldToCell(CellWorldPos);
		if (FluidSubsystem->IsValidCell(Cell.X, Cell.Y))
		{
			OccupiedCells.Add(Cell);
		}
	}

	BlockCells(true);
}

void ALeveeWall::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	BlockCells(false);
	Super::EndPlay(EndPlayReason);
}

void ALeveeWall::ExecuteEffect()
{
	if (!FluidSubsystem || !IsAlive()) { return; }

	// Pressure damage: check fluid height differential across the wall
	// Damage = max height differential across all occupied cells vs their neighbors
	float MaxPressure = 0.f;
	const auto& Grid = FluidSubsystem->GetGrid();

	for (const FIntPoint& Cell : OccupiedCells)
	{
		if (!FluidSubsystem->IsValidCell(Cell.X, Cell.Y)) { continue; }
		const float MyTerrain = Grid[FluidSubsystem->GetCellIndex(Cell.X, Cell.Y)].TerrainHeight;

		// Check 4 neighbors for fluid pressing against the wall
		static const int32 DX[4] = { 1, -1, 0, 0 };
		static const int32 DY[4] = { 0, 0, 1, -1 };

		for (int32 Dir = 0; Dir < 4; ++Dir)
		{
			const int32 NX = Cell.X + DX[Dir];
			const int32 NY = Cell.Y + DY[Dir];
			if (!FluidSubsystem->IsValidCell(NX, NY)) { continue; }

			const auto& Neighbor = Grid[FluidSubsystem->GetCellIndex(NX, NY)];
			const float Pressure = Neighbor.GetSurfaceHeight() - MyTerrain;
			MaxPressure = FMath::Max(MaxPressure, Pressure);
		}
	}

	// Apply pressure as damage (scaled down — wall should survive several minutes under moderate pressure)
	if (MaxPressure > 0.f)
	{
		ApplyDamage(MaxPressure * 0.1f); // 10% of pressure differential per check
	}
}

void ALeveeWall::BlockCells(bool bBlock)
{
	if (!FluidSubsystem) { return; }
	for (const FIntPoint& Cell : OccupiedCells)
	{
		FluidSubsystem->SetBlockedAtCell(Cell.X, Cell.Y, bBlock);
	}
}
```

#### 3E: ACryoSpike

```cpp
// Source/GammaGoo/Public/Towers/CryoSpike.h
#pragma once

#include "CoreMinimal.h"
#include "Towers/FluidTowerBase.h"
#include "CryoSpike.generated.h"

UCLASS(BlueprintType, Blueprintable)
class GAMMAGOO_API ACryoSpike : public AFluidTowerBase
{
	GENERATED_BODY()

public:
	ACryoSpike();
	virtual void ExecuteEffect() override;

protected:
	/** How long the freeze persists before thawing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower|Cryo")
	float FreezeDuration = 30.f;

	/** Cooldown after thaw before re-freezing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower|Cryo")
	float CooldownDuration = 5.f;

private:
	bool bFreezeActive = false;
	FTimerHandle ThawTimerHandle;
	FTimerHandle CooldownTimerHandle;
	bool bOnCooldown = false;

	void Freeze();
	void Thaw();
	void CooldownComplete();
};
```

```cpp
// Source/GammaGoo/Private/Towers/CryoSpike.cpp
#include "Towers/CryoSpike.h"
#include "Fluid/FluidSubsystem.h"
#include "TimerManager.h"

ACryoSpike::ACryoSpike()
{
	EffectRadius = 450.f;
	EffectInterval = 1.0f; // Check interval (not used for primary effect)
	BuildCost = 200.f;
	Health = 100.f;
	MaxHealth = 100.f;
}

void ACryoSpike::ExecuteEffect()
{
	// Auto-cycle: freeze if not active and not on cooldown
	if (!FluidSubsystem || !IsAlive()) { return; }
	if (!bFreezeActive && !bOnCooldown)
	{
		Freeze();
	}
}

void ACryoSpike::Freeze()
{
	if (!FluidSubsystem) { return; }
	bFreezeActive = true;
	FluidSubsystem->SetFrozenInRadius(GetActorLocation(), EffectRadius, true);

	GetWorldTimerManager().SetTimer(
		ThawTimerHandle,
		FTimerDelegate::CreateUObject(this, &ACryoSpike::Thaw),
		FreezeDuration,
		/*bLoop=*/false
	);
}

void ACryoSpike::Thaw()
{
	if (!FluidSubsystem) { return; }
	bFreezeActive = false;
	FluidSubsystem->SetFrozenInRadius(GetActorLocation(), EffectRadius, false);

	bOnCooldown = true;
	GetWorldTimerManager().SetTimer(
		CooldownTimerHandle,
		FTimerDelegate::CreateUObject(this, &ACryoSpike::CooldownComplete),
		CooldownDuration,
		/*bLoop=*/false
	);
}

void ACryoSpike::CooldownComplete()
{
	bOnCooldown = false;
}
```

**Step 2: Compile all tower subclasses**

**Step 3: Commit**

```bash
git add Source/GammaGoo/Public/Towers/ Source/GammaGoo/Private/Towers/
git add Source/GammaGoo/Public/Fluid/FluidSubsystem.h Source/GammaGoo/Private/Fluid/FluidSubsystem.cpp
git commit -m "feat: add all 5 tower types (Evaporator, Repulsor, Siphon, Levee, Cryo)"
```

---

### Task 4: ATownHall — Lose Condition

**Files:**
- Create: `Source/GammaGoo/Public/Game/TownHall.h`
- Create: `Source/GammaGoo/Private/Game/TownHall.cpp`

**Context:** A placed actor that represents the player's settlement. Takes damage from fluid depth at its location on a timer. When health reaches 0, broadcasts OnTownDestroyed for game-over logic.

**Step 1: Create header**

```cpp
// Source/GammaGoo/Public/Game/TownHall.h
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
```

**Step 2: Create implementation**

```cpp
// Source/GammaGoo/Private/Game/TownHall.cpp
#include "Game/TownHall.h"
#include "Fluid/FluidSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

ATownHall::ATownHall()
{
	PrimaryActorTick.bCanEverTick = false;

	BuildingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BuildingMesh"));
	RootComponent = BuildingMesh;
}

void ATownHall::BeginPlay()
{
	Super::BeginPlay();

	FluidSubsystem = GetWorld()->GetSubsystem<UFluidSubsystem>();
	BaseZ = GetActorLocation().Z;

	GetWorldTimerManager().SetTimer(
		DamageTimerHandle,
		FTimerDelegate::CreateUObject(this, &ATownHall::CheckFluidDamage),
		DamageCheckInterval,
		/*bLoop=*/true
	);
}

void ATownHall::CheckFluidDamage()
{
	if (bDestroyed || !FluidSubsystem) { return; }

	const float FluidHeight = FluidSubsystem->GetFluidHeightAtWorldPos(GetActorLocation());
	const float Depth = FluidHeight - BaseZ;

	if (Depth <= 0.f) { return; }

	const float Damage = Depth * DamagePerDepthPerSecond * DamageCheckInterval;
	Health = FMath::Max(0.f, Health - Damage);

	if (Health <= 0.f)
	{
		bDestroyed = true;
		GetWorldTimerManager().ClearTimer(DamageTimerHandle);
		OnTownDestroyed.Broadcast();
	}
}
```

**Step 3: Commit**

```bash
git add Source/GammaGoo/Public/Game/TownHall.h Source/GammaGoo/Private/Game/TownHall.cpp
git commit -m "feat: add ATownHall with fluid damage and lose condition"
```

---

### Task 5: AFluidDefenseCharacter — Player Character

**Files:**
- Create: `Source/GammaGoo/Public/Player/FluidDefenseCharacter.h`
- Create: `Source/GammaGoo/Private/Player/FluidDefenseCharacter.cpp`

**Context:** Third-person character with Mover 2.0. Queries fluid depth on 0.1s timer. 5 depth tiers affect movement speed and apply damage. Heat lance: line trace from camera, RemoveFluidInRadius at hit point, energy-limited (100 max, 10/s drain, 5/s recharge).

Enhanced Input: Reuse existing IA_Move, IA_Look, IA_Jump. Create new IA_Fire (for heat lance) and IA_Build (toggle build mode) as data assets in-editor later. The C++ binds to input action names.

**Step 1: Create header**

```cpp
// Source/GammaGoo/Public/Player/FluidDefenseCharacter.h
// Copyright 2026 Bret Wright. All Rights Reserved.
// Player character with fluid depth interaction, heat lance, and build component.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FluidDefenseCharacter.generated.h"

class UFluidSubsystem;
class UBuildComponent;
class UInputAction;
class UInputMappingContext;

UENUM(BlueprintType)
enum class EFluidDepthTier : uint8
{
	Dry,        // No fluid
	Ankle,      // 0-30cm: 80% speed, no damage
	Knee,       // 30-70cm: 50% speed, 5 HP/s
	Waist,      // 70-120cm: 30% speed, 15 HP/s
	Chest       // 120cm+: 15% speed, 30 HP/s
};

UCLASS(BlueprintType, Blueprintable)
class GAMMAGOO_API AFluidDefenseCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AFluidDefenseCharacter();

	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintPure, Category = "Player|Fluid")
	EFluidDepthTier GetCurrentDepthTier() const { return CurrentDepthTier; }

	UFUNCTION(BlueprintPure, Category = "Player|Fluid")
	float GetCurrentFluidDepth() const { return CurrentFluidDepth; }

	UFUNCTION(BlueprintPure, Category = "Player|HeatLance")
	float GetEnergyPercent() const { return MaxEnergy > 0.f ? Energy / MaxEnergy : 0.f; }

protected:
	// --- Build Component ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Build")
	TObjectPtr<UBuildComponent> BuildComponent;

	// --- Enhanced Input ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Input")
	TObjectPtr<UInputAction> FireAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Input")
	TObjectPtr<UInputAction> BuildToggleAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Input")
	TObjectPtr<UInputMappingContext> FluidSiegeMappingContext;

	// --- Fluid Interaction ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Fluid")
	float FluidCheckInterval = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Health")
	float PlayerHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Health")
	float MaxPlayerHealth = 100.f;

	// --- Heat Lance ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|HeatLance")
	float LanceRange = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|HeatLance")
	float LanceRadius = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|HeatLance")
	float LanceEvaporateRate = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|HeatLance")
	float Energy = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|HeatLance")
	float MaxEnergy = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|HeatLance")
	float EnergyDrainRate = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|HeatLance")
	float EnergyRechargeRate = 5.f;

private:
	// --- Fluid depth ---
	void CheckFluidDepth();
	EFluidDepthTier ComputeDepthTier(float Depth) const;
	void ApplyDepthEffects(float DeltaTime);

	FTimerHandle FluidCheckTimerHandle;
	EFluidDepthTier CurrentDepthTier = EFluidDepthTier::Dry;
	float CurrentFluidDepth = 0.f;
	float BaseSpeedMultiplier = 1.f;

	// --- Heat lance ---
	void StartFiring();
	void StopFiring();
	void FireLanceTick();

	bool bFiring = false;
	FTimerHandle LanceTickHandle;

	// --- Input ---
	void OnBuildToggle();

	UPROPERTY()
	TObjectPtr<UFluidSubsystem> FluidSubsystem = nullptr;

	/** Cached default walk speed from CharacterMovement. */
	float DefaultWalkSpeed = 600.f;
};
```

**Step 2: Create implementation**

```cpp
// Source/GammaGoo/Private/Player/FluidDefenseCharacter.cpp
#include "Player/FluidDefenseCharacter.h"
#include "Fluid/FluidSubsystem.h"
#include "Player/BuildComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Camera/CameraComponent.h"

AFluidDefenseCharacter::AFluidDefenseCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	BuildComponent = CreateDefaultSubobject<UBuildComponent>(TEXT("BuildComponent"));
}

void AFluidDefenseCharacter::BeginPlay()
{
	Super::BeginPlay();

	FluidSubsystem = GetWorld()->GetSubsystem<UFluidSubsystem>();
	DefaultWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;

	// Add mapping context
	if (FluidSiegeMappingContext)
	{
		if (const APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			if (UEnhancedInputLocalPlayerSubsystem* InputSub =
				ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
			{
				InputSub->AddMappingContext(FluidSiegeMappingContext, 1);
			}
		}
	}

	// Fluid depth check timer
	GetWorldTimerManager().SetTimer(
		FluidCheckTimerHandle,
		FTimerDelegate::CreateUObject(this, &AFluidDefenseCharacter::CheckFluidDepth),
		FluidCheckInterval,
		/*bLoop=*/true
	);
}

void AFluidDefenseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (FireAction)
		{
			EIC->BindAction(FireAction, ETriggerEvent::Started, this, &AFluidDefenseCharacter::StartFiring);
			EIC->BindAction(FireAction, ETriggerEvent::Completed, this, &AFluidDefenseCharacter::StopFiring);
		}
		if (BuildToggleAction)
		{
			EIC->BindAction(BuildToggleAction, ETriggerEvent::Started, this, &AFluidDefenseCharacter::OnBuildToggle);
		}
	}
}

// ---------------------------------------------------------------------------
// Fluid Depth Interaction
// ---------------------------------------------------------------------------

void AFluidDefenseCharacter::CheckFluidDepth()
{
	if (!FluidSubsystem) { return; }

	const FVector Loc = GetActorLocation();
	const float FluidSurface = FluidSubsystem->GetFluidHeightAtWorldPos(Loc);
	const float FeetZ = Loc.Z; // Character origin at feet

	CurrentFluidDepth = FMath::Max(0.f, FluidSurface - FeetZ);
	CurrentDepthTier = ComputeDepthTier(CurrentFluidDepth);

	// Apply speed modifier
	float SpeedMult = 1.f;
	switch (CurrentDepthTier)
	{
	case EFluidDepthTier::Dry:    SpeedMult = 1.f;   break;
	case EFluidDepthTier::Ankle:  SpeedMult = 0.8f;  break;
	case EFluidDepthTier::Knee:   SpeedMult = 0.5f;  break;
	case EFluidDepthTier::Waist:  SpeedMult = 0.3f;  break;
	case EFluidDepthTier::Chest:  SpeedMult = 0.15f; break;
	}
	GetCharacterMovement()->MaxWalkSpeed = DefaultWalkSpeed * SpeedMult;

	// Apply damage over time
	ApplyDepthEffects(FluidCheckInterval);
}

EFluidDepthTier AFluidDefenseCharacter::ComputeDepthTier(float Depth) const
{
	if (Depth <= 0.f)   return EFluidDepthTier::Dry;
	if (Depth <= 30.f)  return EFluidDepthTier::Ankle;
	if (Depth <= 70.f)  return EFluidDepthTier::Knee;
	if (Depth <= 120.f) return EFluidDepthTier::Waist;
	return EFluidDepthTier::Chest;
}

void AFluidDefenseCharacter::ApplyDepthEffects(float DeltaTime)
{
	float DPS = 0.f;
	switch (CurrentDepthTier)
	{
	case EFluidDepthTier::Dry:
	case EFluidDepthTier::Ankle:
		DPS = 0.f;
		break;
	case EFluidDepthTier::Knee:
		DPS = 5.f;
		break;
	case EFluidDepthTier::Waist:
		DPS = 15.f;
		break;
	case EFluidDepthTier::Chest:
		DPS = 30.f;
		break;
	}

	if (DPS > 0.f)
	{
		PlayerHealth = FMath::Max(0.f, PlayerHealth - DPS * DeltaTime);
	}
}

// ---------------------------------------------------------------------------
// Heat Lance
// ---------------------------------------------------------------------------

void AFluidDefenseCharacter::StartFiring()
{
	if (bFiring) { return; }
	bFiring = true;

	// Fire at 30Hz (every sim step) for smooth evaporation
	GetWorldTimerManager().SetTimer(
		LanceTickHandle,
		FTimerDelegate::CreateUObject(this, &AFluidDefenseCharacter::FireLanceTick),
		1.f / 30.f,
		/*bLoop=*/true,
		/*InFirstDelay=*/0.f
	);
}

void AFluidDefenseCharacter::StopFiring()
{
	bFiring = false;
	GetWorldTimerManager().ClearTimer(LanceTickHandle);
}

void AFluidDefenseCharacter::FireLanceTick()
{
	if (!FluidSubsystem || Energy <= 0.f)
	{
		StopFiring();
		return;
	}

	const float DeltaTime = 1.f / 30.f;

	// Drain energy
	Energy = FMath::Max(0.f, Energy - EnergyDrainRate * DeltaTime);

	// Line trace from camera
	const APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) { return; }

	FVector CamLoc;
	FRotator CamRot;
	PC->GetPlayerViewPoint(CamLoc, CamRot);

	const FVector TraceEnd = CamLoc + CamRot.Vector() * LanceRange;
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(HeatLance), false, this);

	if (GetWorld()->LineTraceSingleByChannel(Hit, CamLoc, TraceEnd, ECC_WorldStatic, Params))
	{
		FluidSubsystem->RemoveFluidInRadius(Hit.ImpactPoint, LanceRadius, LanceEvaporateRate * DeltaTime);
	}
}

// ---------------------------------------------------------------------------
// Energy Recharge (called from CheckFluidDepth since it runs on a timer already)
// ---------------------------------------------------------------------------
// Note: Energy recharge is handled in CheckFluidDepth to avoid adding another timer.
// We piggyback on the 0.1s fluid check.

// Actually, let's add recharge logic to CheckFluidDepth:
// (This is cleaner than the comment above — the actual implementation follows)

// ---------------------------------------------------------------------------
// Build Mode Toggle
// ---------------------------------------------------------------------------

void AFluidDefenseCharacter::OnBuildToggle()
{
	if (BuildComponent)
	{
		BuildComponent->ToggleBuildMode();
	}
}
```

**Important note:** Energy recharge when not firing needs to happen somewhere. Add to `CheckFluidDepth()`:

After the depth effects block, add:
```cpp
	// Recharge energy when not firing
	if (!bFiring && Energy < MaxEnergy)
	{
		Energy = FMath::Min(MaxEnergy, Energy + EnergyRechargeRate * FluidCheckInterval);
	}
```

**Step 3: Compile and commit**

```bash
git add Source/GammaGoo/Public/Player/FluidDefenseCharacter.h Source/GammaGoo/Private/Player/FluidDefenseCharacter.cpp
git commit -m "feat: add AFluidDefenseCharacter with fluid depth tiers and heat lance"
```

---

### Task 6: UBuildComponent — Ghost Placement System

**Files:**
- Create: `Source/GammaGoo/Public/Player/BuildComponent.h`
- Create: `Source/GammaGoo/Private/Player/BuildComponent.cpp`

**Context:** Attached to AFluidDefenseCharacter. When build mode active: projects ghost mesh from camera onto terrain, snaps to grid, validates placement (dry, flat enough, affordable, not blocked). Player confirms to spawn tower, deduct resources.

**Step 1: Create header**

```cpp
// Source/GammaGoo/Public/Player/BuildComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuildComponent.generated.h"

class AFluidTowerBase;
class UFluidSubsystem;
class UResourceSubsystem;
class UStaticMeshComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GAMMAGOO_API UBuildComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBuildComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Build")
	void ToggleBuildMode();

	UFUNCTION(BlueprintCallable, Category = "Build")
	void SelectTowerType(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Build")
	void ConfirmPlacement();

	UFUNCTION(BlueprintCallable, Category = "Build")
	void CancelBuild();

	UFUNCTION(BlueprintPure, Category = "Build")
	bool IsInBuildMode() const { return bBuildModeActive; }

protected:
	/** Tower classes the player can build. Order matches UI radial menu. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Build")
	TArray<TSubclassOf<AFluidTowerBase>> TowerClasses;

	/** Max terrain slope for placement (dot product of surface normal vs up). 0.87 ≈ 30 degrees. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build")
	float MaxSlopeDot = 0.87f;

	/** Max fluid volume at placement cell for it to count as "dry." */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build")
	float MaxFluidForPlacement = 5.f;

	/** Minimum distance between tower centers in grid cells. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build")
	int32 MinTowerSpacing = 2;

private:
	bool bBuildModeActive = false;
	int32 SelectedTowerIndex = 0;
	bool bPlacementValid = false;
	FVector PlacementLocation = FVector::ZeroVector;
	FRotator PlacementRotation = FRotator::ZeroRotator;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> GhostMesh;

	UPROPERTY()
	TObjectPtr<UFluidSubsystem> FluidSubsystem = nullptr;

	UPROPERTY()
	TObjectPtr<UResourceSubsystem> ResourceSubsystem = nullptr;

	void UpdateGhostPosition();
	bool ValidatePlacement(const FVector& Location, const FHitResult& Hit) const;
	void SetGhostColor(bool bValid);
};
```

**Step 2: Create implementation**

```cpp
// Source/GammaGoo/Private/Player/BuildComponent.cpp
#include "Player/BuildComponent.h"
#include "Towers/FluidTowerBase.h"
#include "Fluid/FluidSubsystem.h"
#include "Fluid/FluidTypes.h"
#include "Game/ResourceSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

UBuildComponent::UBuildComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false; // Only tick in build mode
}

void UBuildComponent::BeginPlay()
{
	Super::BeginPlay();

	FluidSubsystem = GetWorld()->GetSubsystem<UFluidSubsystem>();
	if (const UGameInstance* GI = GetWorld()->GetGameInstance())
	{
		ResourceSubsystem = GI->GetSubsystem<UResourceSubsystem>();
	}

	// Create ghost mesh component (invisible until build mode)
	GhostMesh = NewObject<UStaticMeshComponent>(GetOwner(), TEXT("BuildGhost"));
	GhostMesh->SetupAttachment(GetOwner()->GetRootComponent());
	GhostMesh->RegisterComponent();
	GhostMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GhostMesh->SetCastShadow(false);
	GhostMesh->SetVisibility(false);
}

void UBuildComponent::ToggleBuildMode()
{
	bBuildModeActive = !bBuildModeActive;
	GhostMesh->SetVisibility(bBuildModeActive);
	SetComponentTickEnabled(bBuildModeActive);

	if (!bBuildModeActive)
	{
		GhostMesh->SetVisibility(false);
	}
}

void UBuildComponent::SelectTowerType(int32 Index)
{
	if (Index >= 0 && Index < TowerClasses.Num())
	{
		SelectedTowerIndex = Index;
		// Update ghost mesh to match selected tower's default mesh
		if (const AFluidTowerBase* CDO = TowerClasses[SelectedTowerIndex].GetDefaultObject())
		{
			// Ghost mesh will use the tower's mesh — set from Blueprint defaults
		}
	}
}

void UBuildComponent::ConfirmPlacement()
{
	if (!bBuildModeActive || !bPlacementValid) { return; }
	if (!TowerClasses.IsValidIndex(SelectedTowerIndex)) { return; }

	const TSubclassOf<AFluidTowerBase>& TowerClass = TowerClasses[SelectedTowerIndex];
	const AFluidTowerBase* CDO = TowerClass.GetDefaultObject();
	if (!CDO) { return; }

	// Check affordability
	if (!ResourceSubsystem || !ResourceSubsystem->SpendCurrency(CDO->GetBuildCost()))
	{
		return; // Can't afford
	}

	// Spawn the tower
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	GetWorld()->SpawnActor<AFluidTowerBase>(
		TowerClass,
		PlacementLocation,
		PlacementRotation,
		SpawnParams
	);
}

void UBuildComponent::CancelBuild()
{
	if (bBuildModeActive)
	{
		ToggleBuildMode();
	}
}

void UBuildComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bBuildModeActive)
	{
		UpdateGhostPosition();
	}
}

void UBuildComponent::UpdateGhostPosition()
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn) { return; }

	const APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController());
	if (!PC) { return; }

	// Line trace from camera center
	FVector CamLoc;
	FRotator CamRot;
	PC->GetPlayerViewPoint(CamLoc, CamRot);

	const FVector TraceEnd = CamLoc + CamRot.Vector() * 3000.f;
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(BuildTrace), false, GetOwner());

	if (!GetWorld()->LineTraceSingleByChannel(Hit, CamLoc, TraceEnd, ECC_WorldStatic, Params))
	{
		bPlacementValid = false;
		SetGhostColor(false);
		return;
	}

	// Snap to grid
	if (!FluidSubsystem) { return; }
	const FIntPoint Cell = FluidSubsystem->WorldToCell(Hit.ImpactPoint);
	if (!FluidSubsystem->IsValidCell(Cell.X, Cell.Y))
	{
		bPlacementValid = false;
		SetGhostColor(false);
		return;
	}

	const FVector SnappedPos = FluidSubsystem->CellToWorld(Cell.X, Cell.Y);
	PlacementLocation = FVector(SnappedPos.X, SnappedPos.Y, Hit.ImpactPoint.Z);
	PlacementRotation = FRotator::ZeroRotator;

	GhostMesh->SetWorldLocation(PlacementLocation);

	// Validate
	bPlacementValid = ValidatePlacement(PlacementLocation, Hit);
	SetGhostColor(bPlacementValid);
}

bool UBuildComponent::ValidatePlacement(const FVector& Location, const FHitResult& Hit) const
{
	if (!FluidSubsystem || !ResourceSubsystem) { return false; }
	if (!TowerClasses.IsValidIndex(SelectedTowerIndex)) { return false; }

	const AFluidTowerBase* CDO = TowerClasses[SelectedTowerIndex].GetDefaultObject();
	if (!CDO) { return false; }

	// Check affordability
	if (!ResourceSubsystem->CanAfford(CDO->GetBuildCost())) { return false; }

	// Check slope
	if (FVector::DotProduct(Hit.ImpactNormal, FVector::UpVector) < MaxSlopeDot) { return false; }

	// Check fluid depth at cell
	const FIntPoint Cell = FluidSubsystem->WorldToCell(Location);
	if (!FluidSubsystem->IsValidCell(Cell.X, Cell.Y)) { return false; }

	const auto& Grid = FluidSubsystem->GetGrid();
	const int32 Idx = FluidSubsystem->GetCellIndex(Cell.X, Cell.Y);
	if (Grid[Idx].FluidVolume > MaxFluidForPlacement) { return false; }
	if (Grid[Idx].bBlocked) { return false; }

	return true;
}

void UBuildComponent::SetGhostColor(bool bValid)
{
	// Set material color — green for valid, red for invalid
	// Ghost material setup happens in Blueprint; we just toggle a parameter
	if (UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(GhostMesh->GetMaterial(0)))
	{
		const FLinearColor Color = bValid ? FLinearColor::Green : FLinearColor::Red;
		MID->SetVectorParameterValue(TEXT("GhostColor"), Color);
	}
}
```

**Step 3: Compile and commit**

```bash
git add Source/GammaGoo/Public/Player/BuildComponent.h Source/GammaGoo/Private/Player/BuildComponent.cpp
git commit -m "feat: add UBuildComponent with ghost placement and grid snapping"
```

---

### Task 7: Build.cs Update + Final Compile Verification

**Files:**
- Modify: `Source/GammaGoo/GammaGoo.Build.cs` (if needed — check if GameplayTags is already listed)

**Step 1: Verify Build.cs has all required modules**

The existing Build.cs already includes: Core, CoreUObject, Engine, InputCore, EnhancedInput, RenderCore, RHI, Slate, SlateCore, UMG, Niagara, GameplayTags. No changes needed.

**Step 2: Full compile**

Build the project and verify zero errors across all new files.

**Step 3: Final commit**

```bash
git add -A
git commit -m "feat: Week 3 complete — player character, 5 tower types, build system, resource tracking, town hall"
```

---

## Summary of All New Files

| File | Purpose |
|------|---------|
| `Public/Game/ResourceSubsystem.h` | Currency tracking (UGameInstanceSubsystem) |
| `Private/Game/ResourceSubsystem.cpp` | AddCurrency, SpendCurrency, starting 500 |
| `Public/Towers/FluidTowerBase.h` | Abstract tower base: radius, interval, cost, health |
| `Private/Towers/FluidTowerBase.cpp` | Timer-driven ExecuteEffect loop |
| `Public/Towers/EvaporatorTower.h` | DESTROY: RemoveFluidInRadius |
| `Private/Towers/EvaporatorTower.cpp` | 500cm radius, 50 vol/tick |
| `Public/Towers/RepulsorTower.h` | DISPLACE: ApplyRadialForceInRadius |
| `Private/Towers/RepulsorTower.cpp` | 600cm radius, 200 strength |
| `Public/Towers/SiphonTower.h` | EXPLOIT: drain + currency |
| `Private/Towers/SiphonTower.cpp` | 400cm radius, 0.5 conversion |
| `Public/Towers/LeveeWall.h` | CONTAIN: SetBlockedAtCell, pressure damage |
| `Private/Towers/LeveeWall.cpp` | 1x3 footprint, 500 HP |
| `Public/Towers/CryoSpike.h` | CONTAIN: freeze/thaw cycle |
| `Private/Towers/CryoSpike.cpp` | 450cm radius, 30s freeze, 5s cooldown |
| `Public/Game/TownHall.h` | Lose condition: fluid depth damage |
| `Private/Game/TownHall.cpp` | Broadcasts OnTownDestroyed at 0 HP |
| `Public/Player/FluidDefenseCharacter.h` | Player: depth tiers, heat lance, input |
| `Private/Player/FluidDefenseCharacter.cpp` | 5 tiers, energy system, camera trace |
| `Public/Player/BuildComponent.h` | Ghost placement, validation, grid snap |
| `Private/Player/BuildComponent.cpp` | Camera trace, slope/fluid/cost checks |

**Modification to existing file:**
| `Public/Fluid/FluidSubsystem.h` | Add `ApplyRadialForceInRadius()` declaration |
| `Private/Fluid/FluidSubsystem.cpp` | Add `ApplyRadialForceInRadius()` implementation |
