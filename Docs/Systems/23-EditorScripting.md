# 23. Editor scripting

One of the [system guides](../Systems.md); read [How to use this document](../Systems.md#how-to-use-this-document) first.

**Status:** partly built (the generic, read-only survey script and pure-Python helper tests exist; embedded-Editor validation remains pending; roadmap 4.1).

**References:**

- Epic, [Scripting the Unreal Editor Using Python](https://dev.epicgames.com/documentation/en-us/unreal-engine/scripting-the-unreal-editor-using-python?application_version=5.8).
- Epic UE 5.8 Python API: [`UnrealEditorSubsystem`](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/UnrealEditorSubsystem?application_version=5.8), [`EditorActorSubsystem`](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/EditorActorSubsystem?application_version=5.8), [`EditorLoadingAndSavingUtils`](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/EditorLoadingAndSavingUtils?application_version=5.8), and [`Actor`](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/Actor?application_version=5.8).
- UE 5.8 engine source: `UnrealEd/Private/Subsystems/UnrealEditorSubsystem.cpp`, `UnrealEd/Private/Subsystems/EditorActorSubsystem.cpp`, `UnrealEd/Private/EditorScriptingHelpers.cpp`, and `Engine/Classes/GameFramework/Actor.h`.
- [Reusable level-tool proposals](../Levels/Scripts/README.md).

The local Lyra source has no comparable generic Editor Python level-survey tool. This design takes no Lyra-specific tooling; it uses the UE Editor scripting APIs directly and skips Editor Utility Widgets, plugins, and runtime C++.

**Rules:**

- Keep the first survey in `Scripts/Editor/SurveyLevel.py`, outside `Source/Vestige/` and `Content/`. It runs only when a developer explicitly executes it inside the Unreal Editor. It is not game code, a runtime subsystem, a build step, or a packaged-game feature.
- Start the Python file with the repository's copyright and SPDX license header, as for other source files.
- Run it from the Editor's **Tools > Execute Python Script** menu after the plugin is enabled; never launch it as system Python or from Editor startup command-line execution.
- The Python Editor Script Plugin is required. Enable it manually for the project and restart the Editor before running the script; the Editor may record this setup in `Vestige.uproject`. Do not silently include that setup change in the tool's implementation, and do not add `Editor Scripting Utilities` or an Editor module. `UnrealEditorSubsystem` and `EditorActorSubsystem` are exposed by `UnrealEd`; the older `EditorLevelLibrary` APIs are deprecated.
- Survey only the currently open Editor world. Obtain it from `UnrealEditorSubsystem.get_editor_world()` and refuse to run if it is unavailable or if `get_game_world()` is non-null. UE 5.8's editor scripting helper rejects editor calls during PIE/Simulate, and `get_game_world()` returns the active PIE world. Never call a map-loading API or switch maps.
- Identify the map with its full package path, not its short asset name. Obtain the path from the Editor world's outermost package and fail without writing if it cannot be obtained. Enumerate actors with `EditorActorSubsystem.get_all_level_actors()`. This is a list of loaded, editable actors eligible for the Outliner in active levels, not a complete inventory of every object in a map. It omits World Settings, the builder brush, transient actors, templates and actors not listed in the Outliner. This first version does not claim support for unloaded sublevels or World Partition cells.
- Survey every actor returned by that API. Do not add selections, filters, farm-specific logic, inferred level boundaries, or special Player Start/camera reports in the first version. A missing kind of actor is not a survey failure.
- Read each actor's object path, class path, tags, world location, rotation and scale. Use object paths rather than labels as per-report record identifiers; labels can be duplicated or renamed. Read labels with `get_actor_label(False)`, because the default getter creates and caches a missing label. Report an empty label as unavailable.
- Use centimeters for world positions and bounds, degrees for pitch/yaw/roll, and dimensionless actor scale. Read world bounds with the Python binding for `GetActorBounds(false, Origin, Extent, false)`: set `only_colliding_components` and `include_from_child_actors` to `False`. This is an axis-aligned box, with half-extents, and is not the actor's exact shape, walkable footprint, or level boundary. Record the method and arguments. Serialize center and full size (`2 * Extent`); use `null` when the returned size is zero on all axes. This treats a genuine point-sized bound as unavailable because the API does not distinguish it from an actor with no primitive bounds. Collision, hidden components, rotation, and the chosen component filter affect what the bounds mean. Verify the embedded Python binding's return shape against UE 5.8 before implementation relies on it.
- Write one human-readable Markdown report and one versioned JSON report to `Saved/LevelReports/<safe-map-name>_<sha256-full-package-path>/`; derive the digest from the exact full package path, so same-named maps in different folders have distinct output directories. Preserve the exact package path in both reports. Set JSON `schema_version` to `1`; later incompatible schema changes increment it. Include `generated_at` as an ISO-8601 local timestamp with UTC offset, `map_package`, `actor_scope` (API, unfiltered scope, and exclusions), `units`, `bounds_method` and its arguments, `dirty_map_status` (`before`, `after`, `changed`), and a deterministically sorted `actors` array. Each actor has `object_path`, nullable `label`, `class_path`, sorted `tags`, `transform` (`location_cm`, `rotation_degrees` with pitch/yaw/roll, and dimensionless `scale`; each vector has named `x`, `y`, `z` values), and `bounds_cm` (`center_cm` and `full_size_cm`, each with named `x`, `y`, `z`) or `null`. Use `true`, `false`, or `null` for `dirty_map_status.changed` if either query is unknown. Reject non-finite transform or bounds numbers before writing standard JSON; report the actor path and error instead of emitting NaN/Infinity.
  - Give each run a timestamped stem with a numeric suffix on collision. Reserve that stem by creating a reservation file with exclusive-create mode (`open(..., "x")`); never use a check-then-create sequence as the reservation. Check for existing final report or completion-manifest paths both before attempting the reservation and again after acquiring it. If any exists, release the reservation and try the next suffix. This makes concurrent Editor runs choose different stems and prevents overwriting an earlier run.
  - Write the Markdown and JSON contents to temporary files in the destination directory, then flush and close both successfully before publishing either report. Write a temporary completion manifest containing the two final filenames and their SHA-256 hashes. Publish the reports with non-overwriting same-directory renames, then publish the manifest last. Never use an overwriting replace operation. The pair is complete only when its manifest exists and both file hashes match; two file renames are not an atomic pair.
  - On a caught write or publication failure, remove temporary files and any final report files already published by this run, leave no completion manifest, and release the reservation. Never remove a path that existed before this run. If cleanup itself fails, log each remaining path and the failure, and do not claim success. A process termination can leave an unmarked partial output and reservation; treat an existing reservation as a collision, never auto-delete it, and treat report files without a valid completion manifest as incomplete. A successful pair consists of the Markdown report, JSON report, and completion-manifest sidecar.
  - Report file-write errors in the Output Log and do not silently claim success unless both reports and their completion manifest have been published.
- Read dirty-map status with `EditorLoadingAndSavingUtils.get_dirty_map_packages()` before and after the survey. Record each as `true`, `false`, or `unknown`; use `unknown` if the query or package match is inconclusive. Record a before/after change if detected. Never clear dirty state, save the map, or discard the user's edits.
- The only intended persistent side effects are the Markdown and JSON reports plus their completion-manifest sidecar; the reservation and temporary files are removed after success or a caught failure. Do not change actors, components, labels, tags, level state, selection, viewport, or map state. Do not call selection, transform-setting, spawn, destroy, save, or load APIs.
- This is a one-shot tool with work proportional to the enumerated actor count. It has no tick, timer, game save key, Asset Manager registration, feedback tag, runtime log category, or mod extension point. Print a concise success/failure line to the Editor Output Log; include the report paths on success and the exception or cause on failure.
- Keep report output under `Saved/`, which Git ignores. Copy a snapshot into `Docs/Levels/` only deliberately and after review.

**Pitfalls:**

- Python is an Experimental Editor feature in UE 5.8. It is unavailable in PIE, Standalone, cooked, and Shipping builds; it is not a gameplay scripting language.
- `EditorActorSubsystem.get_all_level_actors()` does not mean every actor serialized in the map. Reports state its scope rather than calling themselves complete.
- A label getter can dirty a map by caching a default label. Always pass `False`; do not “fix” empty labels.
- Zero actor bounds do not establish a meaningful zero-size object. Do not present an unavailable bound as a real point-sized actor.
- A generated report records what the Editor API exposed at that time. It does not prove a route is playable, a camera view is correct, a map is clean, or an unloaded actor does not exist.
- The survey does not read or write project game saves. A dirty-map query only reports Editor package status; it is not a rollback mechanism or proof that no other Editor system changed the map.

**Tests to write:**

- `Vestige.LevelSurvey.UsesOnlyTheOpenEditorWorld`
- `Vestige.LevelSurvey.RefusesPlayAndSimulateWorlds`
- `Vestige.LevelSurvey.FailsClosedWithoutMapIdentity`
- `Vestige.LevelSurvey.SurveysAnEmptyMapWithoutFarmAssumptions`
- `Vestige.LevelSurvey.ReadsLabelsWithoutCreatingThem`
- `Vestige.LevelSurvey.KeepsDuplicateLabelsAsDistinctActors`
- `Vestige.LevelSurvey.ReportsLoadedActorScopeAndUnknownDirtyStateHonestly`
- `Vestige.LevelSurvey.SerializesBoundsAsCenterAndFullSizeOrNull`
- `Vestige.LevelSurvey.RejectsNonFiniteMeasurements`
- `Vestige.LevelSurvey.SortingAndOutputPathsUseFullMapIdentity`
- `Vestige.LevelSurvey.WritesBothReportsWithoutOverwriting`
- `Vestige.LevelSurvey.ReservesStemExclusivelyOnCollision`
- `Vestige.LevelSurvey.CleansUpWhenSecondReportWriteFails`
- `Vestige.LevelSurvey.CleansUpWhenSecondReportPublishFails`
- `Vestige.LevelSurvey.PublishesCompletionManifestLast`
- `Vestige.LevelSurvey.ReportsFileWriteFailures`
- `Vestige.LevelSurvey.DoesNotChangeMapDirtyStateOrSelection`

Use embedded-Editor Python unit tests for the script's serialization and path helpers, plus Editor integration checks on `L_Farm` and an empty or structurally different map. This tool changes no game rules, so it adds no CQTest or C++ dependency.

Add manual checks to [PlayTest.md](../PlayTest.md) with the implementation: run on two maps, inspect the JSON and Markdown, compare selection and dirty status before and after, verify an empty label stays empty, and confirm two maps with the same short name cannot overwrite each other's output. A person should also confirm the report's bounds are useful for the level work; bounds cannot establish camera framing, lighting, or playability.
