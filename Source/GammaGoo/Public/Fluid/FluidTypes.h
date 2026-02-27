// Copyright 2026 Bret Wright. All Rights Reserved.
// Core data types for the fluid simulation grid.

#pragma once

#include "CoreMinimal.h"
#include "FluidTypes.generated.h"

/**
 * Single cell in the 128x128 fluid heightfield grid.
 * TerrainHeight is baked once at init. FluidVolume changes every sim step.
 */
USTRUCT(BlueprintType)
struct GAMMAGOO_API FFluidCell
{
	GENERATED_BODY()

	/** Baked terrain Z at cell center. Set once via downward line trace. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fluid")
	float TerrainHeight = 0.f;

	/** Current fluid volume on this cell. Surface = TerrainHeight + FluidVolume. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fluid")
	float FluidVolume = 0.f;

	/** Cryo spike has frozen this cell. Frozen cells skip flow. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fluid")
	bool bFrozen = false;

	/** Levee wall occupies this cell. Acts as impassable terrain. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fluid")
	bool bBlocked = false;

	/** Derived flow direction for visual effects (normal map scroll). Not sim-critical. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fluid")
	FVector2D FlowVelocity = FVector2D::ZeroVector;

	/** Convenience: total surface height at this cell. */
	FORCEINLINE float GetSurfaceHeight() const { return TerrainHeight + FluidVolume; }
};

/** Grid dimensions — single source of truth. */
namespace FluidConstants
{
	constexpr int32 GridSize = 128;
	constexpr int32 TotalCells = GridSize * GridSize;
	constexpr float DefaultCellWorldSize = 100.f;   // 100cm = 1 Unreal meter
	constexpr float DefaultFlowRate = 0.25f;         // Viscosity control
	constexpr float DefaultOscillationClamp = 0.5f;  // Max transfer fraction
	constexpr float DefaultSimStepRate = 1.f / 30.f; // 30Hz fixed timestep
}
