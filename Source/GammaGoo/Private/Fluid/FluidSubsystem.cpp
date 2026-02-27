// Copyright 2026 Bret Wright. All Rights Reserved.

#include "Fluid/FluidSubsystem.h"
#include "DrawDebugHelpers.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "HAL/IConsoleManager.h"

void UFluidSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Center the grid on world origin
	const float HalfExtent = (FluidConstants::GridSize * CellWorldSize) * 0.5f;
	GridWorldOrigin = FVector(-HalfExtent, -HalfExtent, 0.f);

	Grid.SetNum(FluidConstants::TotalCells);
	FluidDeltas.SetNum(FluidConstants::TotalCells);

	CVarDebugDraw = IConsoleManager::Get().RegisterConsoleVariable(
		TEXT("fluid.DebugDraw"),
		0,
		TEXT("Draw debug boxes for fluid cells. 1=on, 0=off."),
		ECVF_Cheat
	);

	GetWorld()->GetTimerManager().SetTimer(
		SimTimerHandle,
		FTimerDelegate::CreateUObject(this, &UFluidSubsystem::SimStep),
		SimStepRate,
		/*bLoop=*/true
	);
}

void UFluidSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SimTimerHandle);
	}

	if (CVarDebugDraw)
	{
		IConsoleManager::Get().UnregisterConsoleObject(CVarDebugDraw);
		CVarDebugDraw = nullptr;
	}

	Super::Deinitialize();
}

void UFluidSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	BakeTerrainHeights();
}

// ---------------------------------------------------------------------------
// Terrain Baking
// ---------------------------------------------------------------------------

void UFluidSubsystem::BakeTerrainHeights()
{
	UWorld* World = GetWorld();
	if (!World) { return; }

	FCollisionQueryParams Params(SCENE_QUERY_STAT(FluidTerrainBake), /*bTraceComplex=*/false);
	const float TraceStartZ = 100000.f;
	const float TraceEndZ = -100000.f;

	for (int32 Y = 0; Y < FluidConstants::GridSize; ++Y)
	{
		for (int32 X = 0; X < FluidConstants::GridSize; ++X)
		{
			const FVector CellCenter = CellToWorld(X, Y);
			const FVector Start(CellCenter.X, CellCenter.Y, TraceStartZ);
			const FVector End(CellCenter.X, CellCenter.Y, TraceEndZ);

			FHitResult Hit;
			const bool bHit = World->LineTraceSingleByChannel(
				Hit, Start, End,
				ECC_WorldStatic,
				Params
			);

			Grid[GetCellIndex(X, Y)].TerrainHeight = bHit ? Hit.ImpactPoint.Z : 0.f;
		}
	}
}

// ---------------------------------------------------------------------------
// Flow Simulation — Accumulator Pattern
// ---------------------------------------------------------------------------

void UFluidSubsystem::SimStep()
{
	// Clear accumulator
	FMemory::Memzero(FluidDeltas.GetData(), FluidDeltas.Num() * sizeof(float));

	// Cardinal neighbor offsets
	static const int32 DX[4] = { 1, -1, 0,  0 };
	static const int32 DY[4] = { 0,  0, 1, -1 };

	// --- Pass 1: Compute transfers, accumulate deltas ---
	for (int32 Y = 0; Y < FluidConstants::GridSize; ++Y)
	{
		for (int32 X = 0; X < FluidConstants::GridSize; ++X)
		{
			const int32 Idx = GetCellIndex(X, Y);
			const FFluidCell& Cell = Grid[Idx];

			if (Cell.bFrozen || Cell.bBlocked) { continue; }
			if (Cell.FluidVolume <= KINDA_SMALL_NUMBER) { continue; }

			const float CellSurface = Cell.GetSurfaceHeight();
			const float CurrentVolume = Cell.FluidVolume;

			float TotalOutflow = 0.f;
			float NeighborTransfer[4] = {};

			for (int32 Dir = 0; Dir < 4; ++Dir)
			{
				const int32 NX = X + DX[Dir];
				const int32 NY = Y + DY[Dir];
				if (!IsValidCell(NX, NY)) { continue; }

				const int32 NIdx = GetCellIndex(NX, NY);
				const FFluidCell& Neighbor = Grid[NIdx];

				if (Neighbor.bBlocked) { continue; }

				const float Delta = CellSurface - Neighbor.GetSurfaceHeight();
				if (Delta <= 0.f) { continue; }

				float Transfer = Delta * FlowRate;
				Transfer = FMath::Min(Transfer, Delta * OscillationClamp);

				NeighborTransfer[Dir] = Transfer;
				TotalOutflow += Transfer;
			}

			// Scale back if total outflow exceeds available volume
			if (TotalOutflow > CurrentVolume && TotalOutflow > KINDA_SMALL_NUMBER)
			{
				const float Scale = CurrentVolume / TotalOutflow;
				for (int32 Dir = 0; Dir < 4; ++Dir)
				{
					NeighborTransfer[Dir] *= Scale;
				}
				TotalOutflow = CurrentVolume;
			}

			// Accumulate
			FluidDeltas[Idx] -= TotalOutflow;

			for (int32 Dir = 0; Dir < 4; ++Dir)
			{
				if (NeighborTransfer[Dir] <= 0.f) { continue; }
				const int32 NX = X + DX[Dir];
				const int32 NY = Y + DY[Dir];
				FluidDeltas[GetCellIndex(NX, NY)] += NeighborTransfer[Dir];
			}
		}
	}

	// --- Pass 2: Apply deltas ---
	for (int32 I = 0; I < FluidConstants::TotalCells; ++I)
	{
		Grid[I].FluidVolume = FMath::Max(0.f, Grid[I].FluidVolume + FluidDeltas[I]);
		// TODO Week 2: Derive FlowVelocity from directional outflow for material scrolling
	}

	// Debug draw
	if (bDebugDraw || (CVarDebugDraw && CVarDebugDraw->GetInt() != 0))
	{
		DrawDebugFluid();
	}
}

// ---------------------------------------------------------------------------
// Debug Visualization
// ---------------------------------------------------------------------------

void UFluidSubsystem::DrawDebugFluid() const
{
	UWorld* World = GetWorld();
	if (!World) { return; }

	static const float MaxDepth = 300.f;

	for (int32 Y = 0; Y < FluidConstants::GridSize; ++Y)
	{
		for (int32 X = 0; X < FluidConstants::GridSize; ++X)
		{
			const FFluidCell& Cell = Grid[GetCellIndex(X, Y)];
			if (Cell.FluidVolume < KINDA_SMALL_NUMBER) { continue; }

			const float T = FMath::Clamp(Cell.FluidVolume / MaxDepth, 0.f, 1.f);
			const uint8 R = static_cast<uint8>(T * 255.f);
			const uint8 B = static_cast<uint8>((1.f - T) * 255.f);
			const FColor Color(R, 0, B, 180);

			const FVector CellCenter = CellToWorld(X, Y);
			const float HalfHeight = FMath::Max(Cell.FluidVolume * 0.5f, 1.f);
			const FVector BoxCenter(
				CellCenter.X,
				CellCenter.Y,
				Cell.TerrainHeight + HalfHeight
			);
			const FVector HalfExtent(
				CellWorldSize * 0.45f,
				CellWorldSize * 0.45f,
				HalfHeight
			);

			DrawDebugBox(World, BoxCenter, HalfExtent, Color,
				/*bPersistent=*/false, /*LifeTime=*/SimStepRate * 1.1f);
		}
	}
}

// ---------------------------------------------------------------------------
// Coordinate Helpers
// ---------------------------------------------------------------------------

FIntPoint UFluidSubsystem::WorldToCell(FVector WorldPos) const
{
	const int32 X = FMath::FloorToInt((WorldPos.X - GridWorldOrigin.X) / CellWorldSize);
	const int32 Y = FMath::FloorToInt((WorldPos.Y - GridWorldOrigin.Y) / CellWorldSize);
	return FIntPoint(X, Y);
}

FVector UFluidSubsystem::CellToWorld(int32 X, int32 Y) const
{
	return FVector(
		GridWorldOrigin.X + (X + 0.5f) * CellWorldSize,
		GridWorldOrigin.Y + (Y + 0.5f) * CellWorldSize,
		0.f
	);
}

bool UFluidSubsystem::IsValidCell(int32 X, int32 Y) const
{
	return X >= 0 && X < FluidConstants::GridSize
		&& Y >= 0 && Y < FluidConstants::GridSize;
}

int32 UFluidSubsystem::GetCellIndex(int32 X, int32 Y) const
{
	checkf(IsValidCell(X, Y), TEXT("GetCellIndex called with invalid coords (%d, %d)"), X, Y);
	return Y * FluidConstants::GridSize + X;
}

// ---------------------------------------------------------------------------
// Public Gameplay API
// ---------------------------------------------------------------------------

float UFluidSubsystem::GetFluidHeightAtWorldPos(FVector WorldPos) const
{
	const FIntPoint Cell = WorldToCell(WorldPos);
	if (!IsValidCell(Cell.X, Cell.Y)) { return 0.f; }
	return Grid[GetCellIndex(Cell.X, Cell.Y)].GetSurfaceHeight();
}

void UFluidSubsystem::AddFluidAtCell(int32 X, int32 Y, float Amount)
{
	if (!IsValidCell(X, Y) || Amount <= 0.f) { return; }
	Grid[GetCellIndex(X, Y)].FluidVolume += Amount;
}

void UFluidSubsystem::RemoveFluidInRadius(FVector WorldPos, float Radius, float Amount)
{
	const FIntPoint Center = WorldToCell(WorldPos);
	const int32 CellRadius = FMath::CeilToInt(Radius / CellWorldSize);

	TArray<int32, TInlineAllocator<64>> AffectedCells;
	float TotalVolume = 0.f;

	for (int32 DY = -CellRadius; DY <= CellRadius; ++DY)
	{
		for (int32 DX = -CellRadius; DX <= CellRadius; ++DX)
		{
			const int32 X = Center.X + DX;
			const int32 Y = Center.Y + DY;
			if (!IsValidCell(X, Y)) { continue; }

			const FVector CellPos = CellToWorld(X, Y);
			if (FVector2D::Distance(FVector2D(WorldPos), FVector2D(CellPos)) > Radius) { continue; }

			const int32 Idx = GetCellIndex(X, Y);
			if (Grid[Idx].FluidVolume <= 0.f) { continue; }

			AffectedCells.Add(Idx);
			TotalVolume += Grid[Idx].FluidVolume;
		}
	}

	if (TotalVolume <= 0.f) { return; }

	// Proportional removal: deeper cells lose more volume. This gives a natural
	// "draining the deepest part first" feel for evaporators and the heat lance.
	const float RemoveFraction = FMath::Min(Amount / TotalVolume, 1.f);
	for (const int32 Idx : AffectedCells)
	{
		Grid[Idx].FluidVolume = FMath::Max(0.f, Grid[Idx].FluidVolume * (1.f - RemoveFraction));
	}
}

void UFluidSubsystem::ApplyForceInRadius(FVector Center, float Radius, FVector2D Force)
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
			const float Dist = FVector2D::Distance(FVector2D(Center), FVector2D(CellPos));
			if (Dist > Radius) { continue; }

			const int32 Idx = GetCellIndex(X, Y);
			if (Grid[Idx].FluidVolume <= KINDA_SMALL_NUMBER) { continue; }

			// TODO Week 2: Add velocity damping before integrating FlowVelocity into flow
			const float Falloff = 1.f - (Dist / Radius);
			Grid[Idx].FlowVelocity += Force * Falloff;
		}
	}
}

void UFluidSubsystem::SetFrozenInRadius(FVector Center, float Radius, bool bFreeze)
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
			if (FVector2D::Distance(FVector2D(Center), FVector2D(CellPos)) > Radius) { continue; }

			Grid[GetCellIndex(X, Y)].bFrozen = bFreeze;
		}
	}
}

void UFluidSubsystem::SetBlockedAtCell(int32 X, int32 Y, bool bBlock)
{
	if (!IsValidCell(X, Y)) { return; }
	Grid[GetCellIndex(X, Y)].bBlocked = bBlock;
}

float UFluidSubsystem::GetTotalFluidVolume() const
{
	float Total = 0.f;
	for (const FFluidCell& Cell : Grid)
	{
		Total += Cell.FluidVolume;
	}
	return Total;
}
