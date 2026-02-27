# GammaGoo — CONTEXT.md

> **Last Updated:** 2026-02-27
> **Session:** Week 2 — Terrain + Rendering COMPLETE
> **Branch:** `main` (Week 2 merged, pushed to origin)

---

## Current State

### What Exists
- UE 5.7 project with Third-Person Shooter template (Mannequin character, combat variant)
- Plugins enabled: AgentIntegrationKit, ModelingToolsEditorMode
- Existing content: ThirdPerson level, Variant_Combat level, Enhanced Input setup
- GitHub repo: https://github.com/BretWright/GammaGoo.git
- **Week 1 complete:** Full fluid simulation core (UFluidSubsystem, AFluidSource)
- **Week 2 complete:** Terrain, rendering pipeline, surface material, test level — compiled and merged

### What Was Done This Session (Week 2)
1. **Fixed BakeTerrainHeights timing** — moved from `Initialize()` to `OnWorldBeginPlay()` so landscape/terrain actors are registered before line traces fire
2. **Derived FlowVelocity from outflow** — DirVec[4] tracks cardinal direction of each transfer in SimStep Pass 1, accumulated into FlowVelocityDeltas buffer, applied with per-step damping (0.9 decay) in Pass 2
3. **Added velocity damping** — `VelocityDamping` tuning parameter prevents FlowVelocity from accumulating unboundedly from `ApplyForceInRadius`
4. **Built render target update pipeline** — `UpdateRenderTargets()` writes grid data to two render targets each sim step via `ENQUEUE_RENDER_COMMAND`:
   - Height RT (RGBA16F): R=SurfaceHeight, G=FluidVolume, B=0, A=HasFluid
   - Flow RT (RGBA16F): R=FlowVelocity.X (0.5=zero), G=FlowVelocity.Y, B=bFrozen, A=1
5. **Created AFluidSurfaceRenderer** — actor that owns plane mesh + render targets, registers them with subsystem, creates dynamic material instance with HeightTexture/FlowTexture bindings
6. **Created SM_FluidPlane** — 128x128 subdivided plane mesh (16641 verts, 12800x12800cm) imported via OBJ
7. **Created M_FluidSurface** — Substrate material with:
   - World Position → UV mapping for grid-aligned texture sampling
   - WPO vertex displacement from height texture
   - Depth-based color (ShallowColor teal → DeepColor dark blue via normalized FluidVolume)
   - Frozen overlay (lerps to FrozenColor when FlowTexture.B=1)
   - Roughness switching (0.15 wet, 0.05 frozen)
   - Opacity mask from has-fluid flag (masked blend mode)
8. **Built Millbrook Valley test level** (Lvl_MillbrookValley) — 64 terrain tiles forming 3-tier bowl topology with 3 fluid source placeholders and renderer placeholder
9. **Fixed Height RT format** — changed from RTF_R16f to RTF_RGBA16f (was writing 4-channel FFloat16Color to single-channel texture)
10. **Compiled successfully** — all C++ verified against UE 5.7 RHI API (LockTexture2D exists but deprecated since 5.6)
11. **Merged to main** — 8 commits fast-forwarded, pushed to origin

### Implementation Status

| System | Status | Notes |
|---|---|---|
| Fluid Simulation (UFluidSubsystem) | **IMPLEMENTED** | Grid, flow, debug vis, full API, render target updates, FlowVelocity derivation |
| Fluid Source (AFluidSource) | **IMPLEMENTED** | Timer-driven spawn, cached grid coords |
| Fluid Types (FFluidCell) | **IMPLEMENTED** | Struct + FluidConstants (added DefaultVelocityDamping) |
| Fluid Surface Renderer | **IMPLEMENTED** | AFluidSurfaceRenderer + SM_FluidPlane + M_FluidSurface |
| Test Level (Millbrook Valley) | **BUILT (PROTOTYPE)** | Box-tile terrain, placeholder actors — needs real landscape later |
| Tower System (5 tower types) | **DESIGNED** | Stats, costs, synergy matrix complete |
| Player Character | **DESIGNED** | Movement penalties, heat lance, build system |
| Wave System | **DESIGNED** | 5-wave sequence, basin trigger, build phases |

### Key Files

| File | Purpose |
|---|---|
| `Source/GammaGoo/Public/Fluid/FluidTypes.h` | FFluidCell struct, FluidConstants namespace (incl. VelocityDamping) |
| `Source/GammaGoo/Public/Fluid/FluidSubsystem.h` | UFluidSubsystem — full API + render target support + OnWorldBeginPlay |
| `Source/GammaGoo/Private/Fluid/FluidSubsystem.cpp` | Flow algorithm, terrain baking, FlowVelocity derivation, RT updates |
| `Source/GammaGoo/Public/Fluid/FluidSource.h` | AFluidSource header |
| `Source/GammaGoo/Private/Fluid/FluidSource.cpp` | Fluid source implementation |
| `Source/GammaGoo/Public/Fluid/FluidSurfaceRenderer.h` | AFluidSurfaceRenderer header |
| `Source/GammaGoo/Private/Fluid/FluidSurfaceRenderer.cpp` | Renderer: RT creation, subsystem binding, MID setup |
| `Content/FluidSiege/Meshes/SM_FluidPlane.uasset` | 128x128 subdivided plane mesh |
| `Content/FluidSiege/Materials/M_FluidSurface.uasset` | Substrate material with WPO + depth color |
| `Content/FluidSiege/Maps/Lvl_MillbrookValley.umap` | 3-tier terrain test level |
| `docs/plans/2026-02-27-week2-terrain-rendering.md` | Week 2 implementation plan |

---

## What's Next

### Immediate — PIE Test Week 2
1. Replace placeholder actors in Lvl_MillbrookValley with real C++ actors (AFluidSource, AFluidSurfaceRenderer)
2. PIE test: verify fluid flows downhill, surface renders, debug draw matches visual
3. **Milestone:** Visible water flowing through the valley

### Week 3 — Player + Towers
1. Adapt existing third-person character for AFluidDefenseCharacter
2. Add fluid depth movement/damage system (5 depth tiers)
3. Implement heat lance weapon (15 vol/s, 150cm radius, energy-limited)
4. Build UBuildComponent with ghost placement, grid snapping, slope validation
5. Implement all 5 tower types (Evaporator, Repulsor, Siphon, Levee, Cryo)
6. **Milestone:** Player can build towers and fight the fluid

### Week 4 — Waves + Polish
1. AWaveManager with 5-wave sequence
2. Source 3 basin trigger (volume > 5000)
3. ATownHall lose condition, UResourceSubsystem (start 500)
4. HUD widgets, Niagara VFX pass
5. **Milestone:** Full playable PoC loop

---

## Open Decisions

- **Terrain approach:** Currently using box-tile terrain for PoC. May upgrade to proper UE Landscape with sculpted heightmap for visual quality pass.
- **GameplayStateTree plugin:** Listed in CLAUDE.md but missing from .uproject. Add before wave system work (Week 4).
- **Existing Variant_Combat content:** Repurpose weapon VFX and UI as starting points for heat lance and tower effects.
- **RHI texture lock API:** Current code uses `RHICmdList.LockTexture2D` (deprecated since UE 5.6, functional in 5.7). Consider migrating to `LockTexture()` during polish pass.

## Known Issues / Blockers

- **Placeholder actors in Millbrook Valley** — need replacing with real AFluidSource and AFluidSurfaceRenderer before PIE test
- **Normal map scrolling** — M_FluidSurface does not yet have panning normal maps driven by FlowVelocity. Add during visual polish pass.
- **LockTexture2D deprecation** — API works in UE 5.7 but marked for removal. Migrate to `LockTexture()` when convenient.
