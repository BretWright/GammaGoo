# GammaGoo — CLAUDE.md

## Quick Orientation

GammaGoo (codename: **Fluid Siege**) is a fluid-based tower defense game built in Unreal Engine 5.7. The enemy is a rising liquid that spawns from source points, flows based on terrain height, and floods the player's town. Players build defensive towers and wield a personal heat lance from a third-person perspective to evaporate, redirect, freeze, and drain the fluid.

Solo-developed by Bret. Claude AI is a core development pipeline tool.

**Engine:** Unreal Engine 5.7
**Required Plugins:** GameplayStateTree, AgentIntegrationKit, ModelingToolsEditorMode
**Future Plugins (PoC):** GameplayAbilities, EnhancedInput, Niagara, Chaos Physics

Before working on any UE 5.7 engine systems in depth, consult the `unreal-engine-57` skill for API-level reference on Chaos Physics, GAS, Enhanced Input, Substrate, etc.

---

## Project Architecture

### Module Plan

The PoC uses a single game module. Scale to multi-module if the project graduates from prototype.

| Module | Purpose | Key Classes |
|---|---|---|
| **GammaGoo** | All gameplay: fluid sim, towers, player, wave management | UFluidSubsystem, AFluidTowerBase, AFluidDefenseCharacter, AWaveManager |

### Directory Layout

```
GammaGoo/
├── Source/
│   └── GammaGoo/{Public,Private}/
│       ├── Fluid/          ← UFluidSubsystem, FFluidCell, AFluidSource, AFluidSurfaceRenderer
│       ├── Towers/         ← AFluidTowerBase, AEvaporatorTower, ARepulsorTower, ASiphonTower, ALeveeWall, ACryoSpike
│       ├── Player/         ← AFluidDefenseCharacter, UBuildComponent
│       ├── Game/           ← AWaveManager, UResourceSubsystem, ATownHall
│       └── UI/             ← HUD widgets (wave counter, resources, Town Hall HP)
├── Content/
│   ├── Characters/Mannequins/   ← Existing third-person assets
│   ├── ThirdPerson/             ← Base level and materials
│   ├── Variant_Combat/          ← Combat variant (reference for weapon systems)
│   ├── Input/                   ← Enhanced Input actions and mapping contexts
│   ├── FluidSiege/              ← NEW: All Fluid Siege content
│   │   ├── Maps/                ← Millbrook Valley test level
│   │   ├── Materials/           ← Fluid surface material, tower materials
│   │   ├── Meshes/              ← Tower meshes, levee wall, fluid plane
│   │   ├── VFX/                 ← Niagara systems (steam, splash, frost, foam)
│   │   ├── Blueprints/          ← Tower BPs, wave manager BP
│   │   └── UI/                  ← HUD widget blueprints
│   └── LevelPrototyping/       ← Existing prototype assets
├── Config/
├── Design/                      ← Implementation plan (.docx)
├── CLAUDE.md                    ← This file
├── CONTEXT.md                   ← Living state — UPDATE EVERY SESSION
└── GammaGoo.uproject
```

---

## Naming Conventions

These are mandatory. Every class, asset, and identifier must follow this scheme:

| Type | Prefix | Example |
|---|---|---|
| C++ classes (Fluid) | `AFluid` / `UFluid` | `AFluidSource`, `UFluidSubsystem` |
| C++ classes (Tower) | `A...Tower` / `A...Wall` / `A...Spike` | `AEvaporatorTower`, `ALeveeWall` |
| C++ classes (Player) | `AFluidDefense...` | `AFluidDefenseCharacter` |
| Blueprints | `BP_` | `BP_EvaporatorTower`, `BP_FluidSource` |
| Materials | `M_` / `MI_` | `M_FluidSurface`, `MI_FluidFrozen` |
| Niagara Systems | `NS_` | `NS_Steam`, `NS_SplashRing`, `NS_FrostCrystal` |
| Input Actions | `IA_` | `IA_Fire`, `IA_Build`, `IA_PlaceTower` |
| Input Mapping Contexts | `IMC_` | `IMC_Default`, `IMC_BuildMode` |
| Data Assets | `DA_` | `DA_TowerConfig_Evaporator` |
| Gameplay Tags | Dot-separated | `Tower.Evaporator`, `Fluid.Frozen`, `Wave.Active` |
| Render Targets | `RT_` | `RT_FluidHeight`, `RT_FluidFlow` |

Use `FGameplayTag` over enums for state identifiers wherever possible.

---

## Core Systems Reference

### Fluid Simulation (UFluidSubsystem)

The heart of the game. A `UWorldSubsystem` that owns a 128x128 heightfield grid.

- **Grid cell data:** TerrainHeight (float), FluidVolume (float), bFrozen (bool), bBlocked (bool), FlowVelocity (FVector2D)
- **Flow algorithm:** Simplified shallow-water pipe model at fixed 30Hz substep
- **Oscillation prevention:** Transfer clamped to 50% of pressure differential per step
- **Key tuning parameters:**
  - `FlowRate` (0.25) — viscosity control
  - `CellWorldSize` (100cm) — grid resolution
  - `SimStepRate` (30Hz) — fixed timestep

**Public API** — all gameplay queries go through these methods:
- `GetFluidHeightAtWorldPos()` — player/building depth checks
- `RemoveFluidInRadius()` — evaporators, heat lance, siphons
- `AddFluidAtCell()` — fluid sources
- `ApplyForceInRadius()` — repulsor towers
- `SetFrozenInRadius()` — cryo spikes
- `SetBlockedAtCell()` — levee walls
- `GetTotalFluidVolume()` — wave manager escalation checks

### Tower System (AFluidTowerBase)

All towers use timer-driven effects (never Tick). Base class provides: EffectRadius, EffectInterval, Health, BuildCost.

| Tower | Type | API Call | Cost |
|---|---|---|---|
| Evaporator Pylon | DESTROY | RemoveFluidInRadius | 100 |
| Repulsor Tower | DISPLACE | ApplyForceInRadius | 150 |
| Siphon Well | EXPLOIT | RemoveFluid + AddCurrency | 75 |
| Levee Wall | CONTAIN | SetBlockedAtCell | 25 |
| Cryo Spike | CONTAIN | SetFrozenInRadius | 200 |

### Player Character (AFluidDefenseCharacter)

- Third-person with Mover 2.0 movement component
- Fluid depth query on 0.1s timer modifies speed and applies DoT damage
- Heat lance: line trace from camera, RemoveFluidInRadius at impact, energy-limited
- Build system via UBuildComponent: ghost mesh placement, grid snapping, validity checking

### Wave Manager (AWaveManager)

- 5-wave sequence with escalating source count and spawn rates
- 30-second build phase between waves (fluid persists, no new spawns)
- Source 3 basin trigger at total volume threshold (5000 units)
- Positive feedback: more fluid → more sources → exponential pressure

### Rendering

- Single subdivided plane mesh (128x128 verts)
- UTextureRenderTarget2D receives fluid heights each sim step
- Substrate material: World Position Offset from height texture, depth-based color, scrolling normals, foam edge detection, frozen overlay
- Use dithered opacity (opaque pass) for PoC, not true translucency

---

## Code Style

- Follow Unreal Engine coding standards
- Use `UPROPERTY`, `UFUNCTION` macros on all reflected members
- Prefer `TObjectPtr<>` over raw pointers
- Use `FGameplayTag` over enums for identifiers
- Header comments explain "why," not "what"
- Timer-driven effects over Tick — use `GetWorldTimerManager().SetTimer()`
- Use `UWorldSubsystem` for singletons, not static globals
- `ParallelFor` for grid operations if scaling beyond 128x128

---

## Workflow Rules

These are non-negotiable session rules:

1. **ALWAYS update `CONTEXT.md` before ending a session.** Include: what was done, what's next, any open decisions or bugs. This is the most important rule.
2. **Read `CONTEXT.md` at session start** to pick up where the last session left off.
3. **The Implementation Plan is the design source of truth.** Check `Design/Fluid_Siege_Implementation_Plan.docx` for design questions.
4. **Commit atomically.** One feature or fix per commit with descriptive messages.
5. **Never work directly on `main`** for features. Use feature branches.
6. **New Content goes in `Content/FluidSiege/`** — don't scatter assets in the root Content folder.
7. **Fluid subsystem is the single source of truth** for all fluid state. No system should cache or duplicate grid data.

### Compact Instructions

When context is compacted during a long session, preserve:
- Current implementation state from CONTEXT.md
- Open bugs and blockers
- Active tuning parameter values
- The next 3 milestones

Discard: full file contents already committed, exploratory dead ends.
