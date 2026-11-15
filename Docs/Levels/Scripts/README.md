# Reusable Unreal Editor level tools

This document describes **general-purpose, Editor-only authoring tools** for Unreal Engine 5.8. No scripts are implemented yet. The tools should work with any level in the project; HomeFarm is only the first example layout to configure.

The shared tools must not hard-code `/Game/Maps/L_Farm`, farm actor names, an 8 by 8 grid, a bed, a caretaker, or farm-only zones. Those belong in an optional per-level profile or layout file. A level without those actors must still be surveyable and editable.

HomeFarm's example layout is in [HomeFarm/README.md](../HomeFarm/README.md), with the diagram in [HomeFarm/HomeFarm.svg](../HomeFarm/HomeFarm.svg). It can supply one configuration after its Editor measurements are known.

## Reusable design

Separate the implementation into:

- **Shared tool core:** survey the chosen level, read and validate layout data, create or update generated blockout actors, and report geometric warnings.
- **Per-level configuration:** target map path, anchor references, zones, routes, units, optional gameplay-specific measurements, and no-place/clearance areas.
- **Optional level-specific checks:** added only when a level has a genuine special rule. A missing optional feature is reported as unavailable, not treated as an error for every other map.

Keep the data format versioned and the level's records separate. Shared logic should need no code changes when a new level is added.

## Recommended implementation order

1. Build the read-only survey/report tool and verify it on two structurally different levels.
2. Define and validate a generic per-level layout schema; add HomeFarm as its first profile.
3. Generate simple placeholder actors from that data in an explicitly chosen level.
4. Add an Editor Utility Widget only if repeated edits justify an interactive interface.
5. Add generic route/overlap warnings after the schema and generator are stable.

The survey and basic generator are the useful minimum. Do not implement every option at once.

## 1. Level survey and measurement report

**Purpose:** give any level a reliable inventory of its current actors, bounds, and usable reference points before editing.

**Input:** an explicitly chosen level asset or the current Editor world. Record the level asset path in the report. The survey should work when optional actors or components are absent.

**Generic data to collect where available:**

- Level path, world bounds, and whether the report covers the persistent level only or loaded sublevels as well.
- Each actor's label/name, class, tags, transform, and world-space bounds; optionally summarize its components and their bounds.
- Recognizable generic anchors such as Player Starts, cameras, volumes, lights, and mesh actors, using configurable class/tag filters rather than assumptions about one game level.
- Pawn dimensions or camera information only when the user selects a pawn or supplies a class/actor reference.

**Optional per-level measurements:** a profile may provide extra read-only measurements such as HomeFarm's grid dimensions and tile pitch. Keep that adapter separate from the generic survey, and report a measurement as unknown when it is unavailable.

**Output:** a human-readable report and a JSON file in a level-specific folder such as `Docs/Levels/<LevelName>/`. Include the map path, date, units (Unreal world-space units are centimeters), source of each value, and which records were measured versus manually supplied. Use safe filenames; never overwrite another level's report just because display labels match.

**Safety:** read-only by default. Do not move actors, save the map, or silently select one actor when several match a filter. Report ambiguity and allow the user to choose. A level with no Player Start, camera, or custom profile must still produce a useful generic report.

## 2. Data-driven blockout generator

**Purpose:** create repeatable, easy-to-edit placeholder geometry from a layout, for any selected level.

**Input:** one versioned JSON layout per level, stored separately from shared tool code. The common schema should support:

- An explicit target level path and coordinate anchor, such as world origin or a configured actor/tag reference.
- A declared unit convention: centimeters or a level-specific unit converted using a measured value such as tile pitch `P`.
- Named zone IDs with generic shape type, transform, dimensions, height, material/color, collision choice, and optional labels/tags.
- Optional route polylines, boundary markers, and clear/no-place polygons.
- Optional profile metadata for level-specific data; the generic parser must ignore or report unknown extensions rather than assume farm fields.

**Output:** named blockout actors from reusable primitive meshes, organized in a clearly named generated folder in the selected level. Tag generated actors with a tool-owned identifier and the configuration/zone ID so repeated runs can update the right actors.

**Safety and repeatability:**

- Require an explicit target level and show it before generation. The tool may work on any level, but the user should choose a duplicate when preserving an existing level matters.
- Provide a dry-run that lists planned creates, updates, and skips before changing the Editor world.
- Create or update only actors carrying the tool's own generated tag. Never delete or overwrite unrelated actors.
- Do not save the level automatically. Let the user inspect and save in the Editor.
- Fail clearly on invalid dimensions, missing anchors, duplicate IDs, incompatible layout versions, or unresolved mesh references. Do not silently substitute values.

The shared generator creates blockout/presentation actors only. It must not invent or implement gameplay behavior for a particular level.

## 3. Interactive adjustment tool (optional)

An Editor Utility Widget or Editor Utility Blueprint could provide a level-independent interface over the same layout file:

- Select the current level and its associated layout profile.
- List configured zones and adjust position, footprint, height, and visibility.
- Toggle route, boundary, and placeholder layers.
- Convert displayed dimensions between the level's declared units and centimeters.
- Preview a dry-run, then rebuild only actors carrying the tool's generated tag.
- Save changes back to that level's layout file, not a hidden second copy of the data.

Avoid controls named for HomeFarm concepts. A HomeFarm profile may label its own zones "Field Apron" or "Service Yard", while another level can use different zone names without changing the widget.

## 4. Generic route and clearance checks (optional)

A validator should read generic route lines, zone bounds, clear/no-place areas, and an explicitly supplied pawn/capsule size. It can warn about:

- Routes narrower than the selected pawn diameter plus a configurable clearance margin.
- Route endpoints that do not reach their configured anchors or zones.
- Overlapping zones, paths through configured no-place regions, or blockout geometry outside the declared level boundary.
- Missing required anchors that the current level profile explicitly declares.

It must not require a field apron, house, service yard, caretaker spot, or any other HomeFarm-only zone. Add farm-specific checks as an optional HomeFarm profile extension, not as unconditional shared rules.

These are geometric warnings, not proof that a level plays well. Collision, navigation, camera sightlines, interactions, and traversal still need a Play-in-Editor check.

## Unreal Editor requirements and cross-level validation

Unreal Python is an Editor scripting option, not something available automatically in every project setup. Before implementing it, check whether the Python Editor Script Plugin and required Editor scripting support are enabled in the installed Unreal Engine 5.8 editor. Do not enable plugins or change project configuration incidentally. If Python is unavailable or undesirable, use an Editor Utility Widget/Blueprint or perform the initial blockout manually.

Before calling the tools general-purpose, verify them on at least two structurally different levels: one with the optional HomeFarm profile and one without a farm grid, bed, or service yard. Confirm that:

- Surveying the second level succeeds and reports absent optional anchors without failing.
- Each level reads and writes only its own layout/report files.
- The generator creates and updates only its tagged actors in the explicitly selected map.
- A dry-run makes no changes, and generation never saves a map automatically.
- Generic route checks use only data supplied by that level's configuration.

After generation, inspect the Outliner and viewport, save manually, then Play the target level and test its configured routes. No generated level should be treated as approved merely because a script completed.
