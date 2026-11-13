# 22. Farm fields and tiles

One of the [system guides](../Systems.md); read [How to use this document](../Systems.md#how-to-use-this-document) first.

**Status:** the data is built: `UFarmFieldSubsystem` keeps one `UFarmField` per field ID, each a row-by-row array of `FFarmTileState` with the farming rules (2026-11-04, [CodeReview.md, C1](../CodeReview.md#c1--all-game-state-lives-in-the-level-but-the-game-needs-several-maps)). Drawing with instanced meshes is built as designed below (2026-11-05, [Roadmap.md, 4.0](../Roadmap.md#40-structure-from-the-code-review), [CodeReview.md, C2](../CodeReview.md#c2--one-actor-per-tile-each-with-two-meshes-and-two-dynamic-materials)), approved by David on 2026-11-05. A crop's growth stages spread over its days to mature, a small size step each night it grew, and a ripe mesh of its own are built for the first crops of [4.2](../Roadmap.md#42-farming-depth) (2026-11-13, below). Water and radiation per tile ([Roadmap.md, 4.2](../Roadmap.md#42-farming-depth)) and placing objects on the grid ([section 16](16-PlacingAndBuilding.md)) build on it.

**References:**

- Epic's "Instanced Static Mesh Component" page and the `UInstancedStaticMeshComponent` class reference: many copies of one mesh drawn as one primitive, each instance with its own transform and, optionally, a few floats of per-instance custom data.
- Epic's "Nanite Virtualized Geometry" page: Nanite draws every instance of an instanced component through the GPU scene and culls each instance there. The hierarchical variant (`UHierarchicalInstancedStaticMeshComponent`) adds CPU-side culling and LODs for meshes without Nanite, so Nanite art doesn't need it.
- The material nodes **PerInstanceCustomData** and **PerInstanceCustomData3Vector**: a material reads an instance's custom data floats, which replaces a dynamic material per object. A material drawn on instances needs **Used with Instanced Static Meshes** ticked and saved; UE 5.8 doesn't set missing usage flags during play, and such a material draws the grey default material instead.
- The engine source, `Components/InstancedStaticMeshComponent.h` in 5.8: `AddInstanceById`, `RemoveInstanceById`, `UpdateInstanceTransformById` and `SetCustomDataById` name an instance by an `FPrimitiveInstanceId`, which stays the same when other instances are removed. An instance's index doesn't: removing one moves the others.
- Lyra has nothing like a tile grid. We take only its general rule that data and presentation are separate objects.

**How the parts fit:**

| Part | Kind | Owns |
| --- | --- | --- |
| `UFarmField` + `FFarmTileState` | Object in `UFarmFieldSubsystem`, struct per tile | The tiles as data, the rules, and the events `OnTileChanged` (one tile) and `OnTilesChanged` (many at once) |
| `AFarmGrid` | Actor placed in the level, as `BP_FarmGrid` | Where its field lies (the actor's corner, `TileSize`), finding its field by `FieldId`, converting between world space and coordinates, a tile's transform in the world (`GetTileTransform`), `OnFieldReady` once it has its field, resizing for developer tools. The interactable for its tiles: offering Harvest while the interactor faces a ripe tile, from `InteractionVolume` over the field and one tile around ([section 7](07-Interaction.md), harvesting with F). No tile actors and no drawing |
| `UFarmFieldMeshesComponent` | Scene component on the grid | Drawing the grid's field: one instanced static mesh component per instance set, one soil instance and at most one crop instance per tile, kept up to date from the field's events. Presentation only |
| `FarmTileLooks::GetLook` | Pure function (`FarmTileLooks.h`) | What one tile looks like: for its soil and its crop, the mesh, material override, transform in the grid and colour, from the tile's state and the meshes component's settings. Tested without a world |

**Rules:**

- **Tiles are data, not actors.** Nothing spawns an actor or creates a `UObject` per tile. Game code names a tile by its field and its coordinates (column X, row Y) and asks the field to change it. Something on the grid becomes an actor only when it needs to be one, such as a placed sprinkler with behaviour ([section 16](16-PlacingAndBuilding.md)).
- **One instanced static mesh component per instance set.** A set is one mesh with one material override (none means the mesh's own materials): grass, tilled soil, watered soil, each crop growth stage mesh, and the placeholders. The meshes component creates a set's component the first time a tile needs it and keeps it until the grid ends; a set's mesh never changes. A tile whose look changes moves its instance to another set.
- **Per-tile numbers are per-instance custom data, never a dynamic material.** Every set has the same custom data layout, so a material reads the same index on every mesh:

  | Index | Holds | Read by |
  | --- | --- | --- |
  | 0 to 2 | Colour (linear RGB) | `M_FarmPlaceholder`, the placeholder shapes' material. The Blender art ignores it |
  | 3 and up | Free, for [4.2](../Roadmap.md#42-farming-depth): the water amount, then radiation | The soil material that 4.2 makes |

  The meshes component sets `NumCustomDataFloats` when it creates a set's component, before it adds an instance.
- **Watered soil is its own set for now.** It is the tilled soil mesh with `M_Soil_Watered`. The Blender soil's materials are instances of the Interchange importer's material and can't read custom data, and in this milestone water is only on or off. When 4.2 makes water an amount, the soil gets a project material that reads it from custom data, and the watered set goes away.
- **Instances are named by ID.** For each tile the meshes component keeps which set and which `FPrimitiveInstanceId` its soil and its crop instance have, in an array in the field's tile order. It never stores an instance index.
- **Updates follow the field's events,** and nothing ticks:
   - `OnTileChanged` (a tool use): only that tile. If a part keeps its set, its transform and custom data are updated in place; otherwise the instance is removed from the old set and added to the new one.
   - `OnTilesChanged` (a new day, a restore, a resize): every set is rebuilt in one pass. The sets are cleared, each set's instances are added in one batch (`AddInstancesById`) with their custom data. The component's instance data manager sends only the changed instances to the renderer, so nothing marks render state dirty. It runs under a `TRACE_CPUPROFILER_EVENT_SCOPE` and counts against the new day's 100 ms ([Architecture.md, section 10](../Architecture.md#budget)).
- **The set components:** no collision, no overlap events, no effect on navigation, shadows on, ticking off, mobility Movable (the engine's default for instanced components; a static one may not change its instances during play, and an unmoving movable primitive keeps its cached virtual shadow map pages). They are attached to the meshes component with instances in the grid's space, so moving the grid in the editor moves the field.
- **The look settings live on the meshes component and are set in `BP_FarmGrid`:** the tilled soil mesh, the watered soil material, the height crops stand at, the placeholder material, and the placeholder shapes (the engine's cube and cylinder, found with `ConstructorHelpers` as [CodingGuidelines.md, section 12.4](../CodingGuidelines.md#124-assets-and-loading) allows for engine placeholders). Anything left empty falls back to the placeholder. Crop meshes come from each crop's data, loaded with its `Visuals` bundle by the content catalogue ([section 2](02-Content.md)), so drawing never loads an asset. A level places `BP_FarmGrid`, never the C++ `AFarmGrid`, so every map's farm looks the same.
- **A crop's look follows its growth, every night it grew shows, and only a ripe crop looks ripe** ([GameDesign.md, section 6](../GameDesign.md), crops). The crop's data (`UFarmCropData`) owns its look for each growth day; `FarmTileLooks` only asks it (`GetGrowthLook`, a mesh and a scale) and places the answer. A crop has any number of **growth stage meshes** (`GrowthStageMeshes`, seedling first), one **ripe mesh** (`RipeMesh`) and a **stage start scale** (`StageStartScale`, 0.8 by default):
   - From `DaysToMature` watered days on (`IsRipeAt`, the same test `FFarmTileState::IsRipe` and harvesting use) the crop shows its ripe mesh at full size, and never before. A `DaysToMature` below 1 counts as always ripe.
   - Before that, its N growth stage meshes are spread evenly over the D growing days: day d shows stage `d * N / D` (integer division, `GetGrowthStageIndex`), so stage k spans days `ceil(k * D / N)` to `ceil((k + 1) * D / N) - 1`. The first stage shows on the day it is planted and the last on the day before it ripens; with N at most D, every stage shows for at least one day. A 3-day crop with 3 stages shows one per day; a 10-day crop with 4 shows stages 0, 0, 0, 1, 1, 2, 2, 2, 3, 3, then ripe.
   - Within a stage the crop scales evenly, as a whole from its origin at the base, from `StageStartScale` on the stage's first day to 1 on its last: a three-day stage shows 0.8, 0.9, 1.0 and a two-day stage 0.8, 1.0. A one-day stage shows at 1, since its mesh already changes every night. A night without growth changes nothing, since growth days don't advance.
   - Changing a crop's `DaysToMature` spreads its stages again, so tuning a crop's length never needs new art.
   - **The art rule:** each growth stage mesh is at least `1 / StageStartScale` (1.25) times the one before, in height (the bounds' Z size) and in spread (the larger of the bounds' X and Y sizes), so a new stage at its start scale never looks smaller than the end of the one before. The ripe mesh has no size rule (a potato's foliage dies back), only a look clearly unlike the last stage. Every material on a crop mesh has **Used with Instanced Static Meshes** saved.
   - A missing mesh (an empty entry, no growth stage meshes, no ripe mesh, or a mesh not loaded) draws the placeholder cylinder, unscaled: its height already follows the growth fraction (`GrowthDays / DaysToMature` as a float, seedling to ripe height), so it shows every night too, and its colour turns ripe only when the tile is ripe. A shorter list never repeats a mesh in place of another stage.
   - Validation (`UFarmCropData::IsDataValid`): a `DaysToMature` below 1, a `StageStartScale` at or below 0 or above 1, no ripe mesh, no growth stage meshes, an empty entry, or the ripe mesh also in the growth stage list are errors; more growth stage meshes than `DaysToMature` is a warning, since some stages then never show.
   - Crop meshes are named after the crop: `SM_Crop_<Name>_Stage<N>` from `Stage0`, and `SM_Crop_<Name>_Ripe` (`SM_Crop_Turnip_Stage0`), with the origin at the centre of the base on the soil surface, in `Content/Vestige/Meshes`.
- **Gameplay never calls the meshes component.** The field's events reach it directly. Targeting, the target highlight and anything else that needs a tile's place ask the grid for coordinates or a transform.
- **Extension points name tiles by coordinates.** `On Crop Harvested` and `On Tool Used` carry the tile's coordinates (an `FIntPoint`) ([Architecture.md, section 12](../Architecture.md#extension-points-today)).
- **One set of components per field.** Splitting a large field into chunks (for example 32 by 32 tiles each, so culling and rebuilds work on smaller pieces) waits for a profile of a farm-sized field that shows the need.
- **Tiles have no collision.** The farmer walks on the level's floor. If placing objects needs to know what is on a tile, the grid answers by coordinates, not with a shape per tile.

**Tests** (`Source/Vestige/Tests/`):

- `Vestige.FarmCrop.*`: which stage and scale each growth day shows for any stage count and length, the ripe mesh at full size only from `DaysToMature` on, missing meshes giving none, the validation rules, and every base crop showing a mesh for each of its days, its ripe mesh only when ripe, stage meshes that keep the art rule and materials with the instanced usage saved.
- `Vestige.FarmTileLooks.*`: the look of each state with art and with placeholders (grass, tilled, watered, each growth stage, ripe), without a world.
- `Vestige.FarmFieldMeshes.*`: a grid with a field of N tiles draws N soil instances and spawns no actor for them; tilling, watering, planting and harvesting move or add or remove exactly one instance; a new day that grows a crop moves it to its next stage's set; a resize redraws every tile; no `UMaterialInstanceDynamic` is created; `M_FarmPlaceholder` has the instanced usage flag.
- The rule tests cover `FFarmTileState` and `UFarmField`; the farming, targeting and cheat tests name tiles by coordinates.

**Pitfalls:**

- Storing instance indices. Removing an instance moves others (or swaps the last into its place), so a stored index soon points at another tile's instance. Use the IDs.
- Calling `SetStaticMesh` on a set's component to change a tile's look. That changes every tile in the set; move the instance to another set.
- A material without **Used with Instanced Static Meshes**: grey tiles in play, and a "missing usage flag" line in the log.
- Custom data written before `NumCustomDataFloats` is set is dropped without a warning.
- Marking render state dirty or updating navigation per instance inside a loop over the field.
- The hierarchical instanced component for Nanite art: it costs CPU time for culling the GPU already does.
- A dynamic material per tile "just for the placeholder": the placeholder is what the field looks like until there is art, so it has to scale too.
- Indexing a crop's meshes by growth day, or falling back to the last mesh: a long crop then needs a mesh per day, or shows its ripe look days early. Ask the crop for its look.
- Scaling a crop mesh whose origin isn't at its base: it then sinks into the soil or floats as it grows.
- Putting the ripe mesh at the end of `GrowthStageMeshes`: it then shows as a growing stage. Validation reports it.
