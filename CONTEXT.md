# GammaGoo — CONTEXT.md

> **Last Updated:** 2026-02-27
> **Session:** Initial project setup & design phase

---

## Current State

### What Exists
- UE 5.7 project with Third-Person Shooter template (Mannequin character, combat variant)
- Plugins enabled: GameplayStateTree, AgentIntegrationKit, ModelingToolsEditorMode
- Existing content: ThirdPerson level, Variant_Combat level with weapons/VFX/UI, Enhanced Input setup
- GitHub repo: https://github.com/BretWright/GammaGoo.git (branch: main)

### What Was Done This Session
- Designed complete Fluid Siege tower defense PoC (weapon/combat system, fluid simulation, level design)
- Created `Fluid_Siege_Implementation_Plan.docx` — full technical GDD covering all systems
- Set up project documentation: CLAUDE.md, CONTEXT.md, updated README.md
- Pushed all documentation to GitHub

### Implementation Status

| System | Status | Notes |
|---|---|---|
| Fluid Simulation (UFluidSubsystem) | **DESIGNED** | Full architecture, API, flow algorithm spec'd |
| Tower System (5 tower types) | **DESIGNED** | Stats, costs, synergy matrix complete |
| Player Character | **DESIGNED** | Movement penalties, heat lance, build system |
| Test Level (Millbrook Valley) | **DESIGNED** | 3-tier topology, flow paths, strategic zones |
| Fluid Rendering | **DESIGNED** | Heightfield → render target → displaced plane |
| Wave System | **DESIGNED** | 5-wave sequence, basin trigger, build phases |
| C++ Classes | **NOT STARTED** | Header files need scaffolding |

---

## What's Next

### Immediate (Week 1 — Fluid Simulation Core)
1. Create `Source/GammaGoo/` C++ module structure
2. Implement `FFluidCell` struct and `UFluidSubsystem` with grid init
3. Implement flow algorithm with fixed 30Hz substep
4. Debug visualization: `DrawDebugBox` colored by fluid depth
5. Place one `AFluidSource` actor on a flat test terrain
6. **Milestone:** Fluid flows downhill and pools correctly

### After That (Week 2 — Terrain + Rendering)
1. Build Millbrook Valley landscape (3-tier: ridge, plateau, basin)
2. Sample terrain heights into fluid grid at init
3. Create fluid plane mesh (128x128 subdivisions)
4. Build Substrate material with height texture displacement
5. **Milestone:** Visible water flowing through the valley

### Then (Week 3 — Player + Towers)
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
- **GameplayStateTree plugin:** Already enabled — use for wave state management (idle → build phase → wave active → wave complete)?
- **AgentIntegrationKit:** Purpose TBD — check what this provides.

## Known Issues / Blockers

- GitHub push from Cowork sandbox blocked (no credentials). Must push from local machine.
- `C:\project\GammaGoo` is outside home directory — can't mount in Cowork. Use git clone for access.
