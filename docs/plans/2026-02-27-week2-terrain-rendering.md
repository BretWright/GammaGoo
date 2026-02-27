# Week 2: Terrain + Rendering Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Visible fluid flowing through the Millbrook Valley landscape — the first time the game looks like a game.

**Architecture:** Fix BakeTerrainHeights to work with landscapes, derive FlowVelocity from sim outflow, create AFluidSurfaceRenderer (owns displaced plane mesh + two render targets updated from grid data each sim step), and build the Substrate material in-editor via AIK. The landscape itself is sculpted in-editor.

**Tech Stack:** UE 5.7 C++ (UWorldSubsystem, AActor, UProceduralMeshComponent or static mesh), Substrate material system, UTextureRenderTarget2D (PF_R16F for heights, PF_G16R16F for flow), AgentIntegrationKit MCP tools for material creation.

---

## Task 1: Fix BakeTerrainHeights Timing

The current code fires BakeTerrainHeights() in Initialize(), which runs before landscape actors are registered. This must be deferred to world begin play.

**Files:**
- Modify: `Source/GammaGoo/Public/Fluid/FluidSubsystem.h`
- Modify: `Source/GammaGoo/Private/Fluid/FluidSubsystem.cpp`

**Step 1: Add OnWorldBeginPlay override to header**

In `FluidSubsystem.h`, add to the public UWorldSubsystem interface section:

```cpp
virtual void OnWorldBeginPlay(UWorld& InWorld) override;
```

**Step 2: Move BakeTerrainHeights call from Initialize to OnWorldBeginPlay**

In `FluidSubsystem.cpp`, remove the `BakeTerrainHeights()` call and the TODO comment from `Initialize()`. Add the new override:

```cpp
void UFluidSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	BakeTerrainHeights();
}
```

The sim timer stays in `Initialize()` — it's fine to tick with zero terrain heights for the brief window before BeginPlay. No fluid sources are active yet (they start in their own BeginPlay which fires after subsystem OnWorldBeginPlay).

**Step 3: Compile and verify**

Run: Build in UE editor or via `UnrealBuildTool`.
Expected: Clean compile, no errors.

**Step 4: Commit**

```bash
git add Source/GammaGoo/Public/Fluid/FluidSubsystem.h Source/GammaGoo/Private/Fluid/FluidSubsystem.cpp
git commit -m "fix: defer BakeTerrainHeights to OnWorldBeginPlay for landscape support"
```

---

## Task 2: Derive FlowVelocity from Directional Outflow + Add Velocity Damping

Currently FlowVelocity is always zero (TODO in SimStep Pass 2) and ApplyForceInRadius accumulates unboundedly. Fix both.

**Files:**
- Modify: `Source/GammaGoo/Private/Fluid/FluidSubsystem.cpp`
- Modify: `Source/GammaGoo/Public/Fluid/FluidTypes.h` (add damping constant)

**Step 1: Add velocity damping constant to FluidConstants**

In `FluidTypes.h`, add to the `FluidConstants` namespace:

```cpp
constexpr float DefaultVelocityDamping = 0.9f;  // Per-step multiplier. 0.9 = 10% decay per step.
```

**Step 2: Add VelocityDamping tuning parameter to FluidSubsystem.h**

In the tuning parameters section of `FluidSubsystem.h`:

```cpp
UPROPERTY(EditAnywhere, Category = "Fluid|Tuning", meta = (ClampMin = "0.0", ClampMax = "1.0"))
float VelocityDamping = FluidConstants::DefaultVelocityDamping;
```

**Step 3: Add a parallel FlowVelocityDeltas buffer**

In `FluidSubsystem.h`, add to the private section (next to `FluidDeltas`):

```cpp
TArray<FVector2D> FlowVelocityDeltas;
```

In `FluidSubsystem.cpp` `Initialize()`, after `FluidDeltas.SetNum(...)`:

```cpp
FlowVelocityDeltas.SetNum(FluidConstants::TotalCells);
```

**Step 4: Compute FlowVelocity from directional outflow in SimStep**

Replace the SimStep flow loop to track directional outflow. In the Pass 1 inner loop, after accumulating `FluidDeltas`, also accumulate flow velocity from actual transfers. Then in Pass 2, apply damping and merge.

In `SimStep()`, clear the velocity deltas buffer at the top (after the FluidDeltas memzero):

```cpp
FMemory::Memzero(FlowVelocityDeltas.GetData(), FlowVelocityDeltas.Num() * sizeof(FVector2D));
```

In Pass 1, add direction vectors for the 4 cardinal neighbors (after the existing DX/DY arrays):

```cpp
static const FVector2D DirVec[4] = {
    FVector2D(1.f, 0.f),   // +X (East)
    FVector2D(-1.f, 0.f),  // -X (West)
    FVector2D(0.f, 1.f),   // +Y (North)
    FVector2D(0.f, -1.f)   // -Y (South)
};
```

In the inner loop, after writing `FluidDeltas[GetCellIndex(NX, NY)] += NeighborTransfer[Dir];`, also write:

```cpp
FlowVelocityDeltas[Idx] += DirVec[Dir] * NeighborTransfer[Dir];
```

In Pass 2, replace the existing TODO with:

```cpp
// Derive FlowVelocity: damp existing + add new outflow direction
Grid[I].FlowVelocity = Grid[I].FlowVelocity * VelocityDamping + FlowVelocityDeltas[I];
```

**Step 5: Remove the TODO comment in ApplyForceInRadius**

The `// TODO Week 2: Add velocity damping...` comment at line 327 of FluidSubsystem.cpp is now resolved by the per-step damping. Remove the comment.

**Step 6: Compile and verify**

Expected: Clean compile. Debug draw still works. FlowVelocity now shows non-zero values in cells where fluid flows.

**Step 7: Commit**

```bash
git add Source/GammaGoo/Public/Fluid/FluidTypes.h Source/GammaGoo/Public/Fluid/FluidSubsystem.h Source/GammaGoo/Private/Fluid/FluidSubsystem.cpp
git commit -m "feat: derive FlowVelocity from outflow direction, add velocity damping"
```

---

## Task 3: Add Render Target Update API to UFluidSubsystem

The subsystem needs to write grid data to render targets each sim step so the surface renderer can read them. Two render targets: height (R16F) and flow (G16R16F).

**Files:**
- Modify: `Source/GammaGoo/Public/Fluid/FluidSubsystem.h`
- Modify: `Source/GammaGoo/Private/Fluid/FluidSubsystem.cpp`

**Step 1: Add render target pointers and update method to header**

In `FluidSubsystem.h`, add to the public section:

```cpp
/** Called by AFluidSurfaceRenderer to register render targets for GPU updates. */
void SetRenderTargets(UTextureRenderTarget2D* HeightRT, UTextureRenderTarget2D* FlowRT);
```

Add to the private section:

```cpp
void UpdateRenderTargets();

UPROPERTY()
TObjectPtr<UTextureRenderTarget2D> HeightRenderTarget = nullptr;

UPROPERTY()
TObjectPtr<UTextureRenderTarget2D> FlowRenderTarget = nullptr;
```

Add include at top of header:

```cpp
#include "Engine/TextureRenderTarget2D.h"
```

**Step 2: Implement SetRenderTargets and UpdateRenderTargets**

In `FluidSubsystem.cpp`:

```cpp
void UFluidSubsystem::SetRenderTargets(UTextureRenderTarget2D* HeightRT, UTextureRenderTarget2D* FlowRT)
{
	HeightRenderTarget = HeightRT;
	FlowRenderTarget = FlowRT;
}

void UFluidSubsystem::UpdateRenderTargets()
{
	if (!HeightRenderTarget || !FlowRenderTarget) { return; }

	const int32 Size = FluidConstants::GridSize;

	// --- Height render target (R16F — fluid surface Z) ---
	{
		FTextureRenderTargetResource* Resource = HeightRenderTarget->GameThread_GetRenderTargetResource();
		if (!Resource) { return; }

		TArray<FFloat16Color> Pixels;
		Pixels.SetNum(Size * Size);

		for (int32 I = 0; I < FluidConstants::TotalCells; ++I)
		{
			const float SurfaceHeight = Grid[I].GetSurfaceHeight();
			// Encode surface height. Visible range ~0-5000cm. R16F handles this natively.
			Pixels[I] = FFloat16Color(
				FLinearColor(SurfaceHeight, 0.f, 0.f, Grid[I].FluidVolume > KINDA_SMALL_NUMBER ? 1.f : 0.f)
			);
		}

		// Lock and write
		FRenderTarget* RT = Resource->GetRenderTargetTexture() ? Resource : nullptr;
		if (RT)
		{
			uint32 Stride = 0;
			void* Data = RHILockTexture2D(
				Resource->GetRenderTargetTexture(),
				0, RLM_WriteOnly, Stride, false);
			if (Data)
			{
				FMemory::Memcpy(Data, Pixels.GetData(), Pixels.Num() * sizeof(FFloat16Color));
				RHIUnlockTexture2D(Resource->GetRenderTargetTexture(), 0, false);
			}
		}
	}

	// --- Flow render target (RG16F — FlowVelocity.XY) ---
	{
		FTextureRenderTargetResource* Resource = FlowRenderTarget->GameThread_GetRenderTargetResource();
		if (!Resource) { return; }

		TArray<FFloat16Color> Pixels;
		Pixels.SetNum(Size * Size);

		for (int32 I = 0; I < FluidConstants::TotalCells; ++I)
		{
			const FVector2D& Flow = Grid[I].FlowVelocity;
			// Encode flow velocity. Remap from [-MaxFlow, MaxFlow] to [0, 1] range.
			// MaxFlow of 500 is generous — most values will be small.
			const float MaxFlow = 500.f;
			const float R = FMath::Clamp((Flow.X / MaxFlow) * 0.5f + 0.5f, 0.f, 1.f);
			const float G = FMath::Clamp((Flow.Y / MaxFlow) * 0.5f + 0.5f, 0.f, 1.f);
			const float B = Grid[I].bFrozen ? 1.f : 0.f;
			Pixels[I] = FFloat16Color(FLinearColor(R, G, B, 1.f));
		}

		FRenderTarget* RT = Resource->GetRenderTargetTexture() ? Resource : nullptr;
		if (RT)
		{
			uint32 Stride = 0;
			void* Data = RHILockTexture2D(
				Resource->GetRenderTargetTexture(),
				0, RLM_WriteOnly, Stride, false);
			if (Data)
			{
				FMemory::Memcpy(Data, Pixels.GetData(), Pixels.Num() * sizeof(FFloat16Color));
				RHIUnlockTexture2D(Resource->GetRenderTargetTexture(), 0, false);
			}
		}
	}
}
```

**Step 3: Call UpdateRenderTargets at end of SimStep**

In `SimStep()`, after Pass 2 and before the debug draw block, add:

```cpp
UpdateRenderTargets();
```

**Step 4: Compile and verify**

Expected: Clean compile. No runtime change yet (no render targets registered).

**Step 5: Commit**

```bash
git add Source/GammaGoo/Public/Fluid/FluidSubsystem.h Source/GammaGoo/Private/Fluid/FluidSubsystem.cpp
git commit -m "feat: add render target update pipeline for fluid height and flow textures"
```

**IMPORTANT NOTE:** The RHI texture lock approach above may need adjustment depending on the render target format and UE 5.7's threading model. An alternative approach is to use `UCanvas::DrawTile` or `UKismetRenderingLibrary::DrawMaterialToRenderTarget`. If the RHI lock approach fails to compile or crashes at runtime, fall back to:

```cpp
// Alternative: CPU-side FUpdateTextureRegion2D approach
void UFluidSubsystem::UpdateRenderTargets()
{
    if (!HeightRenderTarget) { return; }

    const int32 Size = FluidConstants::GridSize;

    // Use ENQUEUE_RENDER_COMMAND for thread-safe texture update
    TArray<FFloat16Color> HeightPixels;
    HeightPixels.SetNum(Size * Size);

    for (int32 I = 0; I < FluidConstants::TotalCells; ++I)
    {
        const float SurfaceHeight = Grid[I].GetSurfaceHeight();
        HeightPixels[I] = FFloat16Color(
            FLinearColor(SurfaceHeight, 0.f, 0.f,
                Grid[I].FluidVolume > KINDA_SMALL_NUMBER ? 1.f : 0.f)
        );
    }

    // Write to render target via DrawMaterialToRenderTarget or direct texture update
    // The exact UE 5.7 API will be verified at compile time
}
```

---

## Task 4: Create AFluidSurfaceRenderer Actor

This actor owns the plane mesh and render targets. It registers them with the subsystem and provides the visual surface.

**Files:**
- Create: `Source/GammaGoo/Public/Fluid/FluidSurfaceRenderer.h`
- Create: `Source/GammaGoo/Private/Fluid/FluidSurfaceRenderer.cpp`

**Step 1: Create the header**

```cpp
// Copyright 2026 Bret Wright. All Rights Reserved.
// AFluidSurfaceRenderer owns the fluid plane mesh and render targets.
// Purely visual — all simulation state lives in UFluidSubsystem.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FluidSurfaceRenderer.generated.h"

class UStaticMeshComponent;
class UTextureRenderTarget2D;
class UFluidSubsystem;
class UMaterialInstanceDynamic;

UCLASS(BlueprintType, Blueprintable)
class GAMMAGOO_API AFluidSurfaceRenderer : public AActor
{
	GENERATED_BODY()

public:
	AFluidSurfaceRenderer();

	virtual void BeginPlay() override;

protected:
	/** The plane mesh that gets displaced by the material. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fluid|Rendering")
	TObjectPtr<UStaticMeshComponent> FluidPlaneMesh;

	/** 128x128 R16F — encodes fluid surface height per cell. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid|Rendering")
	TObjectPtr<UTextureRenderTarget2D> HeightRenderTarget;

	/** 128x128 RGBA16F — RG = FlowVelocity.XY, B = bFrozen. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid|Rendering")
	TObjectPtr<UTextureRenderTarget2D> FlowRenderTarget;

	/** Base material to create dynamic instance from. Set in Blueprint. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid|Rendering")
	TObjectPtr<UMaterialInterface> FluidMaterial;

private:
	void CreateRenderTargets();

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial;
};
```

**Step 2: Create the implementation**

```cpp
// Copyright 2026 Bret Wright. All Rights Reserved.

#include "Fluid/FluidSurfaceRenderer.h"
#include "Fluid/FluidSubsystem.h"
#include "Fluid/FluidTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"

AFluidSurfaceRenderer::AFluidSurfaceRenderer()
{
	PrimaryActorTick.bCanEverTick = false;

	FluidPlaneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FluidPlaneMesh"));
	RootComponent = FluidPlaneMesh;

	// Mesh assigned in Blueprint (128x128 subdivided plane)
	FluidPlaneMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FluidPlaneMesh->SetCastShadow(false);
}

void AFluidSurfaceRenderer::BeginPlay()
{
	Super::BeginPlay();

	CreateRenderTargets();

	// Register render targets with subsystem
	if (UFluidSubsystem* Subsystem = GetWorld()->GetSubsystem<UFluidSubsystem>())
	{
		Subsystem->SetRenderTargets(HeightRenderTarget, FlowRenderTarget);
	}

	// Create dynamic material instance and bind textures
	if (FluidMaterial)
	{
		DynamicMaterial = UMaterialInstanceDynamic::Create(FluidMaterial, this);
		FluidPlaneMesh->SetMaterial(0, DynamicMaterial);

		if (HeightRenderTarget)
		{
			DynamicMaterial->SetTextureParameterValue(TEXT("HeightTexture"), HeightRenderTarget);
		}
		if (FlowRenderTarget)
		{
			DynamicMaterial->SetTextureParameterValue(TEXT("FlowTexture"), FlowRenderTarget);
		}
	}
}

void AFluidSurfaceRenderer::CreateRenderTargets()
{
	const int32 Size = FluidConstants::GridSize; // 128

	if (!HeightRenderTarget)
	{
		HeightRenderTarget = NewObject<UTextureRenderTarget2D>(this, TEXT("RT_FluidHeight"));
		HeightRenderTarget->InitAutoFormat(Size, Size);
		HeightRenderTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_R16f;
		HeightRenderTarget->Filter = TF_Bilinear;
		HeightRenderTarget->AddressX = TA_Clamp;
		HeightRenderTarget->AddressY = TA_Clamp;
		HeightRenderTarget->UpdateResourceImmediate(true);
	}

	if (!FlowRenderTarget)
	{
		FlowRenderTarget = NewObject<UTextureRenderTarget2D>(this, TEXT("RT_FluidFlow"));
		FlowRenderTarget->InitAutoFormat(Size, Size);
		FlowRenderTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA16f;
		FlowRenderTarget->Filter = TF_Bilinear;
		FlowRenderTarget->AddressX = TA_Clamp;
		FlowRenderTarget->AddressY = TA_Clamp;
		FlowRenderTarget->UpdateResourceImmediate(true);
	}
}
```

**Step 3: Compile and verify**

Expected: Clean compile. Actor can be placed in editor, shows a plane mesh component.

**Step 4: Commit**

```bash
git add Source/GammaGoo/Public/Fluid/FluidSurfaceRenderer.h Source/GammaGoo/Private/Fluid/FluidSurfaceRenderer.cpp
git commit -m "feat: add AFluidSurfaceRenderer with render target creation and subsystem binding"
```

---

## Task 5: Create Subdivided Plane Mesh Asset

The fluid surface needs a 128x128 subdivided plane mesh. This can be created as a static mesh asset in-editor.

**Files:**
- Create: Content asset `Content/FluidSiege/Meshes/SM_FluidPlane` (via editor)

**Step 1: Create the plane mesh in editor**

Using the Modeling Tools plugin (already enabled), create a subdivided plane:
- Dimensions: 12800 x 12800 cm (128m x 128m, matching grid extent)
- Subdivisions: 128 x 128 (one vertex per grid cell)
- Save as: `Content/FluidSiege/Meshes/SM_FluidPlane`

Alternatively, this can be generated via Python in-editor:

```python
# Via UE Python console or AIK execute_python
import unreal
# Use the Modeling Tools API or ProceduralMeshComponent to generate the plane
```

**Note:** If the modeling tools API is not scriptable, create this manually in the editor using Add > Shapes > Plane and setting subdivision in the modeling mode. The exact method depends on what AIK tools support.

**Step 2: Verify mesh**

Open in Static Mesh Editor. Confirm vertex count is ~16,641 (129x129 vertices for 128x128 quads). Confirm dimensions match 12800x12800.

---

## Task 6: Build Millbrook Valley Landscape

This is editor work — sculpt the 3-tier terrain per the implementation plan specification.

**Terrain Tiers:**
- High Ridge (north/east): Z 2000-2500
- Town Plateau (center): Z 1200-1500, ~40x40m flat area
- Low Basin (south/west): Z 800-1000

**Step 1: Create a new level**

Create: `Content/FluidSiege/Maps/Lvl_MillbrookValley`

**Step 2: Add landscape**

- Create a landscape covering at least 128x128m
- Scale so that the fluid grid aligns with the play area
- The landscape should extend slightly beyond the grid to provide visual context

**Step 3: Sculpt terrain to spec**

Using the Landscape sculpting tools:
- North/east edges: Ridge at Z 2000-2500
- Center: Mesa/plateau at Z 1200-1500, flat ~40x40m
- South/west: Basin at Z 800-1000
- Slopes connecting the tiers (steep on west, gradual on east, moderate on south)

**Step 4: Place fluid sources**

- S1: North ridge at approximately Z 2200
- S2: Northeast ridge at approximately Z 2400 (set `bActiveOnBeginPlay = false` — activated by wave manager in Wave 3)
- S3: Low basin at approximately Z 900 (set `bActiveOnBeginPlay = false` — triggered by volume threshold)

**Step 5: Place AFluidSurfaceRenderer**

- Position at the grid origin (0,0,0) so the plane aligns with the fluid grid
- Assign SM_FluidPlane mesh
- Assign M_FluidSurface material (created in Task 7)

**Step 6: Test BakeTerrainHeights**

- PIE the level
- Run `fluid.DebugDraw 1`
- Verify: debug boxes appear at correct terrain heights (not all at Z=0)
- Activate S1 and verify fluid flows downhill from ridge toward plateau

**Step 7: Commit level assets**

```bash
git add Content/FluidSiege/Maps/ Content/FluidSiege/Meshes/
git commit -m "feat: add Millbrook Valley level with 3-tier terrain and fluid sources"
```

---

## Task 7: Create Fluid Surface Substrate Material (M_FluidSurface)

The material reads the two render targets and produces the fluid surface visuals. This is created in-editor via the Material Editor (or AIK if it supports material creation).

**Material Parameters (TextureParameter):**
- `HeightTexture` — RT_FluidHeight (R16F)
- `FlowTexture` — RT_FluidFlow (RGBA16F: RG=flow, B=frozen, A=unused)

**Material Parameter (Scalar):**
- `GridWorldSize` — 12800.0 (128 * 100cm)
- `MaxFluidHeight` — 5000.0 (max expected surface Z)
- `ShallowDepthThreshold` — 30.0 (ankle depth in cm)
- `FoamEdgeThreshold` — 10.0 (edge detection threshold)

**Material Parameter (Vector):**
- `ShallowColor` — Teal (0.0, 0.6, 0.5, 0.2)
- `DeepColor` — Dark Blue (0.02, 0.05, 0.15, 0.9)
- `FrozenColor` — Ice White (0.85, 0.9, 0.95, 1.0)

**Material Logic:**

1. **World Position to UV:** Map vertex world XY to 0-1 UV range using GridWorldSize and grid origin
2. **Height Sampling:** Sample HeightTexture R channel at UV. This is the fluid surface Z.
3. **World Position Offset:** Output (0, 0, SampledHeight - VertexWorldZ) to displace vertices to fluid surface
4. **Opacity:** Alpha channel of HeightTexture (1 where fluid exists, 0 where dry). Use dithered opacity (opaque blend mode with masked)
5. **Base Color:** Lerp ShallowColor to DeepColor based on fluid depth (FluidVolume = SampledHeight - TerrainHeight, normalized)
6. **Normal Scrolling:** Two tiling normal maps panned by FlowTexture RG channels (decoded from 0-1 back to -1..1 range)
7. **Foam Edge:** Where fluid alpha transitions near 0, blend white foam texture
8. **Frozen Overlay:** Where FlowTexture.B = 1, override base color with FrozenColor, kill normal scroll, boost specular

**Blend Mode:** Masked (with dithered opacity for soft edges)

**Step 1: Create the material in-editor**

Save as: `Content/FluidSiege/Materials/M_FluidSurface`

This material is complex enough that it's best built in the Material Editor GUI. The implementation agent should create the material node graph matching the logic above. If AIK material tools are available, use those; otherwise, guide the user to build it manually or create it as a Material Function chain.

**Step 2: Create two placeholder normal map textures**

- `Content/FluidSiege/Materials/T_WaterNormal_A` — tileable water normal
- `Content/FluidSiege/Materials/T_WaterNormal_B` — second tileable water normal (different scale)

These can be starter engine content normals or generated.

**Step 3: Assign material to FluidSurfaceRenderer in the level**

Set the `FluidMaterial` property on the AFluidSurfaceRenderer actor to `M_FluidSurface`.

**Step 4: PIE and verify**

- Fluid plane should be invisible where no fluid exists (masked out)
- Where fluid sources are active, plane should rise to fluid surface height
- Color should shift from teal (shallow) to dark blue (deep)
- Normal maps should scroll in flow direction

**Step 5: Commit**

```bash
git add Content/FluidSiege/Materials/
git commit -m "feat: add M_FluidSurface Substrate material with height displacement and depth coloring"
```

---

## Task 8: Integration Test & Polish Pass

Final verification that all Week 2 systems work together.

**Step 1: Full integration test**

In Lvl_MillbrookValley:
1. PIE with S1 active
2. Verify: fluid spawns at ridge, flows downhill, pools on plateau and in basin
3. Verify: fluid surface is visible with correct coloring
4. Verify: debug draw (`fluid.DebugDraw 1`) matches visual surface position
5. Verify: no significant frame drops (should be well above 60 FPS)

**Step 2: Tune FlowVelocity visual feedback**

If normal map scrolling is too fast/slow, adjust:
- `MaxFlow` constant in `UpdateRenderTargets()` (currently 500)
- Normal map tiling scale in material
- Panning speed multiplier in material

**Step 3: Verify frozen overlay**

Place a test call to `SetFrozenInRadius()` (via Blueprint or console). Verify frozen cells show ice overlay in material.

**Step 4: Update CONTEXT.md**

Update with Week 2 completion status, any issues found, and Week 3 next steps.

**Step 5: Final commit**

```bash
git commit -m "feat: complete Week 2 — terrain, rendering, flow velocity integration"
```

---

## Task Dependencies

```
Task 1 (BakeTerrainHeights fix)
    └── Task 6 (Millbrook Valley) — needs working terrain baking
Task 2 (FlowVelocity + damping)
    └── Task 3 (Render target update) — needs velocity data to encode
        └── Task 4 (FluidSurfaceRenderer) — consumes render targets
            └── Task 7 (Material) — assigned to renderer
                └── Task 8 (Integration) — tests everything together
Task 5 (Plane mesh) — independent, needed by Task 6 placement
```

**Recommended execution order:** Tasks 1, 2, 3, 4 (sequential C++ chain) → Task 5 (mesh) → Task 6 (landscape) → Task 7 (material) → Task 8 (integration)

---

## Known Risks

1. **RHI texture lock threading** — The `RHILockTexture2D` approach in Task 3 may need to be wrapped in `ENQUEUE_RENDER_COMMAND` or replaced with `UKismetRenderingLibrary` utilities. The implementation agent should try the direct approach first, and fall back if it crashes or produces visual artifacts.

2. **Render target format compatibility** — `RTF_R16f` may not be available on all platforms. If compilation fails, fall back to `RTF_RGBA16f` for both targets and pack height into R channel.

3. **Landscape line trace timing** — Even with `OnWorldBeginPlay`, landscape collision may need one more frame to fully initialize. If terrain heights are still zero, add a short delay (0.1s timer) before baking.

4. **Material complexity** — The Substrate material with WPO + masking + normal scrolling + frozen overlay is moderately complex. Build incrementally: get WPO working first, then add visual layers one at a time.
