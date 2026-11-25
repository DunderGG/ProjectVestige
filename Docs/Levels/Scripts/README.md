# Reusable Unreal Editor level tools

The first **Editor-only authoring tool**, [SurveyLevel.py](../../../EditorScripts/SurveyLevel.py), is implemented with passing helper tests; its in-Editor validation is still pending. The other tools below remain proposals. The tools should work with any level in the project without assuming that it is a farm. HomeFarm is only the first example: its [plan](../HomeFarm/README.md) and [diagram](../HomeFarm/HomeFarm.svg) are not measurements of the existing map.

The first tool's technical design is now in [Systems.md, section 23](../../Systems/23-EditorScripting.md). This document remains the proposal for later tools and optional extensions; do not treat those ideas as part of the survey's first implementation.

The shared tools must not hard-code `/Game/Maps/L_Farm`, farm actor names, an 8 by 8 grid, a bed, a caretaker, or farm-only zones. Those belong in an optional per-level profile or layout. A level without them must still be surveyable and editable.

## Are these tools worth building?

| Idea | Assessment | When to build it |
|---|---|---|
| **Level survey** | Useful for recording positions and dimensions consistently, especially when comparing iterations. For a handful of actors, the Editor's Details panel may be quicker. | First, if measurements need to be repeated. Start read-only and keep the output honest about what was actually inspected. |
| **Blockout generator** | Useful if the same collection of shapes will be rebuilt or moved many times. For one small layout, entering JSON and maintaining a generator can take longer than placing shapes by hand. | Only after a manual blockout or survey shows that repeatable generation would save work. Start with boxes, not every possible shape. |
| **Adjustment widget** | A friendly interface could help frequent edits, but Unreal's viewport already moves and scales actors. Keeping a widget, actors, and a layout file synchronized is additional work. | Defer until editing the layout file and rebuilding is demonstrably awkward. |
| **Route and clearance checks** | Useful for finding mistakes in explicitly described routes and obstacles, but cannot prove a route is playable. Setting up accurate input data can cost more than walking a small level. | Add specific checks only when repeated use justifies them. Play-testing remains necessary. |

**Recommendation:** lay out the home farm by hand in `L_Farm`, with the map committed first so Git can restore it. Do not build all four tools just to place the first few shapes. A small, reusable survey is the best first automation; a generator is a separate, optional investment.

**Unreal terms:** an *actor* is an object placed in a level; its *components* can draw a mesh or provide collision. A *static mesh* is a reusable geometry asset. A map has a persistent level, while the Editor's *current level* is where it places new actors. *Play-in-Editor* (PIE) runs a game session for testing. A *dry-run* describes proposed changes without making them; an Editor *transaction* groups supported edits for Undo.

## Scope and suggested order

1. Duplicate the map, inspect a few important actors and dimensions by hand, and note what is tedious or easy to misread.
2. If repetition warrants it, build the read-only survey. Try it on HomeFarm and a structurally different level without farm-specific actors.
3. If repeated blockout edits warrant a generator, implement and test a small layout format and generator together. The example below is a starting point, not a finished schema.
4. Add only the route checks or interface that solve a demonstrated problem.

The common core should not require code changes for each new level. Level-specific measurements belong in optional adapters or explicitly supplied data, not in the generic survey or generator.

The project currently uses separately opened maps, not streamed sublevels or World Partition ([architecture](../../Architecture.md#2-unreals-gameplay-framework-where-things-go)). First support the **currently open Editor map**. Never claim a loaded-actor report covers unloaded sublevels or World Partition cells; editing those map types needs a separately designed and tested scope. Follow [AGENTS.md](../../../AGENTS.md) and the system guides when a tool is actually designed and implemented.

## 1. Level survey and measurement report

**Purpose:** inventory the currently open Editor level and record useful measurements without changing the map. A survey reports what exists; it does not design the level.

**Version 1 input (defined by [Systems.md, section 23](../../Systems/23-EditorScripting.md), which supersedes this proposal for the first implementation):** only the map the user has opened. Selection, class, and tag filters are deferred future extensions, not v1 inputs or requirements. Record the full map package path, such as `/Game/Maps/L_Farm`, rather than using a short display name as its identity. Do not load another map automatically: UE 5.8's `LevelEditorSubsystem.load_level()` closes the current level without saving it. Do not run against a Play-in-Editor or Simulate world.

**Report at least:**

- Map package path, date and time, whether the map has unsaved changes, measurement units, and the precise actor scope and exclusions. Version 1 surveys the unfiltered actor list from `EditorActorSubsystem.get_all_level_actors()`; it has no selection or class/tag filters.
- For each actor in that scope: its object path, label, class, tags, world transform, and meaningful world-space bounds where available. Labels can be duplicated or changed; do not use them alone as unique IDs.
- Special reports for Player Starts, cameras, volumes, lights, or mesh actors are deferred proposals, not part of version 1; section 23 supersedes this suggestion for the first survey.
- Future proposal, not v1: Pawn capsule dimensions or camera settings for an explicitly selected actor or class, with their source. A Player Start is a spawn point, not the actual gameplay camera. Blueprint overrides and runtime changes can differ from C++ defaults; check the opening view in Play.

**What the numbers mean:**

- Unreal world coordinates conventionally use centimeters. State whether each value was read from an Editor property, calculated from other values, or supplied manually; report unavailable values as unknown.
- Actor bounds are an **axis-aligned world-space box**, not the shape or walkable footprint of the actor. Unreal's `GetActorBounds` returns a center and **half-extents**; multiply each half-extent by two for the full size. Rotation can make the box larger than the object's footprint, and collision or hidden components can contribute. An actor without meaningful registered primitive bounds should be reported as having no bounds, not as a real zero-sized object.
- A combined box around every actor is not automatically a level boundary: sky, landscape, lights, or other large actors can dominate it. Use a boundary supplied by the level designer if one is needed.
- `EditorActorSubsystem.get_all_level_actors()` returns loaded, editable actors in active levels that are eligible to be listed in the Outliner, not every object that could exist in a map. In UE 5.8 it also excludes, for example, World Settings, the builder brush, transient actors, and actors not listed in the Outliner. State these limits in the report; do not silently call it a complete inventory.
- Even a getter can have side effects: in UE 5.8, `actor.get_actor_label()` creates and caches a missing label by default. Use `actor.get_actor_label(False)` for a read-only survey and report an empty label as unavailable instead.

**HomeFarm example, not shared logic:** [`AFarmGrid`](../../../Source/Vestige/Farming/FarmGrid.h) stores editable width, height, and tile size; the placed Blueprint may override the C++ defaults. Its field data and tile meshes are initialized at `BeginPlay`, so an Editor survey must not expect one actor per tile or assume the runtime field already exists. The grid's overall actor bounds include its interaction volume and are not the field footprint. A future, separately designed HomeFarm diagnostic could use the grid's settings and transform, or explicitly supplied actor bounds, to calculate the footprint and neighboring tile-center spacing; it would report the inputs and method. Account for rotation and scale: non-uniform scale can give different X and Y spacings, so a single `P` is not always valid. The grid's C++ `GetTileTransform` is not a reflected `UFUNCTION`, so an Unreal Python script cannot assume it can call that method.

**Output and safety:** write a human-readable report and versioned JSON under a level-specific location, for example `Saved/LevelReports/`, which is ignored by Git. The JSON should contain the report metadata above and an actor array, with location in centimeters, named pitch/yaw/roll angles in degrees, dimensionless scale, and either bounds with center and full size in centimeters or `null` when unavailable. Record the bounds method and component/collision filter. `EditorLoadingAndSavingUtils.get_dirty_map_packages()` can help identify unsaved map packages; report the status as unknown if it cannot be established.

Derive a safe, distinct directory from the **full** map path, not just the level's name; do not overwrite an existing report without an explicit choice. Version 1 publishes the Markdown/JSON pair with an exclusive stem reservation and a completion manifest; the manifest is the signal that both files form a complete report, not a claim that two renames are atomic. A useful snapshot can be copied into `Docs/Levels/` deliberately. Sort records consistently for comparison and make file-write failures visible. Writing the report is allowed; moving actors, changing selection, dirtying or saving the map is not.

## 2. Data-driven blockout generator

**Purpose:** if repeated edits justify it, create simple placeholder actors from a level's layout. The data is the source of truth for the fields the generator manages; a manual move or resize of a generated actor will otherwise be overwritten on the next run. Do not pretend there is two-way synchronization.

Keep each reviewed layout as source-controlled JSON in its own level documentation folder, separate from the shared tool code and generated reports. Use distinct folders for maps with the same short name, and verify the map package path inside the layout instead of guessing a profile from a label. The generator reads the file; it does not rewrite it.

### A small first layout

This fictional example illustrates the minimum to define and validate before implementation. `/Game/Maps/L_Example` is not an existing project map or HomeFarm measurement.

```json
{
  "schema_version": 1,
  "layout_id": "Example",
  "map_package": "/Game/Maps/L_Example",
  "unit": "cm",
  "anchor": "world_origin",
  "zones": [
    {
      "id": "MarkerA",
      "shape": "box",
      "center": [0, 0, 50],
      "size": [400, 300, 100],
      "yaw_degrees": 0,
      "collision_profile": "NoCollision"
    }
  ]
}
```

For this proposed first slice, all shown fields are required. The center is a world-space `[X, Y, Z]` in centimeters; size is the **full** local X/Y/Z size before the yaw rotation, also in centimeters; yaw is in degrees about Z. The example is a 4 m by 3 m by 1 m **non-blocking marker**, with its bottom at Z = 0, not a walkable floor.

Use the existing `/Engine/BasicShapes/Cube.Cube` mesh. With no parent or component-relative transform, calculate each scale component as the requested size divided by the mesh's full local bounds size; reject a zero-sized mesh bound. To put the **bounds center**, rather than an arbitrary mesh pivot, at the requested center, subtract the yaw-rotated, scaled local bounds center from the requested center to obtain the actor location. Actor scale itself is not a size in centimeters. Check a rotated box as well as an unrotated one.

Validate the entire input before changing the world: valid JSON and field types, supported version, exact target map, known keys and shape, finite numbers, positive dimensions, a resolvable collision profile, and nonempty IDs unique within the layout even when case differs. Reject duplicate JSON keys and misspelled or unknown core fields rather than silently ignoring them. Future optional extension metadata can be ignored **with a report** only when it is explicitly nonessential and does not affect safety or geometry; an unsupported required feature is an error.

Actor-relative anchors, non-centimeter units such as measured tile pitch, more shapes, materials, route polylines, and clearance polygons are later extensions, not promises of this first slice. Specify their coordinate transforms and validation before adding them. If a level uses tile pitch, measure it first and do not invent a value when it is unavailable.

### Safe generation and updates

- Require the target map to be open and show its full package path. Refuse to run if it differs from the layout, if Play or Simulate is active, or if the intended level is not the Editor's current level. Unreal's actor-spawn API creates actors in the **current level**, which may not be the persistent level. Do not silently switch or save levels.
- Preflight the complete layout, required assets, and existing generated actors; then show a **dry-run** listing creates, updates and their changed properties, unchanged actors, and stale or ambiguous records. A dry-run must not mutate the map, alter selection, or save anything. Apply only after the user explicitly confirms. Actual actor spawning selects the new actor in UE 5.8; preserve or clearly report the selection change.
- Identify each generated actor by tool-owned metadata containing the map, layout ID, and zone ID; put it in a clearly named Outliner folder for readability. A folder or actor label is not ownership. Actor tags are editable and are copied when actors or maps are duplicated, so reject duplicate or conflicting identities, unexpected actor classes, and copied metadata from another map instead of guessing or adopting an untagged actor.
- On a confirmed run, update only the listed tool-managed fields of matched generated actors. Never update unrelated actors. If a zone is removed or renamed, report its old actor as stale and leave it alone; do not automatically delete it. A duplicate map containing generated actors needs an explicit resolution of their old map identity.
- Group changes into an Editor transaction and verify Undo/Redo for both spawned actors and modified properties. A transaction is not an automatic rollback: in UE 5.8, cancelling a `ScopedEditorTransaction` discards the transaction, not the already made changes. If a later operation fails, stop, report what changed and leave the map unsaved so the user can inspect and use a verified Undo or another recovery method. Preflight cannot prevent every failure.
- The tool script is Editor-only, but its generated actors are ordinary level actors unless explicitly configured otherwise. Outliner folders and tags do **not** prevent them from appearing or colliding in Play or a packaged build. Collision affects traversal even though the generator adds no gameplay logic. Use the specified collision profile and a mesh with suitable collision; `NoCollision` cannot provide a floor. Keep the initial blockout small; hundreds of repeated pieces need the project's [instancing and performance rules](../../Architecture.md#design-rules), not an unchecked actor per piece. Remove or replace temporary blockout before release as appropriate.
- Start with the cube's existing material. Arbitrary colors require a suitable material or parameter; setting a color in JSON does not make every Unreal material change color. Outliner labels are not in-world signs. Add visual labels or color support only with an explicit design and assets.

## 3. Interactive adjustment tool (optional)

An Editor Utility Widget could later select a level's layout, edit zone dimensions, preview the planned changes, and invoke the same generator. Keep it independent of HomeFarm names.

The hard part is not the controls: decide whether the file or the actors are authoritative, how viewport edits are detected and saved back, what happens to unsaved changes, and how Undo works across the file and map. Until that is designed, edit the layout file and regenerate, or use ordinary Editor tools for a manual blockout. Do not create a hidden second copy of the layout.

## 4. Generic route and clearance checks (optional)

A polyline is only a centerline: it has **no width** and does not describe obstacles. A useful check needs explicitly supplied route width, a pawn's scaled capsule radius, a clearance margin, and the relevant obstacle or no-place polygons. If any needed input is missing, report the check as unavailable, not passed.

- Define the margin **per side**. For one pawn, the minimum declared route width is `2 * (capsule radius + margin)`. For example, radius 42 cm and margin 20 cm per side require 124 cm: a 123 cm route warns, while 124 cm meets this numerical check. This says nothing about whether the surrounding level really has that clearance.
- Check the declared route corridor, not just its centerline, against explicitly identified no-place areas and the declared boundary. Define endpoints by anchor or zone ID and an explicit distance tolerance. Reject invalid or self-intersecting polygons, invalid widths, and missing required references rather than inventing geometry.
- Zone overlap is not automatically a mistake: a route may intentionally cross a floor or enter a destination. Only warn for combinations the layout explicitly declares incompatible. Do not treat axis-aligned actor bounds as exact rotated shapes.
- State that the first checks are a 2D plan on the specified floor. They do not prove vertical clearance, floor support, collision responses, slopes, navigation, camera sightlines, interaction reach, or movement around live actors. Check those in Play; a navigation mesh, if present, is useful evidence but not a substitute for playing.

Farm-specific checks remain optional HomeFarm extensions. The shared validator must not require a field apron, house, service yard, or any other farm-only zone.

## Unreal Editor setup

Unreal Python runs **inside the Editor**, not in a packaged game or an ordinary system Python process. The project file does not explicitly enable the Python Editor Script Plugin, and the installed UE 5.8 plugin is disabled by default; check **Edit > Plugins > Scripting** before running the survey. Enabling it and restarting the Editor is a project setup change, not something a survey should do silently. Once the plugin is enabled, use **Tools > Execute Python Script** and select [SurveyLevel.py](../../../EditorScripts/SurveyLevel.py).

For actor work, prefer `unreal.EditorActorSubsystem`; old `EditorLevelLibrary` examples are marked deprecated in UE 5.8. Enable other scripting plugins only if the chosen APIs need them. See Epic's [Python Editor scripting guide](https://dev.epicgames.com/documentation/en-us/unreal-engine/scripting-the-unreal-editor-using-python?application_version=5.8), [EditorActorSubsystem API](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/EditorActorSubsystem?application_version=5.8), and [ScopedEditorTransaction API](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/ScopedEditorTransaction?application_version=5.8).

## Acceptance checks for a future implementation

Use automated tests for parsing and geometry, and Editor integration tests on dedicated test maps for changes to actors. Run Editor/build processes one at a time. At minimum:

- Survey two different maps, one without farm features. Missing optional actors remain unknown; duplicate labels do not merge records, and reading an empty label does not create one. Confirm bounds use full sizes correctly, report the actual loaded-actor scope, and do not alter map dirty state or selection.
- Prove that wrong-map, active Play/Simulate, invalid data, missing assets, duplicate IDs, and duplicate or foreign ownership metadata stop generation before any edit. A dry-run also changes nothing.
- Generate twice with identical data: actor identities, counts, transforms, and collision stay the same. Change one zone and confirm unrelated actors stay unchanged. Remove a zone and confirm its stale actor is reported, not deleted.
- Verify Undo and Redo for a real generation and for a simulated partial failure; inspect what remains after failure. Confirm no run saves the map and report files from two maps with the same short name cannot overwrite each other.
- For each route check, test missing inputs, the 123/124 cm boundary example, a corridor crossing a no-place area, and intentional overlaps that must not warn. Confirm an unavailable check is not reported as a pass.

After a real generation, inspect the Outliner and viewport, save only when satisfied, then Play the target map and test its intended routes. Script completion is not level approval.
