# GammaGoo

**Fluid Siege** — A fluid-based tower defense game built in Unreal Engine 5.7.

The enemy is a rising liquid. It spawns from source points, flows based on terrain height, pools in low areas, and floods your town. Build towers to evaporate, redirect, freeze, and drain the fluid. Wield a heat lance to patch breaches while you manage defenses from a third-person perspective.

## Project Status

Proof of Concept — Design phase complete, implementation starting.

## Tech Stack

- **Engine:** Unreal Engine 5.7
- **Perspective:** Third-person
- **Core System:** 2D heightfield fluid simulation (CPU, 128x128 grid, 30Hz fixed step)
- **Plugins:** GameplayStateTree, AgentIntegrationKit, ModelingToolsEditorMode

## Documentation

- `CLAUDE.md` — AI assistant project context and coding conventions
- `CONTEXT.md` — Living session state (read at start, update at end)
- `Design/Fluid_Siege_Implementation_Plan.docx` — Full technical implementation plan

## Tower Types

| Tower | Role | Interaction |
|---|---|---|
| Evaporator Pylon | Primary damage | Destroys fluid volume |
| Repulsor Tower | Area denial | Pushes fluid away |
| Siphon Well | Economy | Drains fluid for resources |
| Levee Wall | Terrain control | Blocks and redirects flow |
| Cryo Spike | Emergency | Freezes fluid temporarily |
