# GammaGoo — CONTEXT.md

> **Last Updated:** 2026-02-27
> **Session:** Week 4 — Waves + HUD COMPLETE
> **Branch:** `feature/week4-waves-polish` (ready to push)

---

## Current State

### What Exists
- UE 5.7 project with Third-Person Shooter template (Mannequin character, combat variant)
- Plugins enabled: AgentIntegrationKit, ModelingToolsEditorMode
- Existing content: ThirdPerson level, Variant_Combat level, Enhanced Input setup
- GitHub repo: https://github.com/BretWright/GammaGoo.git
- **Week 1 complete:** Full fluid simulation core (UFluidSubsystem, AFluidSource)
- **Week 2 complete:** Terrain, rendering pipeline, surface material, test level
- **Week 3 complete:** All 5 tower types, player character, build system, town hall, resource subsystem
- **Week 4 complete:** Wave manager, gameplay HUD, health delegate — all C++ for the PoC loop

### What Was Done This Session (Week 4)
1. **ATownHall OnHealthChanged delegate** — FOnTownHealthChanged broadcasts (CurrentHealth, MaxHealth) every damage tick for HUD binding
2. **AFluidSource SetSpawnRate/GetSpawnRate** — public accessors so AWaveManager can control spawn rates without direct protected member access
3. **AWaveManager** — 5-wave state machine (EWaveState: PreGame/BuildPhase/WaveActive/Victory/Defeat):
   - FWaveConfig struct with SourceCount, SpawnRateMultiplier, Duration, bBasinTriggerEnabled
   - Default config: Wave1(1,1.0x,60s) → Wave5(3,2.5x,120s)
   - 30s build phase between waves (sources deactivated, fluid persists)
   - Basin trigger: GetTotalFluidVolume() > 5000 activates tagged basin sources mid-wave
   - Base spawn rate caching for both regular and basin sources (no cumulative multiplication)
   - Source discovery via actor tags ("FluidSource", "BasinSource")
   - ATownHall OnTownDestroyed binding for defeat condition
   - Delegates: OnWaveStateChanged, OnWaveNumberChanged
4. **UFluidSiegeHUD** — UUserWidget with BindWidgetOptional text blocks and progress bars:
   - Wave counter ("Wave 3 / 5"), timer ("BUILD 25s" / "WAVE 45s"), state label
   - Resource display (currency from UResourceSubsystem)
   - Town Hall HP bar (green, red when <25%) + percentage text
   - Player energy bar + percentage text
   - NativeTick polling with full null-safety
   - CacheReferences via TActorIterator + GetGameInstance subsystem
5. **HUD wiring** — AFluidDefenseCharacter creates UFluidSiegeHUD in BeginPlay (guarded by IsLocallyControlled), calls InitHUD(this), adds to viewport

### Implementation Status

| System | Status | Notes |
|---|---|---|
| Fluid Simulation (UFluidSubsystem) | **IMPLEMENTED** | Grid, flow, debug vis, full API, RT updates, ApplyRadialForceInRadius |
| Fluid Source (AFluidSource) | **IMPLEMENTED** | Timer-driven spawn, SceneRoot, SetSpawnRate/GetSpawnRate |
| Fluid Types (FFluidCell) | **IMPLEMENTED** | Struct + FluidConstants |
| Fluid Surface Renderer | **IMPLEMENTED** | AFluidSurfaceRenderer + SM_FluidPlane + M_FluidSurface |
| Test Level (Millbrook Valley) | **BUILT (PROTOTYPE)** | Box-tile terrain, placeholder actors |
| Tower Base + 5 Types | **IMPLEMENTED** | Evaporator, Repulsor, Siphon, Levee, Cryo |
| Town Hall | **IMPLEMENTED** | Fluid depth damage, OnTownDestroyed, OnHealthChanged |
| Resource Subsystem | **IMPLEMENTED** | Start 500, Add/Spend/CanAfford, OnResourceChanged |
| Player Character | **IMPLEMENTED** | 5 depth tiers, heat lance, energy, Enhanced Input, HUD creation |
| Build Component | **IMPLEMENTED** | Ghost placement, grid snap, validation |
| Wave Manager | **IMPLEMENTED** | 5-wave sequence, build phases, basin trigger, victory/defeat |
| Gameplay HUD | **IMPLEMENTED** | Wave/timer/resource/health/energy display |

### Key Files (Week 4 additions)

| File | Purpose |
|---|---|
| `Source/GammaGoo/Public/Game/WaveManager.h` | EWaveState, FWaveConfig, AWaveManager class |
| `Source/GammaGoo/Private/Game/WaveManager.cpp` | Wave state machine, source management, basin trigger |
| `Source/GammaGoo/Public/UI/FluidSiegeHUD.h` | UFluidSiegeHUD with BindWidgetOptional bindings |
| `Source/GammaGoo/Private/UI/FluidSiegeHUD.cpp` | NativeTick polling of all gameplay systems |
| `docs/plans/2026-02-27-week4-waves-polish.md` | Week 4 implementation plan |

---

## What's Next

### Immediate — In-Editor Setup + PIE Test
1. **Compile project** — verify all C++ builds clean
2. **Create Blueprints:**
   - BP_FluidDefenseCharacter (parent: AFluidDefenseCharacter) — assign camera, input actions, HUDWidgetClass
   - BP_WaveManager (parent: AWaveManager) — place in level
   - WBP_FluidSiegeHUD (parent: UFluidSiegeHUD) — layout text blocks + progress bars with matching names
3. **Create Enhanced Input assets:** IA_Fire, IA_Build, IMC_FluidSiege
4. **Level setup:**
   - Replace placeholder actors with real AFluidSource (tag "FluidSource") and AFluidSurfaceRenderer
   - Add basin source (tag "BasinSource")
   - Place ATownHall
   - Place BP_WaveManager
   - Set game mode to use BP_FluidDefenseCharacter
5. **PIE test:** Verify full gameplay loop — waves start, fluid flows, towers work, HUD updates, victory/defeat triggers
6. **Milestone:** Full playable PoC loop

### Future Polish (Post-PoC)
- Niagara VFX pass (NS_Steam, NS_SplashRing, NS_FrostCrystal, NS_Foam)
- Normal map scrolling on M_FluidSurface driven by FlowVelocity
- Ghost mesh material for build system
- Proper UE Landscape terrain
- Sound effects
- Game over / victory screen UI

---

## Open Decisions

- **Terrain approach:** Currently box-tile terrain for PoC. Upgrade to proper Landscape later.
- **Existing Variant_Combat content:** Repurpose weapon VFX for heat lance effects.
- **RHI texture lock API:** LockTexture2D deprecated since 5.6, migrate to LockTexture() during polish.

## Known Issues / Blockers

- **Placeholder actors in Millbrook Valley** — need replacing with real actors before PIE test
- **Enhanced Input assets not yet created** — IA_Fire, IA_Build, IMC_FluidSiege needed
- **Ghost mesh material** — UBuildComponent needs a MID with "GhostColor" parameter
- **WBP_FluidSiegeHUD not yet created** — need Widget Blueprint with named text blocks and progress bars
- **Normal map scrolling** — M_FluidSurface missing panning normals from FlowVelocity
