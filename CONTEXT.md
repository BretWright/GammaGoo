# GammaGoo — CONTEXT.md

> **Last Updated:** 2026-02-27
> **Session:** Week 3 — Player + Towers COMPLETE
> **Branch:** `feature/week3-player-towers` (pushed to origin, PR pending)

---

## Current State

### What Exists
- UE 5.7 project with Third-Person Shooter template (Mannequin character, combat variant)
- Plugins enabled: AgentIntegrationKit, ModelingToolsEditorMode
- Existing content: ThirdPerson level, Variant_Combat level, Enhanced Input setup
- GitHub repo: https://github.com/BretWright/GammaGoo.git
- **Week 1 complete:** Full fluid simulation core (UFluidSubsystem, AFluidSource)
- **Week 2 complete:** Terrain, rendering pipeline, surface material, test level — compiled and merged
- **Week 3 complete:** All 5 tower types, player character, build system, town hall, resource subsystem — pushed, PR pending

### What Was Done This Session (Week 3)
1. **UResourceSubsystem** — UGameInstanceSubsystem for player currency (start 500, spend/earn)
2. **AFluidTowerBase** — abstract tower base with timer-driven ExecuteEffect loop (never Tick)
3. **5 tower subclasses:**
   - AEvaporatorTower (DESTROY: RemoveFluidInRadius, 500cm, cost 100)
   - ARepulsorTower (DISPLACE: ApplyRadialForceInRadius, 600cm, cost 150)
   - ASiphonTower (EXPLOIT: drain + currency via ConversionRatio 0.5, cost 75)
   - ALeveeWall (CONTAIN: SetBlockedAtCell, 1x3 footprint, 500 HP, pressure damage, cost 25)
   - ACryoSpike (CONTAIN: freeze/thaw cycle 30s/5s, 450cm, cost 200)
4. **ApplyRadialForceInRadius** — new UFluidSubsystem API for per-cell outward force with distance falloff
5. **ATownHall** — lose condition actor, timer-driven fluid depth damage, OnTownDestroyed delegate
6. **AFluidDefenseCharacter** — 5 fluid depth tiers (Dry/Ankle/Knee/Waist/Chest) with speed multipliers (1.0/0.8/0.5/0.3/0.15) and DoT (0/0/5/15/30 HP/s), heat lance (30Hz fire, 150cm radius, 15 vol/s, energy 100 max, 10/s drain, 5/s recharge), Enhanced Input bindings
7. **UBuildComponent** — camera-projected ghost mesh, grid snapping via WorldToCell/CellToWorld, slope validation (30° max), fluid/blocked/cost checks, tower spawning with resource deduction
8. **FluidSource fix** — added SceneComponent root for level placement
9. **Project config** — added module definition to .uproject, removed missing GameplayStateTree plugin, set default map to MillbrookValley, updated BuildSettings to V6

### Implementation Status

| System | Status | Notes |
|---|---|---|
| Fluid Simulation (UFluidSubsystem) | **IMPLEMENTED** | Grid, flow, debug vis, full API, render target updates, FlowVelocity derivation, ApplyRadialForceInRadius |
| Fluid Source (AFluidSource) | **IMPLEMENTED** | Timer-driven spawn, cached grid coords, SceneComponent root |
| Fluid Types (FFluidCell) | **IMPLEMENTED** | Struct + FluidConstants (incl. VelocityDamping) |
| Fluid Surface Renderer | **IMPLEMENTED** | AFluidSurfaceRenderer + SM_FluidPlane + M_FluidSurface |
| Test Level (Millbrook Valley) | **BUILT (PROTOTYPE)** | Box-tile terrain, placeholder actors — needs real landscape later |
| Tower Base (AFluidTowerBase) | **IMPLEMENTED** | Timer-driven effects, health, build cost, damage system |
| Evaporator Tower | **IMPLEMENTED** | RemoveFluidInRadius, 500cm, 50 vol/tick |
| Repulsor Tower | **IMPLEMENTED** | ApplyRadialForceInRadius, 600cm, 200 strength |
| Siphon Tower | **IMPLEMENTED** | Drain + currency conversion (0.5 ratio) |
| Levee Wall | **IMPLEMENTED** | SetBlockedAtCell, 1x3 footprint, pressure damage |
| Cryo Spike | **IMPLEMENTED** | Freeze/thaw cycle (30s/5s cooldown) |
| Town Hall | **IMPLEMENTED** | Fluid depth damage, OnTownDestroyed broadcast |
| Resource Subsystem | **IMPLEMENTED** | Start 500, AddCurrency, SpendCurrency, OnResourceChanged |
| Player Character | **IMPLEMENTED** | 5 depth tiers, heat lance, Enhanced Input, energy system |
| Build Component | **IMPLEMENTED** | Ghost placement, grid snap, slope/fluid/cost validation |
| Wave System | **DESIGNED** | 5-wave sequence, basin trigger, build phases |

### Key Files

| File | Purpose |
|---|---|
| `Source/GammaGoo/Public/Fluid/FluidTypes.h` | FFluidCell struct, FluidConstants namespace |
| `Source/GammaGoo/Public/Fluid/FluidSubsystem.h` | UFluidSubsystem — full API incl. ApplyRadialForceInRadius |
| `Source/GammaGoo/Private/Fluid/FluidSubsystem.cpp` | Flow algorithm, terrain baking, RT updates |
| `Source/GammaGoo/Public/Fluid/FluidSource.h` | AFluidSource header |
| `Source/GammaGoo/Private/Fluid/FluidSource.cpp` | Fluid source with SceneComponent root |
| `Source/GammaGoo/Public/Fluid/FluidSurfaceRenderer.h` | AFluidSurfaceRenderer header |
| `Source/GammaGoo/Private/Fluid/FluidSurfaceRenderer.cpp` | Renderer: RT creation, subsystem binding, MID setup |
| `Source/GammaGoo/Public/Towers/FluidTowerBase.h` | Abstract tower: radius, interval, cost, health |
| `Source/GammaGoo/Private/Towers/FluidTowerBase.cpp` | Timer-driven ExecuteEffect loop |
| `Source/GammaGoo/Public/Towers/EvaporatorTower.h` | DESTROY tower header |
| `Source/GammaGoo/Public/Towers/RepulsorTower.h` | DISPLACE tower header |
| `Source/GammaGoo/Public/Towers/SiphonTower.h` | EXPLOIT tower header |
| `Source/GammaGoo/Public/Towers/LeveeWall.h` | CONTAIN wall header |
| `Source/GammaGoo/Public/Towers/CryoSpike.h` | CONTAIN freeze tower header |
| `Source/GammaGoo/Public/Game/ResourceSubsystem.h` | Currency tracking subsystem |
| `Source/GammaGoo/Public/Game/TownHall.h` | Lose condition actor |
| `Source/GammaGoo/Public/Player/FluidDefenseCharacter.h` | Player character with depth tiers + heat lance |
| `Source/GammaGoo/Public/Player/BuildComponent.h` | Ghost placement + grid snapping |
| `Content/FluidSiege/Maps/Lvl_MillbrookValley.umap` | 3-tier terrain test level |
| `docs/plans/2026-02-27-week3-player-towers.md` | Week 3 implementation plan |

---

## What's Next

### Immediate — PIE Test Week 2+3
1. Replace placeholder actors in Lvl_MillbrookValley with real C++ actors (AFluidSource, AFluidSurfaceRenderer)
2. Place tower actors, TownHall, set up player character BP
3. PIE test: verify fluid flows, towers affect fluid, player depth tiers work
4. **Milestone:** Visible water flowing, towers working, player can build

### Week 4 — Waves + Polish
1. AWaveManager with 5-wave sequence
2. Source 3 basin trigger (volume > 5000)
3. Build phase timer (30s between waves)
4. HUD widgets (wave counter, resources, Town Hall HP)
5. Niagara VFX pass (steam, splash, frost, foam)
6. Create Enhanced Input assets (IA_Fire, IA_Build, IMC_FluidSiege)
7. **Milestone:** Full playable PoC loop

---

## Open Decisions

- **Terrain approach:** Currently using box-tile terrain for PoC. May upgrade to proper UE Landscape with sculpted heightmap for visual quality pass.
- **GameplayStateTree plugin:** Removed from .uproject (was missing). Re-add if needed for wave system state machine in Week 4.
- **Existing Variant_Combat content:** Repurpose weapon VFX and UI as starting points for heat lance and tower effects.
- **RHI texture lock API:** Current code uses `RHICmdList.LockTexture2D` (deprecated since UE 5.6, functional in 5.7). Consider migrating to `LockTexture()` during polish pass.
- **Player Character BP:** Need to create BP_FluidDefenseCharacter blueprint, assign input actions and mapping context, set up camera.

## Known Issues / Blockers

- **Placeholder actors in Millbrook Valley** — need replacing with real AFluidSource and AFluidSurfaceRenderer before PIE test
- **Normal map scrolling** — M_FluidSurface does not yet have panning normal maps driven by FlowVelocity. Add during visual polish pass.
- **LockTexture2D deprecation** — API works in UE 5.7 but marked for removal. Migrate to `LockTexture()` when convenient.
- **Enhanced Input assets not yet created** — IA_Fire, IA_Build, IMC_FluidSiege need to be created as data assets in-editor
- **Ghost mesh material** — UBuildComponent ghost color relies on a MID with "GhostColor" parameter; needs a simple ghost material created in-editor
