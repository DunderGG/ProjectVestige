# Content

This folder contains the Unreal Engine assets used by Project Vestige: maps, Blueprints, meshes, materials, animations, input assets, and data assets. Unreal stores these as `.uasset` and `.umap` packages and tracks references between them, so create, move, rename, and delete assets through the Unreal Editor rather than managing the package files directly.

## Folders

| Folder | Purpose |
|---|---|
| `Vestige/` | Project-authored game assets, organized by type (see below). New Vestige assets belong here. |
| `Maps/` | Game maps. `L_Farm` is the main playable level. |
| `Input/` | Enhanced Input actions and mapping contexts, plus touch-interface assets. |
| `Characters/` | Unreal mannequin meshes, materials, rigs, and animation assets used by the project. |
| `ThirdPerson/` | Blueprint game mode and player controller retained from the Unreal Third Person template. |
| `Collections/` | Unreal Editor asset collections; currently empty. |
| `Developers/` | Editor-only, per-user content; currently empty. Do not put shared game assets here. |

### `Vestige/`

| Folder | Purpose |
|---|---|
| `Abilities/` | Ability-set data assets that group abilities and attribute sets. |
| `Blueprints/` | Project Blueprint classes, including the player character, player state, farm tile, and bed. |
| `Crops/` | Crop definition data assets scanned by the Asset Manager. |
| `Items/` | Item definition data assets, including seeds and harvested crops. |
| `Materials/` | Project materials and material instances. |
| `Meshes/` | Project static meshes, including crop stages, tilled soil, and the watering can. |
| `Tools/` | Tool definition data assets scanned by the Asset Manager. |

Asset naming conventions and instructions for adding crops, tools, and items are in [Content and Asset Naming](../Docs/CodingGuidelines.md#142-content-and-asset-naming) and [Adding Content the Data-Driven Way](../CONTRIBUTING.md#7-adding-content-the-data-driven-way).
