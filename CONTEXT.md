# GammaGoo — CONTEXT.md

> **Last Updated:** 2026-02-27
> **Session:** Week 1 — Fluid Simulation Core implementation

---

## Current State

### What Exists
- UE 5.7 project with Third-Person Shooter template (Mannequin character, combat variant)
- Plugins enabled: GameplayStateTree, AgentIntegrationKit, ModelingToolsEditorMode
- Existing content: ThirdPerson level, Variant_Combat level with weapons/VFX/UI, Enhanced Input setup
- GitHub repo: https://github.com/BretWright/GammaGoo.git (branch: main)

### What Was Done This Session
- Fixed `.gitignore` (added Binaries/, Intermediate/, DerivedDataCache/, .vs/, *.sln, .worktrees/)
- Removed 9 accidentally tracked Intermediate files from git
- Fixed DefaultGame.ini project name to "GammaGoo"
- **Implemented UFluidSubsystem** — full 128x128 heightfield grid with accumulator-pattern flow algorithm at 30Hz
- **Implemented AFluidSource** — timer-driven fluid injection actor with Activate/Deactivate API
- Debug visualization via `fluid.DebugDraw` CVar (blue→red depth coloring)
- All public API methods: GetFluidHeightAtWorldPos, RemoveFluidInRadius, AddFluidAtCell, ApplyForceInRadius, SetFrozenInRadius, SetBlockedAtCell, GetTotalFluidVolume

### Implementation Status

| System | Status | Notes |
|---|---|---|
| Fluid Simulation (UFluidSubsystem) | **IMPLEMENTED** | Grid init, flow algorithm, debug vis, full API |
| Fluid Source (AFluidSource) | **IMPLEMENTED** | Timer-driven spawn, cached grid coords |
| Fluid Types (FFluidCell) | **IMPLEMENTED** | Struct + FluidConstants namespace |
| Tower System (5 tower types) | **DESIGNED** | Stats, costs, synergy matrix complete |
| Player Character | **DESIGNED** | Movement penalties, heat lance, build system |
| Test Level (Millbrook Valley) | **DESIGNED** | 3-tier topology, flow paths, strategic zones |
| Fluid Rendering | **DESIGNED** | Heightfield → render target → displaced plane |
| Wave System | **DESIGNED** | 5-wave sequence, basin trigger, build phases |

### Key Files

| File | Purpose |
|---|---|
| `Source/GammaGoo/Public/Fluid/FluidTypes.h` | FFluidCell struct, FluidConstants namespace |
| `Source/GammaGoo/Public/Fluid/FluidSubsystem.h` | UFluidSubsystem header — full public API |
| `Source/GammaGoo/Private/Fluid/FluidSubsystem.cpp` | Flow algorithm, terrain baking, debug draw |
| `Source/GammaGoo/Public/Fluid/FluidSource.h` | AFluidSource header |
| `Source/GammaGoo/Private/Fluid/FluidSource.cpp` | Fluid source implementation |

---

## What's Next

### Immediate — Verify & Test
1. Open project in UE 5.7 editor and compile
2. Place AFluidSource on the flat ThirdPerson level
3. Run `fluid.DebugDraw 1` in console — verify fluid spreads symmetrically
4. **Milestone:** Fluid flows and pools correctly on flat terrain

### Week 2 — Terrain + Rendering
1. Build Millbrook Valley landscape (3-tier: ridge, plateau, basin)
2. Fix BakeTerrainHeights timing (may need to defer to OnWorldBeginPlay for landscapes)
3. Create fluid plane mesh (128x128 subdivisions)
4. Build Substrate material with height texture displacement
5. Derive FlowVelocity from directional outflow (TODO in SimStep Pass 2)
6. **Milestone:** Visible water flowing through the valley

### Week 3 — Player + Towers
1. Adapt existing third-person character for AFluidDefenseCharacter
2. Add fluid depth movement/damage system
3. Implement heat lance weapon
4. Build UBuildComponent with ghost placement
5. Implement all 5 tower types
6. **Milestone:** Player can build towers and fight the fluid

---

## Open Decisions

- **Fluid representation:** 2D heightfield confirmed for PoC. May revisit with Niagara Fluids for visual polish pass.
- **Existing Variant_Combat content:** Can repurpose weapon systems, VFX, and UI as starting points for heat lance and tower VFX.
- **GameplayStateTree plugin:** Already enabled — use for wave state management?
- **FlowVelocity derivation:** Currently zero in sim (TODO Week 2). Need per-direction outflow tracking.
- **ApplyForceInRadius damping:** FlowVelocity accumulates unboundedly — add per-step decay in Week 2.

## Known Issues / Blockers

- **Not yet compiled** — needs UE 5.7 editor build verification
- BakeTerrainHeights fires during Initialize() before landscape actors may be registered (OK for flat plane, needs fix for Week 2 landscape)
- Build.cs has `"UMG"` in private deps — verify this is a valid module name in UE 5.7
