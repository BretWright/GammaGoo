// Copyright 2026 Bret Wright. All Rights Reserved.

#include "Towers/LeveeWall.h"
#include "Fluid/FluidSubsystem.h"
#include "Fluid/FluidTypes.h"

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

	// Compute occupied cells along the wall's forward axis
	const FVector Location = GetActorLocation();
	const FVector Forward = GetActorForwardVector();
	const float CellSize = FluidConstants::DefaultCellWorldSize;

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

	// Pressure damage: max fluid height differential across all occupied cells vs neighbors
	float MaxPressure = 0.f;
	const TArray<FFluidCell>& Grid = FluidSubsystem->GetGrid();

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

			const FFluidCell& Neighbor = Grid[FluidSubsystem->GetCellIndex(NX, NY)];
			const float Pressure = Neighbor.GetSurfaceHeight() - MyTerrain;
			MaxPressure = FMath::Max(MaxPressure, Pressure);
		}
	}

	// Apply pressure as damage (scaled down — wall survives several minutes under moderate pressure)
	if (MaxPressure > 0.f)
	{
		ApplyDamage(MaxPressure * 0.1f);
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
