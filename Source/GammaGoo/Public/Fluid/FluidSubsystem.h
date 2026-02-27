// Copyright 2026 Bret Wright. All Rights Reserved.
// UFluidSubsystem owns the 128x128 heightfield and drives the shallow-water flow sim.
// All gameplay systems query and mutate fluid state exclusively through this class.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Fluid/FluidTypes.h"
#include "FluidSubsystem.generated.h"

class UTextureRenderTarget2D;

UCLASS()
class GAMMAGOO_API UFluidSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// --- UWorldSubsystem interface ---
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	// --- Public Gameplay API ---

	/** Returns fluid surface height (TerrainHeight + FluidVolume) at any world position. */
	UFUNCTION(BlueprintCallable, Category = "Fluid")
	float GetFluidHeightAtWorldPos(FVector WorldPos) const;

	/** Reduces FluidVolume in a sphere footprint. Used by towers, heat lance, siphons. */
	UFUNCTION(BlueprintCallable, Category = "Fluid")
	void RemoveFluidInRadius(FVector WorldPos, float Radius, float Amount);

	/** Adds fluid volume directly to a specific cell. Called by AFluidSource each tick. */
	UFUNCTION(BlueprintCallable, Category = "Fluid")
	void AddFluidAtCell(int32 X, int32 Y, float Amount);

	/** Adds a 2D flow velocity bias in a radius. Used by repulsor towers. */
	UFUNCTION(BlueprintCallable, Category = "Fluid")
	void ApplyForceInRadius(FVector Center, float Radius, FVector2D Force);

	/** Applies radial outward force in a radius. Each cell gets pushed away from center. Used by repulsor towers. */
	UFUNCTION(BlueprintCallable, Category = "Fluid")
	void ApplyRadialForceInRadius(FVector Center, float Radius, float Strength);

	/** Freezes or thaws cells in a radius. Frozen cells skip the flow step. */
	UFUNCTION(BlueprintCallable, Category = "Fluid")
	void SetFrozenInRadius(FVector Center, float Radius, bool bFreeze);

	/** Marks a single cell as blocked (levee wall). Blocked cells act as terrain. */
	UFUNCTION(BlueprintCallable, Category = "Fluid")
	void SetBlockedAtCell(int32 X, int32 Y, bool bBlock);

	/** Returns sum of all FluidVolume across the grid. Used by AWaveManager. */
	UFUNCTION(BlueprintCallable, Category = "Fluid")
	float GetTotalFluidVolume() const;

	// --- Grid coordinate helpers ---

	UFUNCTION(BlueprintCallable, Category = "Fluid|Grid")
	FIntPoint WorldToCell(FVector WorldPos) const;

	UFUNCTION(BlueprintCallable, Category = "Fluid|Grid")
	FVector CellToWorld(int32 X, int32 Y) const;

	UFUNCTION(BlueprintPure, Category = "Fluid|Grid")
	bool IsValidCell(int32 X, int32 Y) const;

	UFUNCTION(BlueprintPure, Category = "Fluid|Grid")
	int32 GetCellIndex(int32 X, int32 Y) const;

	/** Read-only grid access for rendering systems. */
	const TArray<FFluidCell>& GetGrid() const { return Grid; }

	/** Called by AFluidSurfaceRenderer to register render targets for GPU updates. */
	void SetRenderTargets(UTextureRenderTarget2D* HeightRT, UTextureRenderTarget2D* FlowRT);

protected:
	// --- Grid state ---

	UPROPERTY(VisibleAnywhere, Category = "Fluid|Grid")
	TArray<FFluidCell> Grid;

	/** World-space position of cell [0,0]. Grid is centered on world origin by default. */
	UPROPERTY(EditAnywhere, Category = "Fluid|Grid")
	FVector GridWorldOrigin = FVector::ZeroVector;

	// --- Tuning parameters ---

	UPROPERTY(EditAnywhere, Category = "Fluid|Tuning", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FlowRate = FluidConstants::DefaultFlowRate;

	UPROPERTY(EditAnywhere, Category = "Fluid|Tuning", meta = (ClampMin = "50.0", ClampMax = "500.0"))
	float CellWorldSize = FluidConstants::DefaultCellWorldSize;

	UPROPERTY(EditAnywhere, Category = "Fluid|Tuning", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float OscillationClamp = FluidConstants::DefaultOscillationClamp;

	UPROPERTY(EditAnywhere, Category = "Fluid|Tuning", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float VelocityDamping = FluidConstants::DefaultVelocityDamping;

	UPROPERTY(EditAnywhere, Category = "Fluid|Tuning", meta = (ClampMin = "0.001", ClampMax = "1.0"))
	float SimStepRate = FluidConstants::DefaultSimStepRate;

	// --- Debug ---

	UPROPERTY(EditAnywhere, Category = "Fluid|Debug")
	bool bDebugDraw = false;

private:
	void BakeTerrainHeights();
	void SimStep();
	void DrawDebugFluid() const;

	FTimerHandle SimTimerHandle;

	/** Per-step accumulator parallel to Grid. Avoids double-buffer allocation. */
	TArray<float> FluidDeltas;

	/** Per-step velocity accumulator: tracks directional outflow for FlowVelocity derivation. */
	TArray<FVector2D> FlowVelocityDeltas;

	/** Writes fluid grid data to Height and Flow render targets for the surface renderer. */
	void UpdateRenderTargets();

	UPROPERTY()
	TObjectPtr<UTextureRenderTarget2D> HeightRenderTarget = nullptr;

	UPROPERTY()
	TObjectPtr<UTextureRenderTarget2D> FlowRenderTarget = nullptr;

	IConsoleVariable* CVarDebugDraw = nullptr;
};
