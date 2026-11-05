# Source

This folder contains the Unreal Build Tool target definitions and the C++ source for Project Vestige. The game is currently one module, `Vestige`, under `Vestige/`; the folders inside that module are architectural layers, not separate Unreal modules.

## Files and folders

| Path | Purpose |
|---|---|
| `Vestige.Target.cs` | Defines the game build target. |
| `VestigeEditor.Target.cs` | Defines the Unreal Editor build target used to work on the project. |
| `Vestige/` | The game's module: its module rules, gameplay code, and tests. |

### `Vestige/`

| Folder or file | Purpose |
|---|---|
| `Vestige.Build.cs` | Declares the module's Unreal dependencies and include path. |
| `Core/` | Shared services and module-level code that do not own game rules. |
| `AbilitySystem/` | Gameplay Ability System setup for stamina, abilities, effects, and native tags. |
| `Interaction/` | Interactable interface, interaction component, and interaction ability. |
| `Content/` | Content data definitions and the Asset Manager. |
| `Time/` | The clock, its settings and save data, and the sky lighting that follows it. |
| `Inventory/` | Inventory slots, stack rules, component, and save data. |
| `Farming/` | Farm tiles and grid, farming and rest behavior, tools, toolbar, bed, and related save data. |
| `Save/` | Save-game data gathering and the save subsystem. |
| `Player/` | Player character, movement component, and player state. |
| `UI/` | HUD, its widget, and HUD settings and messages. |
| `Template/` | Unreal Third Person template classes retained for game-mode and player-controller setup. |
| `Tests/` | CQTest automation tests and test-only helpers. |

The layers are ordered from lower-level shared code toward player-facing systems. Keep dependencies pointing down the architecture: a lower layer must not include a higher one. Include project headers by their path from the module root, for example `#include "Farming/FarmTile.h"`.

For ownership, dependency rules, and the layer table, see [`Architecture.md`](../Docs/Architecture.md). 

C++ style, build, and test guidance is in [`CodingGuidelines.md`](../Docs/CodingGuidelines.md).
