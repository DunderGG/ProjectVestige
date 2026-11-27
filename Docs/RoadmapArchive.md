# Project Vestige roadmap archive

Finished roadmap items, moved from [Roadmap.md](Roadmap.md) under the same headings, in the order they stood there. Open items stay in `Roadmap.md`, which keeps every heading, so links to a section of it still work; a link in an item below that names only a section (`Roadmap.md#42-farming-depth`) points to the roadmap's section, where the open work is. Agents don't need to read this file; search it when a task asks what was done before.

## 2. Milestone 1: playable prototype (done)

### Project setup

- [x] Unreal Engine 5.7 C++ project from the Third Person template
- [x] Git with Git LFS for binary files, GitHub repository (public since October 2026)
- [x] Test level `L_Farm` with a farm grid placed in it
- [x] Disabled the VisualStudioTools plugin, which was missing from the engine and broke the build

### Core farming loop (C++)

- [x] Farm tiles with three states: grass, tilled, planted
- [x] 8 by 8 farm grid that spawns its tiles at play start
- [x] Four tools: hoe, seeds, watering can, hand (keys 1 to 4, E to use)
- [x] Watered crops grow one stage per day and can be harvested when ripe
- [x] Seed count and harvest count
- [x] Highlight on the tile the player is facing
- [x] Sleep with N to start a new day

### First art

- [x] Blender script that builds the first meshes: tilled soil, four turnip growth stages and a watering can
- [x] Meshes imported into Unreal and used by tiles and crops
- [x] The current tool is shown in the farmer's hand
- [x] Early Blender assets brought into the game

### HUD, day and night, saving

- [x] On-screen HUD: day, clock, current tool, seeds, harvest count, key hints and short messages
- [x] Day and night clock from 06:00, sun and sky light follow the time
- [x] 24-hour clock display
- [x] Properly dark nights
- [x] Day cycle speed tuned
- [x] Pass out at 02:00 (replaced on 2026-11-02 by the late-night rules, see [stamina and late nights](Roadmap.md#43-stamina-and-late-nights))
- [x] Save at the start of each new day, load on play start, F9 starts a new game

### Clean-up after the prototype

- [x] Clock, lighting and night exposure moved off the grid into their own day cycle actor
- [x] HUD moved into its own HUD class, saving into its own save subsystem
- [x] Farming component trimmed to tools, inventory, targeting, tool use and sleep
- [x] Recovered the level file after a script corrupted it, and fixed the script

### Documents and rules

- [x] Coding guidelines, code formatting rules (`.clang-format`) and instructions for AI agents (`AGENTS.md`, `CLAUDE.md`)
- [x] Architecture document with the ownership table, decision log, known gaps and expected changes
- [x] Rules for designing the game so it can be modded later
- [x] Contributing guide, and Git LFS for more file types
- [x] Step-by-step setup guide for a new Windows PC (`Docs/Setup.md`)
- [x] Game design document
- [x] Lore document
- [x] Licenses: GPL-3.0-or-later with an Unreal Engine permission for code, CC BY-NC-SA 4.0 for assets, and a credits file
- [x] The Unreal Engine permission stated as part of the code's license terms
- [x] Names of other games removed from the documents and from the Git history
- [x] This roadmap

## 3. Housekeeping

- [x] Play-test the 2026-10-29 code clean-up and the 2026-10-30 source folders in the editor: [PlayTest.md, Pending checks](PlayTest.md#2-pending-checks)
- [x] Play-test the latest changes: the HUD looks the same, sleeping saves the game, stopping and playing again restores the farm, and F9 starts a new game
- [x] Force-push the rewritten Git history to GitHub
- [x] Push the newer local commits (a normal `git push`)
- [x] Delete the local backup branch `backup/before-history-rewrite`
- [x] Delete the merged pull request branches on GitHub (`step2-farming-loop`, `step3-asset-script`, `step3-custom-meshes`, `fix-held-tool-shadowing`)
- [x] Delete the unused template code and content (`Variant_*`, `LevelPrototyping`, `Lvl_ThirdPerson` and the unused mannequin assets)
- [x] Remove the leftover redirector `Content/L_Farm` at the content root (right-click > Fix Up Redirectors)
- [x] In the editor, open and save `BP_ThirdPersonGameMode` and `BP_ThirdPersonPlayerController` (Content/ThirdPerson/Blueprints), which still name `/Script/TP_ThirdPerson`, and save `L_Farm`, whose grid and day cycle still name `/Script/FarmSim`. Then the `ActiveClassRedirects` for `TP_ThirdPerson` in `DefaultEngine.ini` can go. Keep the `FarmSim` `[CoreRedirects]`: save files made before the rename name `/Script/FarmSim`
   - Done by @DunderGG (263ebeb). Nothing names `/Script/TP_ThirdPerson` any more, so the redirects were removed from `DefaultEngine.ini`.
- [x] In the editor, move `BP_FarmTile` from `Content/Farm/` to `Content/Farm/Blueprints/` and the `M_` materials from `Content/Farm/Meshes/` to `Content/Farm/Materials/` by dragging them in the Content Browser (choose Move Here), then right-click `Content/Farm` > Fix Up Redirectors. Moving the files outside the editor would break the references to them
- [x] If the Android File Server was ever used with this project, regenerate its security token (Project Settings > Platforms > Android File Server). The old token was removed from `DefaultEngine.ini` but stays in the Git history
   - Not needed: the Android File Server is a tool for copying builds to an Android device over USB, and @DunderGG never used it.
- [x] Rename the farmer Blueprint `BP_ThirdPersonCharacter` to `BP_Farmer` and move it from `Content/ThirdPerson/Blueprints/` to `Content/Farm/Blueprints/`, so our own content lives under `Content/Farm/`. In the editor with the project built: right-click the Blueprint in the Content Browser > Rename, then drag it into `Content/Farm/Blueprints/` (Move Here), then right-click `Content/ThirdPerson` > Fix Up Redirectors. Check the Reference Viewer (right-click > Reference Viewer) before and after: the editor updates the assets that point to it, such as the default pawn class in `BP_ThirdPersonGameMode`, and saves them, so they go in the same commit. In the same commit, change the paths and names the editor can't update:
   - `Source/Vestige/Tests/FarmingTests.cpp` and `Source/Vestige/Tests/VestigePlayerCharacterTests.cpp`, which load `/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter.BP_ThirdPersonCharacter_C` by path (their tests fail without it)
   - the class comment in `Source/Vestige/Player/VestigePlayerCharacter.h`
   - `CONTRIBUTING.md` (trying a crop on the farmer), the `AVestigePlayerCharacter` row in `Docs/Architecture.md` section 13, and the tuning check in `Docs/PlayTest.md` (Smooth turning)
   - Nothing in `Config/` names the character; `DefaultEngine.ini` only names the game mode, which stays where it is
   - Done 2026-10-30: @DunderGG renamed and moved it in the editor (the game mode's default pawn was updated with it, and no redirector is left), and the agent changed the paths above. `LICENSING.md` lists `BP_Farmer` as template material although it now lives under `Content/Farm/`
- [x] Stop loading the tile highlight's material by path in C++: `UFarmTargetHighlightComponent`'s constructor finds `/Game/Vestige/Materials/M_TileHighlight` with `ConstructorHelpers`, which [CodingGuidelines.md, section 12.4](CodingGuidelines.md#124-assets-and-loading) allows only for engine placeholders, and which made the editor refuse to move the folder until the path changed (found in the `Vestige` rename, 2026-11-01). Set `HighlightMaterial` on `BP_PlayerCharacter`'s highlight component instead, keep the engine cube as the C++ default, and keep the test that checks the material's instanced mesh flag
   - Done 2026-11-03: `BP_PlayerCharacter`'s TargetHighlight component sets the material (set by script), and the test now spawns that Blueprint
- [x] Rename the Details panel categories of the general types: the day cycle, rest component, farmer and content data still group their settings under `Farming` (`Farming|Time`, `Farming|Lighting`), and [CodingGuidelines.md, section 6](CodingGuidelines.md#6-uproperty-ufunction-and-blueprint-exposure) still says every category starts with `Farming`. Decide the rule (for example `Time`, `Lighting`, `Input` without a project prefix, and `Farming` only for farm types), then change the code and the guide together. Categories are display text only, so nothing in assets or saves changes. The Blueprint extension points (`On New Day`, Get Day Cycle, Get Content Id) use the same categories, so their place in the Blueprint menus changes with it
   - Done 2026-11-03: categories are named for their topic without a prefix (`Time`, `Lighting`, `Rest`, `Input`...), and only the farm types use `Farming` ([CodingGuidelines.md, section 6](CodingGuidelines.md#6-uproperty-ufunction-and-blueprint-exposure))
- [x] Update the class names in the lore drafts (`Docs/Spoilers/Characters/`): they name `UFarmCharacterData`, which the planned design now calls `UVestigeCharacterData` ([Systems.md](Systems.md))
   - Done 2026-11-03 in `Archivist.md` and `Engineer.md`. Their technical notes also named a `UFarmScheduleComponent`, an `EFarmWeather` enum and a `Content/Narrative/` folder, which the planned design doesn't have: the schedule is now a routine in the character data run by the AI controller and StateTree (Systems.md, section 10), weather is left unnamed until it is designed, and the dialogue tables are under `Content/Vestige/Dialogue/`
- [x] Remove the Print String nodes left in `BP_PlayerCharacter` from the extension point checks ([PlayTest.md](PlayTest.md#2-pending-checks), 2026-11-01): the 2026-11-04 baseline run logged the day number from it (`LogBlueprintUserMessages: [BP_PlayerCharacter_C] 2`) every morning, and it shows on screen in development builds
   - Done 2026-11-05 by @DunderGG, after the Get Day Cycle node they used was removed with the clock move (Architecture.md decision log, 2026-11-04)
- [x] Trim the documents the AI agents read for every task, so a task costs fewer tokens (asked by @DunderGG, 2026-11-11): `Systems.md` is about 59,000 tokens, `Architecture.md` about 49,000 and `PlayTest.md` about 20,000, and each specialist agent in `AGENTS.md`, "How work is done", starts by reading parts of them. Goes with M8's cut of `Architecture.md` above. To decide, through the architect and the reviewer: splitting `Systems.md` into one file per system guide so an agent opens only the guides it needs, moving older decision log entries and finished play-test checks to archive files, and dropping text that restates the code. Measure the token counts before and after
   - Done 2026-11-11 and 2026-11-12 ([Architecture.md, decision log](Architecture.md#16-decision-log), 2026-11-11). Each system guide is its own file in `Docs/Systems/`, and `Systems.md` is their index with the same numbered headings. Passed play-test checks are in `PlayTestArchive.md` and the performance baseline is in `PerformanceBaseline.md`. Replaced decisions are marked where they stand, and the large guides keep their rules and drop retold changes. Sizes, as UTF-8 bytes / 4, before and after: `AGENTS.md` (in every agent's context) 5,700 to 4,100; `Systems.md` 59,000 to an index of 1,100 and 22 guides of 48,600 in all, the largest 7,400; `PlayTest.md` 20,400 to 6,400; `Architecture.md` 49,100 to 47,400, with section 13 7,400 to 3,400 and section 10 2,700 smaller
- [x] Fix the rest of the stale steps in `CONTRIBUTING.md` *(needs @DunderGG's yes: the file is on the maintainer's list)*: "Adding a tool" (line 159) names the farming component's **Toolbar Ids**, which went with the hotbar (a tool is now an item with a Vestige Tool Fragment in the starting items), and line 163 gives a seed item only its Farm Seed Fragment, without the Vestige Tool Fragment that the crop steps above say it needs, and line 94 still says the decision log is in `Docs/Architecture.md` (it is `Docs/DecisionLog.md` since 2026-11-12)
   - Done 2026-11-12, with @DunderGG's yes: a tool is held as an item with a Vestige Tool Fragment, a seed item names the Seeds tool, and pull requests add their decision log entry to `Docs/DecisionLog.md`
- [x] `CONTRIBUTING.md`, "Adding a crop", step 1 *(needs @DunderGG's yes: the file is on the maintainer's list)*: after importing the meshes, set each material slot to an `MI_Crop_<Crop><Part>` instance of `M_Crop` and remove the simple collision (Static Mesh editor, **Collision > Remove Collision**), as [Systems.md, section 22](Systems/22-FarmFields.md) now requires; a plain editor import gives the importer's own material instances and a convex hull, and no test catches either (code review, 2026-11-13)
   - Done 2026-11-13, @DunderGG's yes: step 1 now says to use `MI_Crop_*` instances of `M_Crop` and remove the importer's collision
- [x] Add a row to the path table in `LICENSING.md` for `EditorScripts/*.py` and `EditorScripts/Tests/*.py` (GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception, naming the AI tool that wrote them), then tell `EditorScripts/README.md` to check that table for a new script, as `Tools/README.md` does; needs @DunderGG's yes, since it is a licensing record (found in the code review of the folder move, 2026-11-20)
   - Done 2026-11-20 with @DunderGG's yes: the row is in the path table, `EditorScripts/README.md` points to it, and the open question on AI-written material names `EditorScripts/` and `MI_Crop_Withered`
- [x] Make `Tools/Verify-Vestige.ps1` find the engine instead of hard-coding `C:\Program Files\Epic Games\UE_5.8`: on @DunderGG's PC it is installed under `C:\Users\Dunder\Epic Games\UE_5.8`, so every session has to run a copy with the path changed. For example an `-EngineRoot` parameter, then the `UE_ROOT` environment variable, then the launcher's install list (found 2026-11-22)
   - Done 2026-11-24: the script tries `-EngineRoot`, `UE_ROOT`, the launcher's install list, the engine list in the registry (`HKCU\SOFTWARE\Epic Games\Unreal Engine\Builds`, which is what finds it on @DunderGG's PC), the launcher's registry key, then the default folder, for the version in `Vestige.uproject`
- [x] Check the open play-test checks against the code before the next play-test: 18 checks under nine headings in [PlayTest.md, section 2](PlayTest.md#2-pending-checks) are still open, the oldest from 2026-10-30, and some describe behaviour that later changes replaced ("Five messages and the message log" expects `UVestigeFeedbackSubsystem::Notify`, which `Report` replaced on 2026-11-08). Update or drop each stale one with the test author
   - Done 2026-11-24: 19 open checks went to 13. Six were dropped because tests now cover them (each archived heading's note names the tests), two headings with only passed checks left moved to [PlayTestArchive.md](PlayTestArchive.md), and the rest were brought in line with the code. None was marked passed
- [x] Fix the flaky `Vestige.Feedback.TheLogShowsAReportsClauses`: its `FReportLogCapture` is a buffered output device (it doesn't override `CanBeUsedOnMultipleThreads`), so its lines arrive only when the redirector flushes, and the test removes it without a flush. Call `GLog->Flush()` before `GLog->RemoveOutputDevice`, as `VestigeLogTests.cpp` does, or share one capture helper between the two files; in a commit of its own (code review, 2026-11-22). A flaky test weakens the check that the whole suite passes before each commit
   - Done 2026-11-25: the test flushes `GLog` before removing its capture

## 4. Milestone 2: vertical slice

### 4.0 Structure from the code review

Quick wins:

- [x] Move the test keys, F9 (new game) and N (skip day), to a `UVestigeCheatManager` with exec functions compiled out of Shipping, reached from the console ([CodeReview.md, H5](CodeReview.md#h5--f9-deletes-both-saves-with-one-key-in-a-shipped-build); moved here from [section 6](Roadmap.md#6-before-a-public-release), and it also ends the F9 screenshot clash in [Open problems](Roadmap.md#open-problems)). A real New game comes with the main menu, behind a confirmation
   - Done 2026-11-04: the console commands `NewGame` and `SkipDay` in `UVestigeCheatManager` (`Player/`), set as the template player controller's cheat class outside Shipping; F9 and N are no longer bound. The New game in the menu is part of "Main menu, settings and save slots" in [section 6](Roadmap.md#6-before-a-public-release)
- [x] Delete save versions 0 to 3: their upgrades and backups, the legacy fields, `EFarmTool` and `UFarmSaveGame`. Restart at version 1 and delete the development saves; keep the two alternating slots and the refusal to load or overwrite a newer save ([H4](CodeReview.md#h4--save-upgrade-code-for-a-game-nobody-else-has-played))
- [x] First performance baseline: a packaged build of `L_Farm` with a few hundred crop meshes, measured on the hardware closest to the target (RTX 2060 class, 1080p High) and recorded in Architecture.md ([H8](CodeReview.md#h8--rendering-is-unreals-maximum-feature-set-unmeasured-on-the-target); moved here from [4.1](Roadmap.md#41-foundations))
   - Done 2026-11-04 on @DunderGG's PC (RTX 4070 Ti Super, about three times the target's GPU; no older hardware at hand): the console command `BenchmarkField 20 Base.Turnip` makes a field of 400 ripe turnips, and a CSV capture over a whole day gives 7.05 ms per frame, render thread bound, with the GPU at 6.0 ms. On the target the frame is estimated over budget. Numbers and method in [Architecture.md, section 10](Architecture.md#measuring)
- [x] Finish the baseline: run `BenchmarkField 8 Base.Turnip` in the same build to see what the tiles cost the render thread (for [C2](CodeReview.md#c2--one-actor-per-tile-each-with-two-meshes-and-two-dynamic-materials)), measure video memory and set the memory limit, find the 266 ms GPU stall two seconds after the new day, repeat the run that was disturbed, measure the shadow depths again now that the sun turns in steps (0.93 ms before) and the clouds now that High caps them (1.37 ms windowed before, 0.31 ms off screen after), and measure on target-class hardware if any becomes available ([H8](CodeReview.md#h8--rendering-is-unreals-maximum-feature-set-unmeasured-on-the-target))
   - Done 2026-11-05 off screen at 1920x1080 High: the frame (6.65 ms) waits on the GPU's occlusion results, not on the render thread's own work (3.3 ms); 400 tiles cost about 0.4 ms of GPU more than 64; the sun's steps cut the shadow depths from 0.92 to 0.68 ms at the real clock; the clouds cost 0.74 ms (the earlier off-screen runs were at 1280x720); the stall didn't come back. Video memory is 2.6 GB, and the budget now sets 4 GB of video memory and 8 GB of memory. Target-class hardware moved to [section 6](Roadmap.md#6-before-a-public-release). Numbers in [Architecture.md, section 10](Architecture.md#measuring)
- [x] One windowed run of the benchmark to check that the 25 to 37 ms frames of the off-screen runs (neither thread nor the GPU busy, one or two a second) come from the off-screen mode and not from the game ([Architecture.md, section 10](Architecture.md#measuring))
   - Done 2026-11-05: the windowed run has none after its first few seconds (7.4 ms p99), so they come from the off-screen mode. Hitches are read from windowed runs from now on
- [x] Move the sun in small steps (for example every 0.25 degrees) instead of every frame, so virtual shadow maps keep their cache ([H8](CodeReview.md#h8--rendering-is-unreals-maximum-feature-set-unmeasured-on-the-target))
   - Done 2026-11-04: the day cycle turns the sun only once the clock has moved it `SunStepDegrees` (0.25, under **Lighting**), while its brightness, colour and the night exposure still change every frame, since only the light's direction resets the cached shadows. At 40 real seconds per game hour that is a turn every 0.4 to 2 seconds instead of every frame
   - 2026-11-05: @DunderGG saw the steps as small jumps of the shadows once the clock ran at 40 seconds per game hour, and tuned the step to 0.05 degrees (0.1 still looked jittery): a turn every 0.1 to 0.4 seconds. Its cost is measured in "Finish the baseline" above
- [x] Turn off `r.RayTracing` unless the Epic tier is a product goal, then decide from the baseline whether High uses Lumen *(decide)* ([H8](CodeReview.md#h8--rendering-is-unreals-maximum-feature-set-unmeasured-on-the-target))
   - Done 2026-11-04, decided by @DunderGG: hardware ray tracing is off for the whole project while the game is being developed, so Epic uses software Lumen too, and High keeps Lumen, which cost about 0.6 ms of the 6.0 ms GPU frame in the baseline. Reasons in the [decision log](Architecture.md#16-decision-log)
- [x] Look at the volumetric clouds at High, the costliest GPU pass in the baseline (1.4 ms of 6.0 ms, so roughly 4 ms on the target): lower their quality at High or use a cheaper sky, and compare how mornings and evenings look ([H8](CodeReview.md#h8--rendering-is-unreals-maximum-feature-set-unmeasured-on-the-target))
   - Done 2026-11-04: `Config/DefaultScalability.ini` caps the clouds' samples below Epic (High 48 along the view ray and 5 toward the sun, the engine's 96 and 10). Off screen in the packaged build the cloud pass went from 0.83 ms to 0.31 ms at High; screenshots from 06:30 to 21:00 show the same clouds with slightly grainier edges. Medium and Low are cheaper still. Numbers in [Architecture.md, section 10](Architecture.md#scalability) and the [decision log](Architecture.md#16-decision-log)

Before 4.2:

- [x] Decide the world structure: separate maps for the farm, the town and the zones, or one streamed world, and record it in the decision log *(decide)* ([C1](CodeReview.md#c1--all-game-state-lives-in-the-level-but-the-game-needs-several-maps))
   - Done 2026-11-04, decided by @DunderGG: separate maps opened with `OpenLevel`; the simulation lives in game-instance subsystems, the player's data crosses a map change as an in-memory snapshot, and interiors are part of their map ([Architecture.md, section 2](Architecture.md#2-unreals-gameplay-framework-where-things-go), [decision log](Architecture.md#16-decision-log))
- [x] The clock and each map's farm data in game-instance subsystems that advance whether or not their map is loaded; `AFarmGrid` becomes a view of the farm data ([C1](CodeReview.md#c1--all-game-state-lives-in-the-level-but-the-game-needs-several-maps))
   - Done 2026-11-04: `UVestigeClockSubsystem` (with `UVestigeClockSettings` in Project Settings) and `UFarmFieldSubsystem` with a `UFarmField` per grid `FieldId`. The grid and its tiles show the field, `AVestigeDayCycle` only lights the level, and saves are version 2. `L_Farm`'s faster clock is gone with the per-level setting ([decision log](Architecture.md#16-decision-log))
- [x] Split the lighting from the clock: a sky actor in each level listens to the clock and owns the sun, the sky light and the night exposure ([M1](CodeReview.md#m1--the-day-cycle-is-both-the-clock-and-the-lighting))
   - Since 2026-11-04 `AVestigeDayCycle` already holds only the lighting and reads `UVestigeClockSubsystem`; what is left is naming it for its job (with a redirect, since `L_Farm` places it) and deciding whether the sunrise and sunset hours belong to the clock
   - Done 2026-11-05: renamed `AVestigeSkyLighting` (a class redirect loads `L_Farm` until it is saved), and the sunrise and sunset hours moved to the clock's Project Settings (**Sun**), the same on every map ([decision log](Architecture.md#16-decision-log))
- [x] Tiles as a `TArray<FFarmTileState>` drawn with one instanced static mesh component per mesh and per-instance custom data for colour and wetness: no actor and no dynamic material per tile ([C2](CodeReview.md#c2--one-actor-per-tile-each-with-two-meshes-and-two-dynamic-materials))
   - Done 2026-11-05 after @DunderGG approved the guide ([Systems.md, section 22](Systems.md#22-farm-fields-and-tiles)): `UFarmFieldMeshesComponent` on the grid draws the field (the tile states were already an array in `UFarmField` since C1), `AFarmTile` and `BP_FarmTile` are gone, `L_Farm` places `BP_FarmGrid`, and the placeholder colour is custom data read by `M_FarmPlaceholder`. Wetness stays a separate watered set with `M_Soil_Watered` until [4.2](Roadmap.md#42-farming-depth) makes water an amount, which needs a soil material that reads custom data. `On Crop Harvested` and `On Tool Used` now carry the tile's coordinates
- [x] A save registry: each saveable owner has a stable key, its own struct and its own version; the save keeps keys it doesn't know (mod data), is read into a game-instance subsystem before the world starts, and owners pull their part when they initialise. Loading then happens once per game, not each time a world begins play as `UVestigeSaveSubsystem` does today, which would roll the clock and the fields back after travel. The same snapshot carries the player's data (inventory, stamina, exhaustion) across a map change: owners write their part before the map closes and read it back in the new one, with a test that the player's data survives reopening the map ([H3](CodeReview.md#h3--the-save-subsystem-knows-every-system-by-name))
   - Done 2026-11-06, approved by @DunderGG after a review by a senior Unreal C++ reviewer agent: `UVestigeSaveRegistry` and `IVestigeSaveable` in `Core/`, six owners with their own keys and versions (`Base.Clock`, `Base.Fields`, `Base.Inventory`, `Base.Stamina`, `Base.Rest`, `Base.Farming`), the save file at version 3 with records by key, read once per game when the first map is initialised. The open map's owners give their parts as it starts tearing down, and the rest component starts the late-night drain when a map opens after midnight. Test `Vestige.SaveRegistry.ThePlayersDataSurvivesReopeningTheMap` ([Systems.md, section 1](Systems.md#1-saving-and-loading))
- [x] Our own game mode and player controller in a `Game/` folder, outside `Template/`. The game mode owns the end of the day; the rest component only asks for sleep and applies the stamina effects ([M2](CodeReview.md#m2--a-pawn-component-ends-the-worlds-day), [M6](CodeReview.md#m6--the-template-code-is-load-bearing))
   - Done 2026-11-06 after a review by a senior Unreal C++ reviewer agent: `AVestigeGameMode` and `AVestigePlayerController` in `Game/`, the top layer, with `BP_GameMode` (now `GlobalDefaultGameMode`) and `BP_PlayerController` in `Content/Vestige/Blueprints`; the template's Blueprints and their C++ are deleted. The clock ends the day (at 06:00, or early with `EndDay` when the player goes to bed) and keeps how it ended; the game mode starts the next day, with or without a pawn; the rest component wakes the player on `OnNewDay`. Test `Vestige.GameMode.TheNextDayStartsAtTheDayEndWithoutAPlayer` ([decision log](Architecture.md#16-decision-log))
- [x] The rest component on the player state instead of the pawn, so a night with no pawn still refills stamina and keeps the exhaustion in the save: exhaustion and stamina already live on the player state's ability system (found in the review of M2, 2026-11-06)
   - Done 2026-11-06: `AVestigePlayerState` holds it, and the bed and `SkipDay` find it with `UVestigeRestComponent::Find`. Test `Vestige.Rest.ANightWithoutABodyStillRefillsStaminaAndSetsExhaustion`

Before more tools and [4.6](Roadmap.md#46-interface-and-input):

- [x] Each tool data asset names the ability that is its behaviour, granted while the tool is in hand; the abilities do the tile work through an interface on the grid, and holding the key repeats inside the ability. Remove `EFarmToolAction` and the switch in the farming component ([H1](CodeReview.md#h1--the-gameplay-ability-system-gates-tools-but-doesnt-run-them))
   - Done 2026-11-07 after two rounds of review by a senior Unreal C++ reviewer agent, whose approval @DunderGG accepted ([Systems.md, section 9](Systems.md#9-abilities-attributes-and-effects-gameplay-ability-system)): the four tool assets name `UFarmGameplayAbility_Till`, `_Plant`, `_Water` and `_Harvest`, granted by the farming component while the tool is in hand; the base `UVestigeGameplayAbility_UseTool` runs one activation per press with the repeat inside it, and owns `Vestige.State.UsingTool` so one tool use runs at a time. The field's rule functions are the interface to the tiles. Holding E while switching tools now stops the repeat until the next press. Tests `Vestige.Farming.TheToolInHandGrantsItsAbility`, `ANewPawnLeavesNoToolAbilityBehind`, `SwitchingToolsDuringAUseWaitsForTheUseToFinish`, `APressDuringTheLastUsesTimeKeepsTheRepeat`, `ANewToolNeedsOnlyItsAbility` and `Vestige.Content.EveryToolNamesAToolAbility`
- [x] Tunable gameplay effects (tool costs, the late-night drain, exhaustion) as `GE_` data assets instead of C++ classes ([H1](CodeReview.md#h1--the-gameplay-ability-system-gates-tools-but-doesnt-run-them))
   - Done 2026-11-07 after two rounds of review by a senior Unreal C++ reviewer agent, whose approval counts as @DunderGG's ([Systems.md, section 9](Systems.md#9-abilities-attributes-and-effects-gameplay-ability-system), effects as `GE_` assets; decision log): `GE_StaminaCost`, `GE_StaminaRefill`, `GE_LateNightDrain` and `GE_Exhausted` in `Content/Vestige/Abilities/`, named by the global game data asset `DA_GameData` (`UVestigeGameData`), which `UVestigeAssetManager` loads once at startup; the four C++ effect classes are deleted. The drain's amount per period follows the effect's own period. Tests `Vestige.GameData.NamesEveryEffect` and `ADrainThatIsntInfiniteAndPeriodicIsInvalid`, and the stamina, rest, farming and save tests now use the assets
- [x] Split `UFarmingComponent`: a targeting component for the aim and the target tile, the toolbar into the inventory hotbar, and planting from the seed item in hand instead of one `SeedItemId` ([H2](CodeReview.md#h2--ufarmingcomponent-does-seven-jobs-and-can-plant-only-one-seed))
   - Done 2026-11-07 after three rounds of review by a senior Unreal C++ reviewer agent, whose approval counts as @DunderGG's ([Systems.md, section 8](Systems.md#8-inventory-and-items), the hotbar; decision log): `UFarmTargetingComponent` on the farmer; the hotbar is the inventory's first nine slots, with the selected slot's item in hand and saved in `Base.Inventory` (version 2); `DA_Item_Hoe` and `DA_Item_WateringCan` are items with a `UVestigeToolFragment`, every seed item names the Seeds tool, and an empty hand harvests (`HandToolId`); the plant ability plants the seed in hand from its slot. `FFarmToolbar` and `Base.Farming` are gone. Tests `Vestige.Inventory.SelectingAHotbarSlotPutsItsItemInHand` and the other hotbar tests, `Vestige.Farming.TheToolOfTheItemInHandIsGranted`, `AFarmerPossessedBeforeBeginPlayFollowsTheHand`, `PlantingUsesTheSeedInHand`, `PlantingTheLastSeedLeavesTheHandEmpty`, `PlantingTheLastSeedOnARepeatLeavesTheHandEmpty`, `ASecondHandChangeDuringTheFirstUseLeavesOneToolAbility`, `Vestige.Item.AToolFragmentNeedsAKnownTool` and `Vestige.Content.EverySeedItemNamesATool`
- [x] Input actions and mapping contexts as `IA_` and `IMC_` assets, bound by gameplay tag through an input config data asset ([H6](CodeReview.md#h6--input-is-built-in-c-with-hard-coded-keys); replaces the "decide when" item in 4.6). The template's moving and looking contexts, added by `AVestigePlayerController` since 2026-11-06, are the body's and move to the pawn's input setup with it
   - Done 2026-11-07 after three rounds of review by a senior Unreal C++ reviewer agent, whose approval counts as @DunderGG's ([Systems.md, section 5](Systems.md#5-input); decision log): `IA_UseTool`, `IA_Interact`, `IA_HotbarSlot1` to `9`, `IA_HotbarNext`, `IA_HotbarPrevious` and `IMC_Farming` in `Content/Vestige/Input`, `DA_InputConfig_Player` (`UVestigeInputConfig`) naming the action for each native `Vestige.Input.*` tag, and the character binding by tag; `BP_PlayerCharacter` adds `IMC_Default`, `IMC_MouseLook` and `IMC_Farming` while possessed and `AVestigePlayerController::DefaultMappingContexts` is gone. Gamepad X uses, B interacts, every action has player-mappable key settings, and the character releases a held use key when it is unpossessed. Tests `Vestige.Input.ThePlayerBindsEveryConfigEntry`, `EveryFunctionHasAKeyboardAndAGamepadKey`, `NoKeyDoesTwoThings`, `EveryFarmingActionHasAUniqueMappableName`, `ThePlayersConfigValidates`, `AConfigWithADuplicateTagIsInvalid`, `Vestige.PlayerCharacter.WithoutAnInputConfigTheHintsAreEmpty` and `UnpossessingReleasesTheUseKey`
- [x] The HUD in UMG with a C++ base class or view model, before the hotbar ([H7](CodeReview.md#h7--the-hud-is-raw-slate-written-in-c))
   - Done 2026-11-08 after two rounds of review by a senior Unreal C++ reviewer agent, whose approval counts as @DunderGG's, checked against Epic's "UMG Best Practices" tech blog and documentation page ([Systems.md, section 6](Systems.md#6-user-interface); decision log): `WBP_Hud` (`UVestigeHudWidget`) and `WBP_HudMessage` (`UVestigeHudMessageWidget`) in `Content/Vestige/UI`, `BP_HUD` naming the widget and `BP_GameMode` naming `BP_HUD`. `AVestigeHUD` pushes its event-built texts to the widget, which has no bindings and no tick; only a shown message box ticks, for its fade. The widgets fire Blueprint events after each change for restyling. `SVestigeHudWidget` is gone. Tests `Vestige.UI.TheGameModeShowsTheHudWidget`, `OnlyTheMessageBoxesTick`, `TheHudDoesNotTakeTheMouse`, `EachMessageShowsInItsOwnBox`, `AMessageFadesInAndOut`, `ThePromptHidesWhenEmpty` and `TheStaminaLineWarnsWhileOutOfStamina`
- [x] Key hints for the gamepad, following the last used device: today the character hints only keyboard and mouse keys (found with H6, 2026-11-07). Kept apart from the UMG move, which changed only the technology (2026-11-08)
   - Done 2026-11-08 after two rounds of review by a senior Unreal C++ reviewer agent, whose approval counts as @DunderGG's ([Systems.md, section 5](Systems.md#5-input), gamepad key hints; decision log). Done before Common UI with the first menu, which waits for a menu. The hints and the prompt name the keys of the device used last, from Common Input's `UCommonInputSubsystem` (the CommonUI plugin is on; Common UI's widgets and routing wait for the first menu): "X Use tool | B Interact", "LB / RB Hotbar" and "B Sleep" on the gamepad, with Xbox labels in short text (`VestigeKeyHints::GetKeyName`) until icons. Tests `Vestige.PlayerCharacter.GamepadActionHintsNameTheGamepadKeys`, `TheInteractKeyFollowsTheDevice`, `TheGamepadHotbarHintIsTheSteppingKeys`, `Vestige.Input.GamepadKeysHaveShortHintNames`, `ThePlatformDefaultsToKeyboardAndMouse`, `TheActionRouterCheckIsOff` and `Vestige.UI.TheHintDeviceFollowsTheInputType`
- [x] Gameplay reports results and failure reasons (a tag or an enum), and presentation picks the text, sound or effect; no message for a use that worked ([M4](CodeReview.md#m4--gameplay-code-writes-the-players-text))
   - Done 2026-11-08 after two rounds of review by a senior Unreal C++ reviewer agent, whose approval counts as @DunderGG's ([Systems.md, section 15](Systems.md#15-visual-effects-and-feedback), feedback reports; decision log): `UVestigeFeedbackSubsystem::Report(World, Tag, Arguments)` with 22 native `Vestige.Feedback.*` tags replaces `Notify` with text, and `AVestigeHUD` shows the text `DA_FeedbackResponses` (`UVestigeFeedbackResponses`, on `BP_HUD`) gives each tag. Tool abilities give their reason from `CanUseOn` and the base reports it on the press only. Tilling, planting, watering and harvesting show no message, the morning save shows @DunderGG's "Game Saved!" from the asset (the design left it empty), and the missing grid is a log warning only. Tests `Vestige.Feedback.ReportsReachEveryListener`, `AnInvalidTagIsNotReported`, `Vestige.Farming.AUseThatWorkedReportsNothing`, `APressReportsWhyAUseChangesNothing`, `Vestige.GameMode.TheReportSaysWhetherThePlayerSleptOrStayedUp`, `Vestige.Stamina.RunningOutReportsOnce`, `Vestige.UI.TheFeedbackResponsesFormatTheReport` and `EveryFeedbackTagHasAResponse`
- [x] One content catalogue loaded once for every inventory, with item icons in their own bundle loaded when a UI needs them ([M5](CodeReview.md#m5--every-inventory-loads-and-keeps-every-item-definition))
   - Done 2026-11-08 after two rounds of review by a senior Unreal C++ reviewer agent, whose approval counts as @DunderGG's ([Systems.md, section 2](Systems.md#2-content-and-the-asset-manager), the content catalogue; decision log): `UVestigeContentCatalog`, a game-instance subsystem, loads every crop, tool and item with its `Visuals` once per game through a preload handle of its own, and every system finds content there; `LoadWithVisuals` is deleted. Icons go in an `Icons` bundle the catalogue never loads, and the icon itself comes with the hotbar row in [4.6](Roadmap.md#46-interface-and-input). All 320 tests pass, and the test run took 22.2 s against 21.8 s before. Tests `Vestige.ContentCatalog.KnowsEveryContentAsset`, `LoadsTheVisualsOfCropsAndTools`, `FindsOnlyTheAskedClass`, `TestContentStaysInItsGame`, `ShuttingDownReleasesTheLoad`, `Vestige.Inventory.InventoriesShareTheCatalogue`, `WithoutACatalogueNoItemIsKnown`, `Vestige.Farming.WithoutACatalogueThereIsNoHarvestTool` and `Vestige.FarmFields.ASavedCropIsFoundInTheCatalogue`
- [x] The late-night drain on the clock's own events instead of a real-time effect period ([M7](CodeReview.md#m7--game-time-events-run-on-real-time))
   - Done 2026-11-11 after two rounds of review by a senior Unreal C++ reviewer agent, whose approval counts as @DunderGG's ([Systems.md, sections 4 and 9](Systems.md#4-time-days-and-seasons); decision log): the clock broadcasts `OnMinuteStarted` once per whole game minute, before that minute's hour, through skips too; `GE_LateNightDrain` is instant, and the rest component applies it on each minute after midnight with a minute's share of `LateNightDrainPerGameHour`; `SkipToTime` lands half a minute into the typed minute. All 331 tests pass. New tests `Vestige.Clock.EachWholeMinuteIsBroadcastOnceInOrder`, `AMinuteThatStartsAnHourComesBeforeTheHour`, `TheLastMinuteAndHourComeBeforeTheDayEnds`, `AClockJustBeforeAnHourBroadcastsNoHour`, `AdvancingTheClockBroadcastsEachMinuteItPasses`, `RestoringOrStartingADayBroadcastsNoMinute`, `AMinuteThatEndsTheDayIsTheLastTimeBroadcast`, `Vestige.Rest.TheLateNightDrainsItsRatePerGameHour`, `ASkipPastMidnightDrainsAsIfTheTimePassed`, `TheLateNightLeavesNoActiveEffect`, `AMinuteThatEndsTheDayDrainsNothingAfterTheRefill`, `Vestige.Cheats.SkipToTimeReachesTheTypedMinuteAfterMidnight` and `Vestige.GameData.ADrainThatIsntInstantIsInvalid`; the rest tests that check the drain failed on the old code. Play-test in [PlayTest.md](PlayTest.md), "Late-night drain on game time"

When working nearby:

- [x] Standard ability tasks (wait delay, montages) instead of timers in abilities ([L3](CodeReview.md#low))
   - Done 2026-11-07 with H1: the tool abilities wait with `WaitDelay` and `WaitMovementModeChange`; no other ability used a timer. Montages come with tool animations

- [x] Remove the data field nothing reads (`ContaminationTolerance` on crops) until its feature is designed ([L2](CodeReview.md#low))
   - Done 2026-11-22 with contamination: each crop states how it grows on each contamination level instead ([Systems/27-Contamination.md](Systems/27-Contamination.md))
- [x] Common UI with the first menu ([H7](CodeReview.md#h7--the-hud-is-raw-slate-written-in-c)): reparent the widget bases to `UCommonUserWidget`, set `UCommonGameViewportClient` as the game viewport client, remove the `CommonUI.Debug.CheckGameViewportClientValid` setting and its test `Vestige.Input.TheActionRouterCheckIsOff`, and log the decision ([Systems.md, section 6](Systems.md#6-user-interface)). The plugin and Common Input are already on since the gamepad key hints (2026-11-08). Due with Hob's dialogue box ([4.4](#44-the-caretaker-robot)) or the title screen ([4.7](#47-starting-and-testing-the-slice)), whichever comes first (roadmap review, 2026-11-24)
   - Done 2026-11-26 with Hob ([4.4](#44-the-caretaker-robot)): `UCommonGameViewportClient`, a root layout with a game and a menu layer, the HUD as the game layer's activatable widget (not a `UCommonUserWidget`, so input returns to the game when a screen closes), and the setting and its test removed ([Systems/06-UserInterface.md](Systems/06-UserInterface.md))

- [x] A script that builds and runs the tests before each commit ([L5](CodeReview.md#low))
   - Done by `Tools/Verify-Vestige.ps1`, which every commit with a source or test change already needs. A Git pre-commit hook was rejected (2026-11-26): a build and the suite take minutes, so a hook would hold up every docs commit and would soon be skipped

### 4.1 Foundations

**Engine** (added 2026-10-30: @DunderGG is installing Unreal Engine 5.8.4)

- [x] Upgrade the project to Unreal Engine 5.8.4 and verify it, in its own commits and before new systems such as the inventory and the Gameplay Ability System:
   - one way only: an asset saved in 5.8 no longer opens in 5.7, so commit the upgrade only once it is verified, and contributors need 5.8 from then on
   - read the release notes for 5.8 and its hotfixes for changes to the APIs and plugins we use (Asset Manager, data validation, Enhanced Input, CQTest, Slate, developer settings, save games) and for the Gameplay Ability System we are about to adopt
   - switch the engine association in `Vestige.uproject` (right-click it > Switch Unreal Engine version), regenerate the project files, and update the toolchain if the engine asks for a newer Visual Studio, MSVC or Windows SDK
   - a clean build of `VestigeEditor` with no errors or warnings, deprecation warnings included, and fix what the new engine deprecates
   - all `Vestige` automation tests pass
   - open the editor, check the Output Log at start for errors, run Validate Assets on `Content/Farm`, and re-save assets only where the editor asks for it
   - the [smoke test](PlayTest.md#3-smoke-test) in Play in Editor and in Standalone Game, with the lighting and performance checks, since rendering defaults can change between versions
   - update every document that names Unreal Engine 5.7 (`AGENTS.md`, `README.md`, `CONTRIBUTING.md`, `Docs/Setup.md`, `Docs/CodingGuidelines.md`, `Docs/Architecture.md`, `Docs/Systems.md`, `Docs/PlayTest.md`), and add a decision log entry
   - Done 2026-10-31: builds with no warnings, all tests pass, the smoke test passes in Play in Editor and Standalone Game, `Content/Farm` validates, and the documents and decision log are updated. The one regression, the dark tile highlight, was fixed with a project material. The release notes weren't read separately: the clean build shows nothing we use is deprecated, and the Gameplay Ability System gets its own 5.8 check at setup (Systems.md, section 9)

**Content as data**

- [x] Crop data assets (`UFarmCropData`) found through the Asset Manager: ID, name, seasons, growth stage meshes, days to mature (one stage per watered day), water need, contamination tolerance, yield, seed price and sell price
- [x] The current turnip becomes the first crop asset, `Base.Turnip`, and its settings move off the tile Blueprint
- [x] Tool data assets (`UVestigeToolData`), replacing the hard-coded tool list (`EFarmTool`)
- [x] Item data assets (`UVestigeItemData`) for seeds, harvested crops and other things the player can carry, built from fragments ([Systems.md section 8](Systems.md#8-inventory-and-items)): `Base.TurnipSeeds` and `Base.Turnip`, and the turnip crop names its harvest item
   - The crop's seed and sell prices moved to items on 2026-10-30 (decision log)
- [x] All content uses namespaced IDs (`Base.<Name>`) in a `ContentId` property that also builds the asset's primary asset ID, with data validation of the format and a startup check for duplicate IDs ([Architecture.md section 5](Architecture.md#5-data-and-content))
- [x] Investigate whether `ContentId` should also allow a category segment (`Base.Character.Gage`) before the first character asset is created. Decide whether primary asset types already provide enough category information; whether existing `Namespace.Name` IDs and mod IDs remain valid; and the effects on validation, duplicate detection, saves and ID redirects. If adopted, change the generic ID contract and its tests, then document the migration and mod-compatibility rules before shipping content with the new form.
   - Decided 2026-11-10: no category segment. The primary asset type is the category (`VestigeCharacter:Base.Gage`); all existing and mod IDs stay valid, and validation, duplicate detection and saves don't change. The rules for type names, cross-type references and redirects are in [Architecture.md section 5](Architecture.md#5-data-and-content) (decision log, 2026-11-10)

**Tiles and the farm grid**

- [x] Tile rules in a plain state struct, separate from the tile actor that shows them
- [x] A shared way to find the grid (`AFarmGrid::Find`), replacing the repeated searches
- [x] The grid announces when its tiles are ready, so loading no longer waits a frame (loading waits for the world's begin-play broadcast, which also covers the farmer's toolbar)
- [x] `AVestigeDayCycle::Get` follows the find-or-spawn rules: spawns only in a running game world and logs a warning when it spawns or finds more than one ([Architecture.md section 2](Architecture.md#2-unreals-gameplay-framework-where-things-go))

**Saving**

- [x] Each owner saves and restores its own state, instead of the save subsystem writing into other classes
- [x] Tiles are saved with their grid coordinates, not by position in a list
- [x] Saves store content IDs, and skip content that no longer exists with a warning
- [x] A save version number
- [x] A save that can't be loaded (corrupt, newer version, failed migration) is left untouched: saving stays off until the player starts a new game ([Architecture.md section 6](Architecture.md#6-saving))
- [x] A failed save write keeps the previous save

**Communication and extension points**

- [x] A feedback event for player messages, so gameplay code stops calling the HUD directly
- [x] Blueprint events for key moments (new day, crop harvested, tool used), marked as extension points
   - Done 2026-11-01 ([Architecture.md, section 12](Architecture.md#extension-points-today), decision log): `On New Day` on the day cycle (found with Get Day Cycle), `On Crop Harvested` and `On Tool Used` on the farming component, and Get Content Id on all content. A test lists them, since mods use them by name
- [x] An interaction interface (`IVestigeInteractable`) for anything the player can use: bed, shipping bin, water source, robots
   - Done 2026-11-01 ([Systems.md, section 7](Systems.md#7-interaction), decision log): a list of options per interactable, the first used until there is a way to choose; a `UVestigeInteractionComponent` on the pawn that finds the nearest interactable on its own collision channel; interacting as an ability on F, with a prompt on the HUD. The first interactable is the bed ([section 4.3](Roadmap.md#43-stamina-and-late-nights))

**Player data**

- [x] Inventory and money move into an inventory component on the player state, so they outlive the character
   - Done 2026-10-31 without money: `UVestigeInventoryComponent` on `AVestigePlayerState` ([Systems.md section 8](Systems.md#8-inventory-and-items)). Seeds and harvests are items, and old saves upgrade. Money came with the shipping bin on 2026-11-22 ([section 4.5](Roadmap.md#45-automation-and-selling))
- [x] Rename the general `Farm` types to the `Vestige` prefix (decision log, 2026-10-31): the HUD and its widget, settings and messages, the save subsystem and save game, the feedback subsystem, the day cycle, the content base, asset manager, tool and item data and the item fragments, the rest and movement components, the farmer character, and their save structs and delegates. Farm types (tile, grid, crops, the farming component, targeting, the toolbar) keep `Farm`. In its own commits, with `[CoreRedirects]` for every class and struct that assets or save files name, a save test that loads a save naming the old classes, and the documents updated
   - Done 2026-11-01 (decision log). The test showed that a redirect doesn't load an old save's class, so the old save game name stays as an empty `UFarmSaveGame` subclass. The seed fragment, `EFarmToolAction` and the old `EFarmTool` keep `Farm` as farm types
- [x] Whether the data names change too, while no mods exist: the `FarmTool` and `FarmItem` asset types, the `Farm.` tag root (before the stamina tags in [Systems.md section 9](Systems.md#9-abilities-attributes-and-effects-gameplay-ability-system) are made) and `Content/Farm/`
   - Done 2026-11-01 (decision log): @DunderGG chose to change all three. The asset types are `VestigeTool` and `VestigeItem` (`FarmCrop` stays), the tag root is `Vestige.`, and our content lives in `Content/Vestige/`, moved by a headless editor script with no redirectors left
- [x] Seed and harvest counts become private, with a restore function for loading
- [x] Tunable settings on the tile, grid and day cycle become protected or private

**Logging** (chosen 2026-10-30: build on Unreal's logging instead of a logging tool of our own, which would rebuild what the engine has and lose the Output Log's filters and colours, the log files, crash reports and the `log` console command)

- [x] A logging guide in [Systems.md](Systems.md), written from Epic's logging documentation and Lyra's source, approved by @DunderGG before any code (AGENTS rule 13). It covers:
   - categories per system, named `LogVestige<System>` (`LogVestigeSave`, `LogVestigeHud`), or `LogFarm<System>` for farm systems, each declared in its layer's folder
   - which verbosity to use when (Error, Warning, Display, Log, Verbose, VeryVerbose)
   - a small macro on top of `UE_LOGFMT` that adds the sending object's name and function to every line
   - log volume: nothing logged every frame, `Verbose` for tracing
   - setting a category's level with the `log` console command and in the `[Core.Log]` section of `DefaultEngine.ini`
   - reading our lines in the Output Log: its filters, and Category Colorization Mode (Editor Preferences > General > Output Log), which gives each category its own colour
   - the Visual Logger (Tools > Debug > Visual Logger) for debugging actors and gameplay over time
   - Done 2026-11-02 ([Systems.md, section 21](Systems.md#21-logging), decision log), approved by @DunderGG on 2026-11-01: `LogFarm` for the farm, which is one system, instead of `LogFarm<System>`
- [x] Split `LogVestige` into the per-system categories and move the existing `UE_LOG` calls to them
- [x] Add the sender macro and use it in those calls
   - Done 2026-11-02: `VESTIGE_LOG(Sender, Category, Verbosity, Format, ...)`, with a source test that fails on a direct `UE_LOG`
- [x] A note in [PlayTest.md](PlayTest.md) on the colour preference and filtering the Output Log to our categories

**Tests**

- [x] Set up CQTest in the project
- [x] Tile rule tests: tilling, planting, watered crops grow, dry crops don't, harvesting only when ripe
- [x] Day clock tests: the time and day roll over correctly
- [x] Save tests: tile, farming and clock state survive a save and load round trip through each owner's own save and restore functions
- [x] Save failure tests: a corrupt or newer-version save is not overwritten, and unknown content IDs are skipped
- [x] Content ID tests: the format, a duplicate ID and an unknown ID in a save

**Performance** ([Architecture.md section 10](Architecture.md#10-performance-by-design))

- [x] Confirm the target hardware and the frame budget
- [x] Set the rendering settings and the four scalability levels for the target: hardware ray tracing and other expensive features only at Epic
- [x] The day cycle broadcasts the end of the day, so the farming component stops polling it every frame
- [x] A shipping-safe highlight for the targeted tile, replacing the debug box
- [x] The HUD keeps its references instead of searching the world every frame
- [x] Trace scopes on the new day, loading and saving

### 4.2 Farming depth

- [x] Three or four crops for the season, each with its own meshes
   - Done 2026-11-13: potato (6 days, yield 3) and cabbage (10 days, yield 1) join the turnip, decided by @DunderGG; their seeds are in the starting bag (4 and 2, on hotbar keys 4 and 5). A crop's few growth stage meshes spread over its days to mature, with a ripe mesh of its own and a small size step each night it grew ([Systems.md, section 22](Systems/22-FarmFields.md)). The mutated crop below makes the fourth
   - 2026-11-13, after @DunderGG's play-test (the cabbage didn't look grown after 4 sleeps): each crop's data sets the day each stage starts instead of the even spread, the cabbage has 5 growing stages, the potato and cabbage change shape after the first night, and seedlings are sized for the game camera (at least about 20 cm across), decided by @DunderGG
- [x] Move the turnip's three materials to instances of `M_Crop` (`MI_Crop_Turnip<Part>`, with redirects or fixed-up references), so every crop reads custom data the same way; before contamination or water cues need it (art review, 2026-11-13)
   - Done 2026-11-13: the turnip's slots use `MI_Crop_TurnipLeaf`, `MI_Crop_TurnipRoot` and `MI_Crop_TurnipTop`, and the importer's `M_Turnip*` instances are deleted
- [x] The turnip's leaves (`add_leaf` in `Art/Blender/make_farm_assets.py`) are two coincident sheets joined by a zero-area rim, as the potato's and cabbage's were before their review; build them as a reversed back copy, and give Stage0 and Stage1 more distinct outlines (2 then 3 leaves read alike), in the turnip's first art improvement pass; the turnip meshes and `SM_Soil_Tilled` also carry a generated convex collision hull they don't need, and reimporting them needs the Interchange pipeline override the potato and cabbage import used, or it makes material instances and collision again (art review, 2026-11-13)
   - Done 2026-11-13, with the turnip rebuilt for the game camera after @DunderGG's play-test: leaves built as a reversed back copy, every stage a new outline, the convex hull gone on the reimport
- [x] Water level on each tile, so watering is a state the tile tracks
   - Done 2026-11-20, after design and code review by a senior Unreal C++ reviewer agent, whose approval counts as @DunderGG's ([Systems/22-FarmFields.md](Systems/22-FarmFields.md); decision log): each tile holds a water level from 0 to 2; a watering-can use adds 1 (a held press waters each tile once), a full tile says "The soil is already soaked." and costs nothing; each night crops on wet tiles grow, then every tile loses 1. The numbers are in Project Settings > Game > Vestige Farm Fields (`UFarmFieldSettings`). Saves stay version 1: an older save loads with dry tiles. The soil darkens per step (`M_FarmSoil`, custom data index 3), replacing `M_Soil` and `M_Soil_Watered`. The game designer's recommended options, decided by @DunderGG on 2026-11-20 ([GameDesign.md, section 6](GameDesign.md#tiles)); play-tested 2026-11-20
- [x] Clean water as a resource: a water source on the farm that the watering can fills from
   - Done 2026-11-20 with the well and the watering can's water ([Systems.md, section 24](Systems.md#24-water-sources-and-the-watering-can)): the can holds 8 waterings and the well fills it, 24 a day, refilled each morning; play-tested 2026-11-21
- [x] Set up audio and make the first sound: the splash when the watering can fills at the well (the clean water item above; until the sound exists the HUD shows "Can filled."). First the architect writes [Systems/14-Audio.md](Systems/14-Audio.md) out from its outline into a full guide, from Epic's documentation and Lyra, reviewed before any code: sound classes and submixes, the volume control buses Lyra uses, concurrency, when a sound is a MetaSound, and how presentation plays a sound for a gameplay result without game rules calling audio. The splash is synthesized by an agent-written script, committed with its source like the Blender scripts; the script and the sounds it makes get a path-table row in [LICENSING.md](../LICENSING.md) and a line in [CREDITS.md](../CREDITS.md) naming the agent tool, like `Art/Blender/*.py`, and join its "Still to check" item on agent-made assets. Sounds that come out weak that way (natural ones such as footsteps on soil) can come from @DunderGG's own recordings or from free libraries under a license [CONTRIBUTING.md](../CONTRIBUTING.md) accepts (CC0 where there is a choice), each license checked and recorded in [LICENSING.md](../LICENSING.md) and [CREDITS.md](../CREDITS.md) (@DunderGG, 2026-11-20)
   - Done 2026-11-21 ([Systems/14-Audio.md](Systems/14-Audio.md)): sound classes, feedback rows with a 2D sound played by the HUD, and the script-made splash. Play-tested 2026-11-21: plays at the right level and times, but sounds artificial.
- [x] Add an audio designer agent (`vestige-audio`) once the first sound is in, written from what it taught: it plans the sound and music for each system (feedback responses, gameplay cues, ambience), the mixing and the audio budget, from [Systems/14-Audio.md](Systems/14-Audio.md), and checks licenses for every sound. Made like the other agents, with @DunderGG's yes on its instructions (asked by @DunderGG, 2026-11-12; moved here from [section 6](Roadmap.md#6-before-a-public-release) on 2026-11-20 to follow the first sound)
    - Done 2026-11-21 as `vestige-audio-designer` (decision log, 2026-11-21), reviewed by `vestige-reviewer`, with no audio reviewer of its own. Its instructions approved by @DunderGG on 2026-11-21.
- [x] A 28-day season with the date and the days left shown on the HUD
- [x] Seeds show their days to harvest, and planting a crop that can't ripen before the season ends gives a warning
   - Done 2026-11-19 with refusing out-of-season seeds, after design and code review by a senior Unreal C++ reviewer agent, whose approval counts as @DunderGG's ([Systems/22-FarmFields.md](Systems/22-FarmFields.md), "Seeds go in only in their seasons"; decision log): `UFarmCropData::GetSeasonFit` decides fits, out of season or too late from the calendar; seeds out of season are refused; the first too-late press of a crop each day plants nothing and warns, and the next press plants (`UFarmingComponent` keeps the day's warnings, not saved); the hand label reads "Cabbage Seeds (10 days to ripen)". `CanUseOn` now gives a feedback report with values. The three crops list `Vestige.Season.Spring`. Refusing and the press-again warning are still open for @DunderGG ([GameDesign.md, section 15](GameDesign.md#15-open-questions)). Tests `Vestige.FarmCrop.*` season fit and validation, `Vestige.Farming.OutOfSeasonSeedsAreRefused`, `TheFirstLatePressWarnsAndPlantsNothing` and the other planting tests, `Vestige.UI.SeedsInHandShowTheirDaysToRipen`. Play-test in [PlayTest.md](PlayTest.md), "Out-of-season seeds and the late-planting warning"
- [x] Out-of-season crops wither at 06:00 on the first day of the next season
   - Done 2026-11-20 after design and code review by a senior Unreal C++ reviewer agent, whose approval counts as @DunderGG's, and an art review ([Systems/22-FarmFields.md](Systems/22-FarmFields.md), "Out-of-season crops wither when a season starts"; decision log): on `OnSeasonStarted`, `UFarmFieldSubsystem` withers every unripe crop whose data doesn't list the new season (`UFarmCropData::GrowsIn`), before that night's growth and never on a load or `SetDay` jump. The tile's new `EFarmTileState::Withered` keeps the crop and water; F offers "Clear" (the hand ability, 1 stamina) back to tilled soil, and the hoe and seeds say "Clear the withered crop first." The dead plant keeps its stage mesh with `MI_Crop_Withered`, slumped to 0.75 height. One line follows the season message, its order set by the day's steps ([Systems/04-Time.md](Systems/04-Time.md)). Ripe crops are kept, a dead plant stays until cleared, and there is no reminder yet: @DunderGG chose these on 2026-11-20, and the dead plant's colour moved to straw-brown ([GameDesign.md, section 6](GameDesign.md#6-farming)). Tests `Vestige.TileState.*`, `Vestige.FarmField.*`, `Vestige.GameMode.*`, `Vestige.Farming.*`, `Vestige.FarmTileLooks.*` and `Vestige.FarmFieldMeshes.*` withering tests. Play-test in [PlayTest.md](PlayTest.md), "Out-of-season crops wither"

- [x] One mutated crop that grows in irradiated soil
- [x] Normal crops can mutate in irradiated soil
- [x] Radiation level on each tile: slows or stops growth, and can be cleaned
- [x] A way to clean radiation, such as a tool, fertilizer or a crop that absorbs it
   - Done 2026-11-22 together, after design and code review by a senior Unreal C++ reviewer agent, whose approval counts as @DunderGG's, and an art review ([Systems/27-Contamination.md](Systems/27-Contamination.md)): three contamination levels on each tile (light halves growth, heavy stops it and refuses normal seeds), starting bands on `L_Farm`, Sinkwort, which grows only in contaminated soil and cleans its tile one level per harvest, and a 1 in 4 mutation of normal crops ripening on light soil, tinted with `MI_Crop_Mutated`. Built on the recommended answers of the open questions in [GameDesign.md, section 15](GameDesign.md#15-open-questions); every number is data

### 4.3 Stamina and late nights

- [x] Approve the stamina design in [Systems.md section 9](Systems.md#9-abilities-attributes-and-effects-gameplay-ability-system)
- [x] Gameplay Ability System set up: plugin and modules, the ability system component on `AVestigePlayerState` (which exists since the inventory), `UVestigeStaminaSet`, an ability set data asset
   - Done 2026-11-01 (decision log): `AbilitySystem/` layer, `DA_AbilitySet_Player` on `BP_PlayerState`. The effects are C++ classes rather than `GE_` assets, and 5.8 needs no `InitGlobalData` call
- [x] Sleep and the late-night rules move from the farming component to a `UVestigeRestComponent` (sleep and passing out moved; the late-night drain and exhaustion are built there with stamina)
- [x] Stamina: tool actions cost stamina, shown on the HUD
   - Done 2026-11-01: a use that changes the farm costs the tool's `StaminaCost` (hoe and watering can 2, seeds and hand 1, of 100); one that changes nothing costs nothing. Sleep refills it, and saves store it (version 3)
- [x] At zero stamina every action still works but takes twice as long; stamina never goes below zero
   - Done 2026-11-01: each tool use takes the tool's `UseSeconds` (0.4 s), doubled at zero, and the next use waits for it
- [x] No forced sleep: remove the pass-out at 02:00
   - Done 2026-11-02 (decision log): the night ends at 06:00 instead, and a player who never slept starts the next day where they stand, exhausted
- [x] After midnight, stamina drains faster
   - Done 2026-11-02: there is no daytime drain, so a drain of 10 per game hour starts at midnight and stops when the next day starts
- [x] Going to bed late makes the next day start "exhausted" with less stamina; one level only, cleared by one night in bed before midnight
   - Done 2026-11-02: going to bed at 00:00 or later, or staying up all night, starts the next day at 75%, with "Exhausted" on the HUD; saves store it (version 4)
- [x] HUD warnings at 22:00 and 00:00
   - Done 2026-11-02 as feedback messages from the rest component. The midnight one names the drain until hostile robots exist
- [x] Tune the stamina numbers in play: 100 stamina, the tool costs and the 0.4 s use time ([PlayTest.md, Stamina](PlayTest.md#2-pending-checks)), and the late-night drain (10 per game hour) and the 75% exhausted refill, which are settings on the farmer's Rest component
   - Done 2026-11-03: @DunderGG play-tested the numbers and kept them all (100 stamina, the tool costs, the 0.4 s use, 10 per game hour after midnight, the 75% refill, half speed while E is held)
- [x] Show on the HUD that the player is out of stamina, for example the stamina line in red (@DunderGG, 2026-11-01). The HUD follows the `Vestige.State.OutOfStamina` tag (`RegisterGameplayTagEvent` on the ability system) rather than comparing the number itself, so the rule stays in `UVestigeStaminaSet`, and it changes the line's colour only when the tag changes, never in a per-frame binding
   - Done 2026-11-02 as described, in `AVestigeHUD` and `SVestigeHudWidget` ([PlayTest.md, Pending checks](PlayTest.md#2-pending-checks))
- [x] Holding E repeats the tool use, so walking along a row while holding it works the row (@DunderGG, 2026-11-01). E is bound to the press only today. Repeat only after a use that changed the farm, so a use that changes nothing (hoeing tilled soil) stops the repeat quietly instead of posting its message every frame; the next use still waits for `UseSeconds`
   - Done 2026-11-02, changed with @DunderGG: stopping at the first use that changes nothing would stop after a tile or two, because the next use usually lands on the tile just worked. Instead the next use starts when the last ends or the target tile changes, and a use that would change nothing is skipped quietly until the next tile. Only the press posts its message
   - Walking slows to half speed while E is held (@DunderGG, after play-testing on 2026-11-03), which at 0.4 s per use works every tile of a row; at zero stamina a use takes 0.8 s and every other tile is still skipped
   - Tools don't work in the air, and jumps with E held are slowed too (@DunderGG, 2026-11-03): otherwise jumping along a row got round the slower walk
- [x] A bed in the farmhouse to sleep in, instead of a key that works anywhere
   - Done 2026-11-01 without the farmhouse: `AVestigeBed` (`BP_Bed`), a placeholder box along the near edge of the field in `L_Farm`, used with F. N stays as a test key, "Skip day (test)", until the cheat manager (decided with @DunderGG). The bed moves into the farmhouse when there is one, and gets its mesh with [4.6](Roadmap.md#46-interface-and-input)

### 4.4 The caretaker robot

- [x] Hob *(working name)* in the game with a placeholder body
- [x] Simple data-driven dialogue
   - Its dialogue box is the first screen that takes the input focus, so it is built with Common UI ([4.0](#40-structure-from-the-code-review), H7) and designed with the game designer
- [x] Characters as data assets, so more robots and humans can be added the same way
   - Done 2026-11-26, designed by the architect agent and approved by a senior Unreal C++ reviewer agent after two design rounds and two code rounds, whose approval counts as @DunderGG's on technical choices: `UVestigeCharacterData` (`Base.Hob`), talks as `UVestigeDialogueData` run by `UVestigeConversationComponent`, "has met" saved by `UVestigeRelationshipComponent`, `AVestigeNpcCharacter` (`BP_Hob`) in `L_Farm`, the lines in `ST_Dialogue` from the writer's [Hob-Everyday.md](Spoilers/Dialogue/Hob-Everyday.md), and the Common UI dialogue box ([Systems/11-Dialogue.md](Systems/11-Dialogue.md), [Systems/10-Robots.md](Systems/10-Robots.md))
- [x] One relationship track with Hob, who starts as a dutiful servant rather than a sceptic; trust grows through talking, gifts and working together, and the slice arc is in [Hob's character sheet](Spoilers/Characters/Hob.md#vertical-slice-arc)
   - Done 2026-11-27 as [GameDesign.md, section 8](GameDesign.md#relationships) decided: trust from asking Hob's opinion and the farm's firsts, four steps, Hob's dawn watering. Gifts are not in the slice; they come with the town's characters
   - How the player sees his opinion change, one of the play-test goals: his lines change and he helps on his own initiative ([GameDesign.md, section 8](GameDesign.md#relationships)), with no number on screen unless the play-test shows it is needed. Designed with the game designer and the writer (roadmap review, 2026-11-24)
- [x] Hob's short lines on events, said once each in passing with no dialogue box (the player's first mutated crop, watering in the rain once there is rain, still awake past midnight, the first sale): they make him react to the world with no new system, using the dialogue data above. The lines are the writer's; which events, and how often he may speak, are designed with the game designer (2026-11-26, [RoadmapSuggestions.md](RoadmapSuggestions.md) item 16). Built 2026-11-27 for three events: the first harvest, the first mutated crop at its harvest and the first time near Hob after midnight, shown as a caption within earshot; the first sale was left out, since the morning message and the bin topic already tell it ([GameDesign.md, section 8](GameDesign.md#relationships))

### 4.5 Automation and selling

- [x] A shipping bin: crops put in it are sold overnight
  - Done 2026-11-22: [section 25](Systems/25-Selling.md)
- [x] The currency (brass gear coins, name still open) on the HUD
  - Done 2026-11-22: [section 25](Systems/25-Selling.md); @DunderGG added the `MoneyText` line to `WBP_Hud`
- [x] Starting seeds and a small purse for a new game
  - Done 2026-11-22: 100 coins, tunable on `BP_PlayerState`
- [x] A simple budget for an ordinary and a poor season before prices are set
  - Done 2026-11-22: [GameDesign.md, section 9](GameDesign.md#economy)
- [x] A simple way to buy seeds, with one cheap basic seed always for sale
  - Done 2026-11-22: three seed dispensers beside the shipping bin sell turnip, potato and cabbage seeds (10, 20, 40), one per press, from `DA_Shop_Farm` ([section 26](Systems/26-Shops.md)); stand-in cubes until the real mesh

### 4.6 Interface and input

- [x] Harvesting ripe crops with F, the interact key, with an "F Harvest" prompt on the faced ripe tile; E only uses the item in hand, and an empty hand is no tool (@DunderGG, after the hotbar play-test, 2026-11-07)
   - Done 2026-11-07 after three rounds of review by a senior Unreal C++ reviewer agent, whose approval counts as @DunderGG's ([Systems.md, section 7](Systems.md#7-interaction), harvesting with F; decision log): the farm grid is the interactable for its tiles, the harvest ability is granted all the time (`HarvestToolId`), and the interaction focus goes by facing: the faced tile within 50 degrees of the aim, the bed only when the facing ray hits it. Tests `Vestige.Farming.NextToTheBedTheFacedRipeTileWins`, `NextToTheBedAnUnripeTargetFocusesNothing`, `InteractingWithARipeTileHarvestsIt`, `AnEmptyHandUsesNothing`, `Vestige.Interaction.AnOptionWithoutALocationIsFacedOnlyWhenTheFacingRayHitsItsShape` and the others in the design
- [x] Stepping through the hotbar with the mouse wheel and a gamepad button, with the input assets ([4.0](Roadmap.md#40-structure-from-the-code-review), H6); slots beyond the ninth have no key until then
   - Done 2026-11-07 with H6 ([Systems.md, section 8](Systems.md#8-inventory-and-items), the hotbar): the mouse wheel and the gamepad shoulders step to the next or previous hotbar slot, wrapping around and counting empty slots (`UVestigeInventoryComponent::StepHotbarSlot`). Tests `Vestige.Inventory.SteppingTheHotbarWrapsAround`, `SteppingWithoutAHotbarDoesNothing` and `SteppingTheHotbarPutsTheNextItemInHand`
- [x] A project material for the tile highlight frame (`M_TileHighlight`, unlit), replacing the engine material it uses today
   - Done 2026-10-31, earlier than planned: in 5.8 the engine material drew as the default grey material (Architecture.md, decision log)

## 5. Milestone 3 and later

### Story

- [x] Add a writer agent (`vestige-writer`, first named `vestige-lore-keeper`) for the documents in `Docs/Spoilers/`: it writes the lore, world building, story, characters, dialogue, item and crop texts, found texts and names, keeps them consistent with each other and with `Docs/GameDesign.md`, and turns [LoreAudit.md](Spoilers/LoreAudit.md) findings into fixes. Writing stays out of the code flow (it skips the steps in "How work is done"), but text that reaches the game is written and marked Ready by this agent before the implementer enters it. It also keeps spoilers out of everything that must stay spoiler-free (the other docs, code comments and commit messages, as [Spoilers/README.md](Spoilers/README.md) says). Asked by @DunderGG 2026-11-12 and widened on 2026-11-14 to all the writing; done 2026-11-14, with its limits in the [decision log](DecisionLog.md)

### Open problems

- [x] F9 seems to be a hotkey for taking screenshots in the editor, noticed it when starting new game.
   - Done 2026-11-04: F9 is no longer bound; a new game is the `NewGame` console command ([4.0](Roadmap.md#40-structure-from-the-code-review))

## 6. Before a public release

- [x] License headers (an SPDX line) in our own source files
- [x] Make the GitHub repository public, so contributors can work on it as the licensing intends
   - Done in October 2026, when the repository went public ([section 2](#2-milestone-1-playable-prototype-done)); the open line stayed in the roadmap until 2026-11-24
