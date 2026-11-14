# Project Vestige roadmap

What we have built so far, and everything we know we still have to build, in the order we plan to build it.

- [GameDesign.md](GameDesign.md) says what the game is and why. This roadmap only tracks the work, and links to the design for details.
- [Architecture.md](Architecture.md) says how the code fits together. Many items in the foundations section come from its known gaps and expected changes.
- [Lore.md](Spoilers/Lore.md) holds the story, characters and names.

**Keep this document up to date.** Tick an item in the same commit that finishes it. When new work turns up, add it under the right milestone. When the plan changes, move items rather than deleting them, and record the reason in the design or architecture decision log.

Legend: `[x]` done, `[ ]` to do. Items marked *(decide)* need a decision before work starts.

Contents

1. [Where we are](#1-where-we-are)
2. [Milestone 1: playable prototype (done)](#2-milestone-1-playable-prototype-done)
3. [Housekeeping](#3-housekeeping)
4. [Milestone 2: vertical slice](#4-milestone-2-vertical-slice)
5. [Milestone 3 and later](#5-milestone-3-and-later)
6. [Before a public release](#6-before-a-public-release)
7. [Open decisions](#7-open-decisions)

---

## 1. Where we are

- **Done:** milestone 1, the playable prototype, plus the project's documents and rules.
- **Now:** the code follows the guidelines and the architecture (both audits' code reviews are closed, 2026-10-29). Next: play-test that clean-up, the editor housekeeping steps in [section 3](#3-housekeeping), then the rest of the milestone 2 foundations.
- **Next up**, in order:
  1. [Section 4.0](#40-structure-from-the-code-review): the structural fixes from the [code review](CodeReview.md) (2026-11-04), the quick wins first, then the world structure and the tiles before 4.2 builds on them.
  2. The rest of [section 4.3](#43-stamina-and-late-nights): tuning stamina and the late-night numbers in play. The late-night rules, the red stamina line and holding E are done (2026-11-02); starting stamina from data and montage timing wait for their triggers. Logging, Blueprint events, the interaction interface and the bed are done (2026-11-01 and 2026-11-02); the rest of [section 4.1](#41-foundations) waits for a packaged build (the performance baseline) or for the need that starts it (the ID redirect list, more Blueprint extension points, a second interaction option).

## 2. Milestone 1: playable prototype (done)

A third-person farmer on a small field who can till, plant, water and harvest, with a day and night cycle, a HUD and saving.

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
- [x] Pass out at 02:00 (replaced on 2026-11-02 by the late-night rules, see [stamina and late nights](#43-stamina-and-late-nights))
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

Small things to finish before milestone 2 starts.

- [x] Play-test the 2026-10-29 code clean-up and the 2026-10-30 source folders in the editor: [PlayTest.md, Pending checks](PlayTest.md#2-pending-checks)
- [x] Play-test the latest changes: the HUD looks the same, sleeping saves the game, stopping and playing again restores the farm, and F9 starts a new game
- [x] Force-push the rewritten Git history to GitHub
- [x] Push the newer local commits (a normal `git push`)
- [x] Delete the local backup branch `backup/before-history-rewrite`
- [x] Delete the merged pull request branches on GitHub (`step2-farming-loop`, `step3-asset-script`, `step3-custom-meshes`, `fix-held-tool-shadowing`)
- [x] Delete the unused template code and content (`Variant_*`, `LevelPrototyping`, `Lvl_ThirdPerson` and the unused mannequin assets)
- [x] Remove the leftover redirector `Content/L_Farm` at the content root (right-click > Fix Up Redirectors)
- [x] In the editor, open and save `BP_ThirdPersonGameMode` and `BP_ThirdPersonPlayerController` (Content/ThirdPerson/Blueprints), which still name `/Script/TP_ThirdPerson`, and save `L_Farm`, whose grid and day cycle still name `/Script/FarmSim`. Then the `ActiveClassRedirects` for `TP_ThirdPerson` in `DefaultEngine.ini` can go. Keep the `FarmSim` `[CoreRedirects]`: save files made before the rename name `/Script/FarmSim`
   - Done by David (263ebeb). Nothing names `/Script/TP_ThirdPerson` any more, so the redirects were removed from `DefaultEngine.ini`.
- [x] In the editor, move `BP_FarmTile` from `Content/Farm/` to `Content/Farm/Blueprints/` and the `M_` materials from `Content/Farm/Meshes/` to `Content/Farm/Materials/` by dragging them in the Content Browser (choose Move Here), then right-click `Content/Farm` > Fix Up Redirectors. Moving the files outside the editor would break the references to them
- [x] If the Android File Server was ever used with this project, regenerate its security token (Project Settings > Platforms > Android File Server). The old token was removed from `DefaultEngine.ini` but stays in the Git history
   - Not needed: the Android File Server is a tool for copying builds to an Android device over USB, and David never used it.
- [x] Rename the farmer Blueprint `BP_ThirdPersonCharacter` to `BP_Farmer` and move it from `Content/ThirdPerson/Blueprints/` to `Content/Farm/Blueprints/`, so our own content lives under `Content/Farm/`. In the editor with the project built: right-click the Blueprint in the Content Browser > Rename, then drag it into `Content/Farm/Blueprints/` (Move Here), then right-click `Content/ThirdPerson` > Fix Up Redirectors. Check the Reference Viewer (right-click > Reference Viewer) before and after: the editor updates the assets that point to it, such as the default pawn class in `BP_ThirdPersonGameMode`, and saves them, so they go in the same commit. In the same commit, change the paths and names the editor can't update:
   - `Source/Vestige/Tests/FarmingTests.cpp` and `Source/Vestige/Tests/VestigePlayerCharacterTests.cpp`, which load `/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter.BP_ThirdPersonCharacter_C` by path (their tests fail without it)
   - the class comment in `Source/Vestige/Player/VestigePlayerCharacter.h`
   - `CONTRIBUTING.md` (trying a crop on the farmer), the `AVestigePlayerCharacter` row in `Docs/Architecture.md` section 13, and the tuning check in `Docs/PlayTest.md` (Smooth turning)
   - Nothing in `Config/` names the character; `DefaultEngine.ini` only names the game mode, which stays where it is
   - Done 2026-10-30: David renamed and moved it in the editor (the game mode's default pawn was updated with it, and no redirector is left), and the agent changed the paths above. `LICENSING.md` lists `BP_Farmer` as template material although it now lives under `Content/Farm/`
- [x] Stop loading the tile highlight's material by path in C++: `UFarmTargetHighlightComponent`'s constructor finds `/Game/Vestige/Materials/M_TileHighlight` with `ConstructorHelpers`, which [CodingGuidelines.md, section 12.4](CodingGuidelines.md#124-assets-and-loading) allows only for engine placeholders, and which made the editor refuse to move the folder until the path changed (found in the `Vestige` rename, 2026-11-01). Set `HighlightMaterial` on `BP_PlayerCharacter`'s highlight component instead, keep the engine cube as the C++ default, and keep the test that checks the material's instanced mesh flag
   - Done 2026-11-03: `BP_PlayerCharacter`'s TargetHighlight component sets the material (set by script), and the test now spawns that Blueprint
- [x] Rename the Details panel categories of the general types: the day cycle, rest component, farmer and content data still group their settings under `Farming` (`Farming|Time`, `Farming|Lighting`), and [CodingGuidelines.md, section 6](CodingGuidelines.md#6-uproperty-ufunction-and-blueprint-exposure) still says every category starts with `Farming`. Decide the rule (for example `Time`, `Lighting`, `Input` without a project prefix, and `Farming` only for farm types), then change the code and the guide together. Categories are display text only, so nothing in assets or saves changes. The Blueprint extension points (`On New Day`, Get Day Cycle, Get Content Id) use the same categories, so their place in the Blueprint menus changes with it
   - Done 2026-11-03: categories are named for their topic without a prefix (`Time`, `Lighting`, `Rest`, `Input`...), and only the farm types use `Farming` ([CodingGuidelines.md, section 6](CodingGuidelines.md#6-uproperty-ufunction-and-blueprint-exposure))
- [x] Update the class names in the lore drafts (`Docs/Spoilers/Characters/`): they name `UFarmCharacterData`, which the planned design now calls `UVestigeCharacterData` ([Systems.md](Systems.md))
   - Done 2026-11-03 in `Archivist.md` and `Engineer.md`. Their technical notes also named a `UFarmScheduleComponent`, an `EFarmWeather` enum and a `Content/Narrative/` folder, which the planned design doesn't have: the schedule is now a routine in the character data run by the AI controller and StateTree (Systems.md, section 10), weather is left unnamed until it is designed, and the dialogue tables are under `Content/Vestige/Dialogue/`
- [x] Remove the Print String nodes left in `BP_PlayerCharacter` from the extension point checks ([PlayTest.md](PlayTest.md#2-pending-checks), 2026-11-01): the 2026-11-04 baseline run logged the day number from it (`LogBlueprintUserMessages: [BP_PlayerCharacter_C] 2`) every morning, and it shows on screen in development builds
   - Done 2026-11-05 by David, after the Get Day Cycle node they used was removed with the clock move (Architecture.md decision log, 2026-11-04)
- [ ] Rules from the [code review](CodeReview.md#project-rules-that-are-themselves-wrong) (2026-11-04), each *(decide)* by David, then changed in its document:
   - Replace the strict linear layer order with folders that depend on `Core/` and the features they declare, and plan a `VestigeCore` module once a boundary settles ([M3](CodeReview.md#m3--the-strict-linear-layer-order-is-the-wrong-shape))
   - Cut `Architecture.md` to principles and a one-page system map, keep ownership in the class comments, and log only hard-to-undo decisions ([M8](CodeReview.md#m8--the-documents-outweigh-the-code), rule 5). Done 2026-11-11 and 2026-11-12, as David chose it ("yes to all" to the recommended options): one ownership list, section 13 as a one-page map with what each class doesn't own (the detail lives in the class comments and the guides), and the same decision log scope with new entries of about 100 words. Logging only hard-to-undo decisions was rejected because it would take away the maintainer's way to overrule an agent's choice ([Architecture.md, decision log](Architecture.md#16-decision-log), 2026-11-11)
   - Performance as a frame budget per system and a packaged-build profile at every milestone, with micro rules only for per-tile and per-frame code (rule 1)
   - Drop "change structure only when a profile says so" for structural choices whose cost is known up front (rule 2). Done 2026-11-05 with C2: [Architecture.md, section 10](Architecture.md#design-rules) now says to decide structure up front when its cost is known
   - Let a system be prototyped and played before its `Systems.md` guide; keep guide approval for adopting engine frameworks (rule 3)
   - Build what is right for the game even when it needs an editor asset; David makes the asset or the agent writes the steps (rule 6)
   - One class prefix, `Vestige`, for new types, and no more renames of existing `Farm` classes ([L1](CodeReview.md#low))
- [x] Trim the documents the AI agents read for every task, so a task costs fewer tokens (asked by David, 2026-11-11): `Systems.md` is about 59,000 tokens, `Architecture.md` about 49,000 and `PlayTest.md` about 20,000, and each specialist agent in `AGENTS.md`, "How work is done", starts by reading parts of them. Goes with M8's cut of `Architecture.md` above. To decide, through the architect and the reviewer: splitting `Systems.md` into one file per system guide so an agent opens only the guides it needs, moving older decision log entries and finished play-test checks to archive files, and dropping text that restates the code. Measure the token counts before and after
   - Done 2026-11-11 and 2026-11-12 ([Architecture.md, decision log](Architecture.md#16-decision-log), 2026-11-11). Each system guide is its own file in `Docs/Systems/`, and `Systems.md` is their index with the same numbered headings. Passed play-test checks are in `PlayTestArchive.md` and the performance baseline is in `PerformanceBaseline.md`. Replaced decisions are marked where they stand, and the large guides keep their rules and drop retold changes. Sizes, as UTF-8 bytes / 4, before and after: `AGENTS.md` (in every agent's context) 5,700 to 4,100; `Systems.md` 59,000 to an index of 1,100 and 22 guides of 48,600 in all, the largest 7,400; `PlayTest.md` 20,400 to 6,400; `Architecture.md` 49,100 to 47,400, with section 13 7,400 to 3,400 and section 10 2,700 smaller
- [x] Fix the rest of the stale steps in `CONTRIBUTING.md` *(needs David's yes: the file is on the maintainer's list)*: "Adding a tool" (line 159) names the farming component's **Toolbar Ids**, which went with the hotbar (a tool is now an item with a Vestige Tool Fragment in the starting items), and line 163 gives a seed item only its Farm Seed Fragment, without the Vestige Tool Fragment that the crop steps above say it needs, and line 94 still says the decision log is in `Docs/Architecture.md` (it is `Docs/DecisionLog.md` since 2026-11-12)
   - Done 2026-11-12, with David's yes: a tool is held as an item with a Vestige Tool Fragment, a seed item names the Seeds tool, and pull requests add their decision log entry to `Docs/DecisionLog.md`
- [ ] Check the open play-test checks against the code before the next play-test: 18 checks under nine headings in [PlayTest.md, section 2](PlayTest.md#2-pending-checks) are still open, the oldest from 2026-10-30, and some describe behaviour that later changes replaced ("Five messages and the message log" expects `UVestigeFeedbackSubsystem::Notify`, which `Report` replaced on 2026-11-08). Update or drop each stale one with the test author
- [x] `CONTRIBUTING.md`, "Adding a crop", step 1 *(needs David's yes: the file is on the maintainer's list)*: after importing the meshes, set each material slot to an `MI_Crop_<Crop><Part>` instance of `M_Crop` and remove the simple collision (Static Mesh editor, **Collision > Remove Collision**), as [Systems.md, section 22](Systems/22-FarmFields.md) now requires; a plain editor import gives the importer's own material instances and a convex hull, and no test catches either (code review, 2026-11-13)
   - Done 2026-11-13, David's yes: step 1 now says to use `MI_Crop_*` instances of `M_Crop` and remove the importer's collision
- [ ] Check every static mesh in `Content/Vestige/` automatically against its triangle budget and for missing collision, so a mistake in an asset nobody re-checks by hand is caught (asked by David, 2026-11-12). To decide through the architect and the reviewer: an editor data validator (the Data Validation plugin's `UEditorValidatorBase`, which runs when an asset is saved and on Validate Assets) or a `Vestige` automation test, where the budgets per kind of asset live (crop, tool, building, from [CONTRIBUTING.md, Budgets](../CONTRIBUTING.md#budgets)) and how a mesh's kind is known, how Nanite meshes are counted, and which meshes need no collision (crops). Cheap to add to the same check: textures over 1024, small props that cast shadows, and any asset or map that references the store-asset folders `.gitignore` keeps out of the repository (`Content/Fab/`, `Content/Megascans/`...), which would break in a fresh clone (art direction review, 2026-11-13). A validator needs an editor-only module or dependency, since `Vestige` is a runtime module; an automation test doesn't
- [ ] Automate the play-tests, so the maintainer only judges feel and fun (asked by David, 2026-11-13; the first step, the test author checking the game off-screen itself before writing a manual check, is in its instructions). To design through the architect and the reviewer: functional tests in a test map (Unreal's `AFunctionalTest`) that drive input, sleep and save and check the result; a script that runs the game off-screen (`-RenderOffScreen`) and takes screenshots at set moments for an agent to look at; Unreal's screenshot comparison against reference images approved once, which needs a run with a GPU (not `-nullrhi`) and stable lighting and time; an automatic performance run against the budget; and later Gauntlet for packaged builds. Then go through the open checks in `PlayTest.md` and move each one that can be automated
- [ ] Make the agent workflow cheaper over time (asked by David, 2026-11-13; the first steps, a small-task path, Sonnet for the test author, filtered logs and diff-first reviews, are in `AGENTS.md`, `CLAUDE.md` and the agents' files):
   - Measure what a task costs: the main session notes each agent's token count (Claude Code reports it when the agent finishes) in its final reply, so the expensive steps show up before anything else is changed
   - After a few tasks with the test author on Sonnet, compare its fix rounds and review findings with before, and decide whether the implementer and the artist move to Sonnet too
   - Move finished items out of `Roadmap.md` into a `RoadmapArchive.md`, as `PlayTest.md` does with `PlayTestArchive.md` (`Roadmap.md` is about 21,000 tokens, mostly ticked items)
   - Move replaced and old entries of `DecisionLog.md` (about 30,000 tokens) to an archive file if searching it gets slow or noisy
   - Keep `AGENTS.md` and `CLAUDE.md` short (about 5,500 tokens together): they load into every agent and the main session, so text that only one role needs belongs in that role's file
   - Watch the largest system guides (Inventory and the Gameplay Ability System are about 6,000 to 7,500 tokens) and split a guide that keeps growing

## 4. Milestone 2: vertical slice

**Goal:** one full 28-day season that proves the farming core and a first relationship with the caretaker robot. The town, humans, exploration, combat and survival needs are tested in later milestones. See [GameDesign.md section 13](GameDesign.md#13-scope-and-milestones).

**Done when** a player can start a new game, meet the caretaker robot, grow three or four crops (one of them mutated) in soil with water and radiation to manage, run out of stamina, pay for staying up late, set up a sprinkler, sell the harvest, and play through to the end of the season, and playtests meet the experience criteria in [GameDesign.md section 13](GameDesign.md#next-vertical-slice).

### 4.0 Structure from the code review

The fixes from the [code review](CodeReview.md) of 2026-11-04, in the order to do them. C1 and C2 come before [4.2](#42-farming-depth), because water and radiation on every tile build on the world structure and the tile representation. Each item names its finding; tick the item and add a status line under the finding in the same commit.

Quick wins:

- [x] Move the test keys, F9 (new game) and N (skip day), to a `UVestigeCheatManager` with exec functions compiled out of Shipping, reached from the console ([CodeReview.md, H5](CodeReview.md#h5--f9-deletes-both-saves-with-one-key-in-a-shipped-build); moved here from [section 6](#6-before-a-public-release), and it also ends the F9 screenshot clash in [Open problems](#open-problems)). A real New game comes with the main menu, behind a confirmation
   - Done 2026-11-04: the console commands `NewGame` and `SkipDay` in `UVestigeCheatManager` (`Player/`), set as the template player controller's cheat class outside Shipping; F9 and N are no longer bound. The New game in the menu is part of "Main menu, settings and save slots" in [section 6](#6-before-a-public-release)
- [x] Delete save versions 0 to 3: their upgrades and backups, the legacy fields, `EFarmTool` and `UFarmSaveGame`. Restart at version 1 and delete the development saves; keep the two alternating slots and the refusal to load or overwrite a newer save ([H4](CodeReview.md#h4--save-upgrade-code-for-a-game-nobody-else-has-played))
- [x] First performance baseline: a packaged build of `L_Farm` with a few hundred crop meshes, measured on the hardware closest to the target (RTX 2060 class, 1080p High) and recorded in Architecture.md ([H8](CodeReview.md#h8--rendering-is-unreals-maximum-feature-set-unmeasured-on-the-target); moved here from [4.1](#41-foundations))
   - Done 2026-11-04 on David's PC (RTX 4070 Ti Super, about three times the target's GPU; no older hardware at hand): the console command `BenchmarkField 20 Base.Turnip` makes a field of 400 ripe turnips, and a CSV capture over a whole day gives 7.05 ms per frame, render thread bound, with the GPU at 6.0 ms. On the target the frame is estimated over budget. Numbers and method in [Architecture.md, section 10](Architecture.md#measuring)
- [x] Finish the baseline: run `BenchmarkField 8 Base.Turnip` in the same build to see what the tiles cost the render thread (for [C2](CodeReview.md#c2--one-actor-per-tile-each-with-two-meshes-and-two-dynamic-materials)), measure video memory and set the memory limit, find the 266 ms GPU stall two seconds after the new day, repeat the run that was disturbed, measure the shadow depths again now that the sun turns in steps (0.93 ms before) and the clouds now that High caps them (1.37 ms windowed before, 0.31 ms off screen after), and measure on target-class hardware if any becomes available ([H8](CodeReview.md#h8--rendering-is-unreals-maximum-feature-set-unmeasured-on-the-target))
   - Done 2026-11-05 off screen at 1920x1080 High: the frame (6.65 ms) waits on the GPU's occlusion results, not on the render thread's own work (3.3 ms); 400 tiles cost about 0.4 ms of GPU more than 64; the sun's steps cut the shadow depths from 0.92 to 0.68 ms at the real clock; the clouds cost 0.74 ms (the earlier off-screen runs were at 1280x720); the stall didn't come back. Video memory is 2.6 GB, and the budget now sets 4 GB of video memory and 8 GB of memory. Target-class hardware moved to [section 6](#6-before-a-public-release). Numbers in [Architecture.md, section 10](Architecture.md#measuring)
- [x] One windowed run of the benchmark to check that the 25 to 37 ms frames of the off-screen runs (neither thread nor the GPU busy, one or two a second) come from the off-screen mode and not from the game ([Architecture.md, section 10](Architecture.md#measuring))
   - Done 2026-11-05: the windowed run has none after its first few seconds (7.4 ms p99), so they come from the off-screen mode. Hitches are read from windowed runs from now on
- [x] Move the sun in small steps (for example every 0.25 degrees) instead of every frame, so virtual shadow maps keep their cache ([H8](CodeReview.md#h8--rendering-is-unreals-maximum-feature-set-unmeasured-on-the-target))
   - Done 2026-11-04: the day cycle turns the sun only once the clock has moved it `SunStepDegrees` (0.25, under **Lighting**), while its brightness, colour and the night exposure still change every frame, since only the light's direction resets the cached shadows. At 40 real seconds per game hour that is a turn every 0.4 to 2 seconds instead of every frame
   - 2026-11-05: David saw the steps as small jumps of the shadows once the clock ran at 40 seconds per game hour, and tuned the step to 0.05 degrees (0.1 still looked jittery): a turn every 0.1 to 0.4 seconds. Its cost is measured in "Finish the baseline" above
- [x] Turn off `r.RayTracing` unless the Epic tier is a product goal, then decide from the baseline whether High uses Lumen *(decide)* ([H8](CodeReview.md#h8--rendering-is-unreals-maximum-feature-set-unmeasured-on-the-target))
   - Done 2026-11-04, decided by David: hardware ray tracing is off for the whole project while the game is being developed, so Epic uses software Lumen too, and High keeps Lumen, which cost about 0.6 ms of the 6.0 ms GPU frame in the baseline. Reasons in the [decision log](Architecture.md#16-decision-log)
- [x] Look at the volumetric clouds at High, the costliest GPU pass in the baseline (1.4 ms of 6.0 ms, so roughly 4 ms on the target): lower their quality at High or use a cheaper sky, and compare how mornings and evenings look ([H8](CodeReview.md#h8--rendering-is-unreals-maximum-feature-set-unmeasured-on-the-target))
   - Done 2026-11-04: `Config/DefaultScalability.ini` caps the clouds' samples below Epic (High 48 along the view ray and 5 toward the sun, the engine's 96 and 10). Off screen in the packaged build the cloud pass went from 0.83 ms to 0.31 ms at High; screenshots from 06:30 to 21:00 show the same clouds with slightly grainier edges. Medium and Low are cheaper still. Numbers in [Architecture.md, section 10](Architecture.md#scalability) and the [decision log](Architecture.md#16-decision-log)

Before 4.2:

- [x] Decide the world structure: separate maps for the farm, the town and the zones, or one streamed world, and record it in the decision log *(decide)* ([C1](CodeReview.md#c1--all-game-state-lives-in-the-level-but-the-game-needs-several-maps))
   - Done 2026-11-04, decided by David: separate maps opened with `OpenLevel`; the simulation lives in game-instance subsystems, the player's data crosses a map change as an in-memory snapshot, and interiors are part of their map ([Architecture.md, section 2](Architecture.md#2-unreals-gameplay-framework-where-things-go), [decision log](Architecture.md#16-decision-log))
- [x] The clock and each map's farm data in game-instance subsystems that advance whether or not their map is loaded; `AFarmGrid` becomes a view of the farm data ([C1](CodeReview.md#c1--all-game-state-lives-in-the-level-but-the-game-needs-several-maps))
   - Done 2026-11-04: `UVestigeClockSubsystem` (with `UVestigeClockSettings` in Project Settings) and `UFarmFieldSubsystem` with a `UFarmField` per grid `FieldId`. The grid and its tiles show the field, `AVestigeDayCycle` only lights the level, and saves are version 2. `L_Farm`'s faster clock is gone with the per-level setting ([decision log](Architecture.md#16-decision-log))
- [x] Split the lighting from the clock: a sky actor in each level listens to the clock and owns the sun, the sky light and the night exposure ([M1](CodeReview.md#m1--the-day-cycle-is-both-the-clock-and-the-lighting))
   - Since 2026-11-04 `AVestigeDayCycle` already holds only the lighting and reads `UVestigeClockSubsystem`; what is left is naming it for its job (with a redirect, since `L_Farm` places it) and deciding whether the sunrise and sunset hours belong to the clock
   - Done 2026-11-05: renamed `AVestigeSkyLighting` (a class redirect loads `L_Farm` until it is saved), and the sunrise and sunset hours moved to the clock's Project Settings (**Sun**), the same on every map ([decision log](Architecture.md#16-decision-log))
- [x] Tiles as a `TArray<FFarmTileState>` drawn with one instanced static mesh component per mesh and per-instance custom data for colour and wetness: no actor and no dynamic material per tile ([C2](CodeReview.md#c2--one-actor-per-tile-each-with-two-meshes-and-two-dynamic-materials))
   - Done 2026-11-05 after David approved the guide ([Systems.md, section 22](Systems.md#22-farm-fields-and-tiles)): `UFarmFieldMeshesComponent` on the grid draws the field (the tile states were already an array in `UFarmField` since C1), `AFarmTile` and `BP_FarmTile` are gone, `L_Farm` places `BP_FarmGrid`, and the placeholder colour is custom data read by `M_FarmPlaceholder`. Wetness stays a separate watered set with `M_Soil_Watered` until [4.2](#42-farming-depth) makes water an amount, which needs a soil material that reads custom data. `On Crop Harvested` and `On Tool Used` now carry the tile's coordinates
- [x] A save registry: each saveable owner has a stable key, its own struct and its own version; the save keeps keys it doesn't know (mod data), is read into a game-instance subsystem before the world starts, and owners pull their part when they initialise. Loading then happens once per game, not each time a world begins play as `UVestigeSaveSubsystem` does today, which would roll the clock and the fields back after travel. The same snapshot carries the player's data (inventory, stamina, exhaustion) across a map change: owners write their part before the map closes and read it back in the new one, with a test that the player's data survives reopening the map ([H3](CodeReview.md#h3--the-save-subsystem-knows-every-system-by-name))
   - Done 2026-11-06, approved by David after a review by a senior Unreal C++ reviewer agent: `UVestigeSaveRegistry` and `IVestigeSaveable` in `Core/`, six owners with their own keys and versions (`Base.Clock`, `Base.Fields`, `Base.Inventory`, `Base.Stamina`, `Base.Rest`, `Base.Farming`), the save file at version 3 with records by key, read once per game when the first map is initialised. The open map's owners give their parts as it starts tearing down, and the rest component starts the late-night drain when a map opens after midnight. Test `Vestige.SaveRegistry.ThePlayersDataSurvivesReopeningTheMap` ([Systems.md, section 1](Systems.md#1-saving-and-loading))
- [x] Our own game mode and player controller in a `Game/` folder, outside `Template/`. The game mode owns the end of the day; the rest component only asks for sleep and applies the stamina effects ([M2](CodeReview.md#m2--a-pawn-component-ends-the-worlds-day), [M6](CodeReview.md#m6--the-template-code-is-load-bearing))
   - Done 2026-11-06 after a review by a senior Unreal C++ reviewer agent: `AVestigeGameMode` and `AVestigePlayerController` in `Game/`, the top layer, with `BP_GameMode` (now `GlobalDefaultGameMode`) and `BP_PlayerController` in `Content/Vestige/Blueprints`; the template's Blueprints and their C++ are deleted. The clock ends the day (at 06:00, or early with `EndDay` when the player goes to bed) and keeps how it ended; the game mode starts the next day, with or without a pawn; the rest component wakes the player on `OnNewDay`. Test `Vestige.GameMode.TheNextDayStartsAtTheDayEndWithoutAPlayer` ([decision log](Architecture.md#16-decision-log))
- [x] The rest component on the player state instead of the pawn, so a night with no pawn still refills stamina and keeps the exhaustion in the save: exhaustion and stamina already live on the player state's ability system (found in the review of M2, 2026-11-06)
   - Done 2026-11-06: `AVestigePlayerState` holds it, and the bed and `SkipDay` find it with `UVestigeRestComponent::Find`. Test `Vestige.Rest.ANightWithoutABodyStillRefillsStaminaAndSetsExhaustion`

Before more tools and [4.6](#46-interface-and-input):

- [x] Each tool data asset names the ability that is its behaviour, granted while the tool is in hand; the abilities do the tile work through an interface on the grid, and holding the key repeats inside the ability. Remove `EFarmToolAction` and the switch in the farming component ([H1](CodeReview.md#h1--the-gameplay-ability-system-gates-tools-but-doesnt-run-them))
   - Done 2026-11-07 after two rounds of review by a senior Unreal C++ reviewer agent, whose approval David accepted ([Systems.md, section 9](Systems.md#9-abilities-attributes-and-effects-gameplay-ability-system)): the four tool assets name `UFarmGameplayAbility_Till`, `_Plant`, `_Water` and `_Harvest`, granted by the farming component while the tool is in hand; the base `UVestigeGameplayAbility_UseTool` runs one activation per press with the repeat inside it, and owns `Vestige.State.UsingTool` so one tool use runs at a time. The field's rule functions are the interface to the tiles. Holding E while switching tools now stops the repeat until the next press. Tests `Vestige.Farming.TheToolInHandGrantsItsAbility`, `ANewPawnLeavesNoToolAbilityBehind`, `SwitchingToolsDuringAUseWaitsForTheUseToFinish`, `APressDuringTheLastUsesTimeKeepsTheRepeat`, `ANewToolNeedsOnlyItsAbility` and `Vestige.Content.EveryToolNamesAToolAbility`
- [x] Tunable gameplay effects (tool costs, the late-night drain, exhaustion) as `GE_` data assets instead of C++ classes ([H1](CodeReview.md#h1--the-gameplay-ability-system-gates-tools-but-doesnt-run-them))
   - Done 2026-11-07 after two rounds of review by a senior Unreal C++ reviewer agent, whose approval counts as David's ([Systems.md, section 9](Systems.md#9-abilities-attributes-and-effects-gameplay-ability-system), effects as `GE_` assets; decision log): `GE_StaminaCost`, `GE_StaminaRefill`, `GE_LateNightDrain` and `GE_Exhausted` in `Content/Vestige/Abilities/`, named by the global game data asset `DA_GameData` (`UVestigeGameData`), which `UVestigeAssetManager` loads once at startup; the four C++ effect classes are deleted. The drain's amount per period follows the effect's own period. Tests `Vestige.GameData.NamesEveryEffect` and `ADrainThatIsntInfiniteAndPeriodicIsInvalid`, and the stamina, rest, farming and save tests now use the assets
- [x] Split `UFarmingComponent`: a targeting component for the aim and the target tile, the toolbar into the inventory hotbar, and planting from the seed item in hand instead of one `SeedItemId` ([H2](CodeReview.md#h2--ufarmingcomponent-does-seven-jobs-and-can-plant-only-one-seed))
   - Done 2026-11-07 after three rounds of review by a senior Unreal C++ reviewer agent, whose approval counts as David's ([Systems.md, section 8](Systems.md#8-inventory-and-items), the hotbar; decision log): `UFarmTargetingComponent` on the farmer; the hotbar is the inventory's first nine slots, with the selected slot's item in hand and saved in `Base.Inventory` (version 2); `DA_Item_Hoe` and `DA_Item_WateringCan` are items with a `UVestigeToolFragment`, every seed item names the Seeds tool, and an empty hand harvests (`HandToolId`); the plant ability plants the seed in hand from its slot. `FFarmToolbar` and `Base.Farming` are gone. Tests `Vestige.Inventory.SelectingAHotbarSlotPutsItsItemInHand` and the other hotbar tests, `Vestige.Farming.TheToolOfTheItemInHandIsGranted`, `AFarmerPossessedBeforeBeginPlayFollowsTheHand`, `PlantingUsesTheSeedInHand`, `PlantingTheLastSeedLeavesTheHandEmpty`, `PlantingTheLastSeedOnARepeatLeavesTheHandEmpty`, `ASecondHandChangeDuringTheFirstUseLeavesOneToolAbility`, `Vestige.Item.AToolFragmentNeedsAKnownTool` and `Vestige.Content.EverySeedItemNamesATool`
- [x] Input actions and mapping contexts as `IA_` and `IMC_` assets, bound by gameplay tag through an input config data asset ([H6](CodeReview.md#h6--input-is-built-in-c-with-hard-coded-keys); replaces the "decide when" item in 4.6). The template's moving and looking contexts, added by `AVestigePlayerController` since 2026-11-06, are the body's and move to the pawn's input setup with it
   - Done 2026-11-07 after three rounds of review by a senior Unreal C++ reviewer agent, whose approval counts as David's ([Systems.md, section 5](Systems.md#5-input); decision log): `IA_UseTool`, `IA_Interact`, `IA_HotbarSlot1` to `9`, `IA_HotbarNext`, `IA_HotbarPrevious` and `IMC_Farming` in `Content/Vestige/Input`, `DA_InputConfig_Player` (`UVestigeInputConfig`) naming the action for each native `Vestige.Input.*` tag, and the character binding by tag; `BP_PlayerCharacter` adds `IMC_Default`, `IMC_MouseLook` and `IMC_Farming` while possessed and `AVestigePlayerController::DefaultMappingContexts` is gone. Gamepad X uses, B interacts, every action has player-mappable key settings, and the character releases a held use key when it is unpossessed. Tests `Vestige.Input.ThePlayerBindsEveryConfigEntry`, `EveryFunctionHasAKeyboardAndAGamepadKey`, `NoKeyDoesTwoThings`, `EveryFarmingActionHasAUniqueMappableName`, `ThePlayersConfigValidates`, `AConfigWithADuplicateTagIsInvalid`, `Vestige.PlayerCharacter.WithoutAnInputConfigTheHintsAreEmpty` and `UnpossessingReleasesTheUseKey`
- [x] The HUD in UMG with a C++ base class or view model, before the hotbar ([H7](CodeReview.md#h7--the-hud-is-raw-slate-written-in-c))
   - Done 2026-11-08 after two rounds of review by a senior Unreal C++ reviewer agent, whose approval counts as David's, checked against Epic's "UMG Best Practices" tech blog and documentation page ([Systems.md, section 6](Systems.md#6-user-interface); decision log): `WBP_Hud` (`UVestigeHudWidget`) and `WBP_HudMessage` (`UVestigeHudMessageWidget`) in `Content/Vestige/UI`, `BP_HUD` naming the widget and `BP_GameMode` naming `BP_HUD`. `AVestigeHUD` pushes its event-built texts to the widget, which has no bindings and no tick; only a shown message box ticks, for its fade. The widgets fire Blueprint events after each change for restyling. `SVestigeHudWidget` is gone. Tests `Vestige.UI.TheGameModeShowsTheHudWidget`, `OnlyTheMessageBoxesTick`, `TheHudDoesNotTakeTheMouse`, `EachMessageShowsInItsOwnBox`, `AMessageFadesInAndOut`, `ThePromptHidesWhenEmpty` and `TheStaminaLineWarnsWhileOutOfStamina`
- [ ] Common UI with the first menu ([H7](CodeReview.md#h7--the-hud-is-raw-slate-written-in-c)): reparent the widget bases to `UCommonUserWidget`, set `UCommonGameViewportClient` as the game viewport client, remove the `CommonUI.Debug.CheckGameViewportClientValid` setting and its test `Vestige.Input.TheActionRouterCheckIsOff`, and log the decision ([Systems.md, section 6](Systems.md#6-user-interface)). The plugin and Common Input are already on since the gamepad key hints (2026-11-08)
- [x] Key hints for the gamepad, following the last used device: today the character hints only keyboard and mouse keys (found with H6, 2026-11-07). Kept apart from the UMG move, which changed only the technology (2026-11-08)
   - Done 2026-11-08 after two rounds of review by a senior Unreal C++ reviewer agent, whose approval counts as David's ([Systems.md, section 5](Systems.md#5-input), gamepad key hints; decision log). Done before Common UI with the first menu, which waits for a menu. The hints and the prompt name the keys of the device used last, from Common Input's `UCommonInputSubsystem` (the CommonUI plugin is on; Common UI's widgets and routing wait for the first menu): "X Use tool | B Interact", "LB / RB Hotbar" and "B Sleep" on the gamepad, with Xbox labels in short text (`VestigeKeyHints::GetKeyName`) until icons. Tests `Vestige.PlayerCharacter.GamepadActionHintsNameTheGamepadKeys`, `TheInteractKeyFollowsTheDevice`, `TheGamepadHotbarHintIsTheSteppingKeys`, `Vestige.Input.GamepadKeysHaveShortHintNames`, `ThePlatformDefaultsToKeyboardAndMouse`, `TheActionRouterCheckIsOff` and `Vestige.UI.TheHintDeviceFollowsTheInputType`
- [x] Gameplay reports results and failure reasons (a tag or an enum), and presentation picks the text, sound or effect; no message for a use that worked ([M4](CodeReview.md#m4--gameplay-code-writes-the-players-text))
   - Done 2026-11-08 after two rounds of review by a senior Unreal C++ reviewer agent, whose approval counts as David's ([Systems.md, section 15](Systems.md#15-visual-effects-and-feedback), feedback reports; decision log): `UVestigeFeedbackSubsystem::Report(World, Tag, Arguments)` with 22 native `Vestige.Feedback.*` tags replaces `Notify` with text, and `AVestigeHUD` shows the text `DA_FeedbackResponses` (`UVestigeFeedbackResponses`, on `BP_HUD`) gives each tag. Tool abilities give their reason from `CanUseOn` and the base reports it on the press only. Tilling, planting, watering and harvesting show no message, the morning save shows David's "Game Saved!" from the asset (the design left it empty), and the missing grid is a log warning only. Tests `Vestige.Feedback.ReportsReachEveryListener`, `AnInvalidTagIsNotReported`, `Vestige.Farming.AUseThatWorkedReportsNothing`, `APressReportsWhyAUseChangesNothing`, `Vestige.GameMode.TheReportSaysWhetherThePlayerSleptOrStayedUp`, `Vestige.Stamina.RunningOutReportsOnce`, `Vestige.UI.TheFeedbackResponsesFormatTheReport` and `EveryFeedbackTagHasAResponse`
- [x] One content catalogue loaded once for every inventory, with item icons in their own bundle loaded when a UI needs them ([M5](CodeReview.md#m5--every-inventory-loads-and-keeps-every-item-definition))
   - Done 2026-11-08 after two rounds of review by a senior Unreal C++ reviewer agent, whose approval counts as David's ([Systems.md, section 2](Systems.md#2-content-and-the-asset-manager), the content catalogue; decision log): `UVestigeContentCatalog`, a game-instance subsystem, loads every crop, tool and item with its `Visuals` once per game through a preload handle of its own, and every system finds content there; `LoadWithVisuals` is deleted. Icons go in an `Icons` bundle the catalogue never loads, and the icon itself comes with the hotbar row in [4.6](#46-interface-and-input). All 320 tests pass, and the test run took 22.2 s against 21.8 s before. Tests `Vestige.ContentCatalog.KnowsEveryContentAsset`, `LoadsTheVisualsOfCropsAndTools`, `FindsOnlyTheAskedClass`, `TestContentStaysInItsGame`, `ShuttingDownReleasesTheLoad`, `Vestige.Inventory.InventoriesShareTheCatalogue`, `WithoutACatalogueNoItemIsKnown`, `Vestige.Farming.WithoutACatalogueThereIsNoHarvestTool` and `Vestige.FarmFields.ASavedCropIsFoundInTheCatalogue`
- [x] The late-night drain on the clock's own events instead of a real-time effect period ([M7](CodeReview.md#m7--game-time-events-run-on-real-time))
   - Done 2026-11-11 after two rounds of review by a senior Unreal C++ reviewer agent, whose approval counts as David's ([Systems.md, sections 4 and 9](Systems.md#4-time-days-and-seasons); decision log): the clock broadcasts `OnMinuteStarted` once per whole game minute, before that minute's hour, through skips too; `GE_LateNightDrain` is instant, and the rest component applies it on each minute after midnight with a minute's share of `LateNightDrainPerGameHour`; `SkipToTime` lands half a minute into the typed minute. All 331 tests pass. New tests `Vestige.Clock.EachWholeMinuteIsBroadcastOnceInOrder`, `AMinuteThatStartsAnHourComesBeforeTheHour`, `TheLastMinuteAndHourComeBeforeTheDayEnds`, `AClockJustBeforeAnHourBroadcastsNoHour`, `AdvancingTheClockBroadcastsEachMinuteItPasses`, `RestoringOrStartingADayBroadcastsNoMinute`, `AMinuteThatEndsTheDayIsTheLastTimeBroadcast`, `Vestige.Rest.TheLateNightDrainsItsRatePerGameHour`, `ASkipPastMidnightDrainsAsIfTheTimePassed`, `TheLateNightLeavesNoActiveEffect`, `AMinuteThatEndsTheDayDrainsNothingAfterTheRefill`, `Vestige.Cheats.SkipToTimeReachesTheTypedMinuteAfterMidnight` and `Vestige.GameData.ADrainThatIsntInstantIsInvalid`; the rest tests that check the drain failed on the old code. Play-test in [PlayTest.md](PlayTest.md), "Late-night drain on game time"

When working nearby:

- [ ] Remove the data fields nothing reads (`WaterNeed`, `ContaminationTolerance` on crops, `SellPrice` on items) until their feature is designed ([L2](CodeReview.md#low)). The crops' `Seasons` stays: its tags are the calendar's seasons (2026-11-14), and the seeds' warning and withering items in 4.2 read it next
- [x] Standard ability tasks (wait delay, montages) instead of timers in abilities ([L3](CodeReview.md#low))
   - Done 2026-11-07 with H1: the tool abilities wait with `WaitDelay` and `WaitMovementModeChange`; no other ability used a timer. Montages come with tool animations
- [ ] Blueprint tool abilities, H1's modding half: make `CanUseOn` and `UseOn` Blueprint-implementable, make the field's rules callable from Blueprints, and turn the tool's `Ability` into a soft class reference in its bundle, when mod support is designed ([Systems.md, section 9](Systems.md#9-abilities-attributes-and-effects-gameplay-ability-system), tools as abilities)
- [ ] A cost effect per tool (`CostEffect` on the tool data asset, falling back to the game data's `GE_StaminaCost`), when a tool costs something other than stamina (found with the `GE_` assets, 2026-11-07)
- [ ] Load only the crops of the seeds the player carries, if crops ever number in the dozens: the content catalogue loads every crop with its meshes once per game today (since 2026-11-08). Measure first (found with H2, 2026-11-07)
- [ ] A held tool use resumes after a jump only when the farmer walks again: landing in water or in navigation walking doesn't resume it (releasing E still ends it). Revisit when the farmer can swim or robots use tools
- [ ] Write saves with `AsyncSaveGameToSlot` once they grow ([L4](CodeReview.md#low)). First measure saving and loading on the 60 by 60 benchmark field: each tile is about five tagged properties with their names as text, so a large field is a large record. Saving only the tiles that differ from grass is the likely fix, since tiles carry their coordinates. `AsyncSaveGameToSlot` still builds the bytes on the game thread
- [ ] Start a new game with a save: `NewGame` deletes both slots, so quitting before the first night leaves no save at all. Write a day-1 save once the new map has begun play instead (found in the save registry review, 2026-11-06)
- [ ] The content catalogue sees content added while a game runs (a mod's game feature plugin turned on mid-game) by listening for new primary assets, when mod support is designed; until then it sees it at the next game (found with M5, 2026-11-08)
- [ ] A script that builds and runs the tests before each commit ([L5](CodeReview.md#low))
- [ ] Remove the template's touch controls when the character is reparented ([M6](CodeReview.md#m6--the-template-code-is-load-bearing)): nothing shows them since 2026-11-06, but `BP_PlayerCharacter` still implements `BPI_TouchInterface`, so `Content/Input/Touch/` stays until then
   - The template's `IA_Move`, `IA_Look`, `IA_MouseLook` and `IA_Jump` have no player-mappable names and are bound through the template's own properties; when the template goes they get mappable names (their input tags), join `DA_InputConfig_Player` and are bound by tag, and their contexts move to `Content/Vestige/Input` (found with H6, 2026-11-07)
   - The gamepad layout is provisional: interact (B today) likely moves to A when jumping moves with the template's replacement (H6, 2026-11-07)
- [ ] An automated test that the pawn's mapping contexts don't stack when it is possessed again, if a test map with a local player can run headless (`FMapTestSpawner` on an empty test map, never `L_Farm`, editor builds only); a play-test check until then (found with H6, 2026-11-07)
- [ ] Ability input actions in the input config (abilities activated by input tag), when the first key maps straight to an ability with no owner logic in between ([Systems.md, section 5](Systems.md#5-input); decided with H6, 2026-11-07)

### 4.1 Foundations

The code changes that the slice's content depends on. Doing them first means adding a crop, tool or item later is a new data asset, not new code. Details in [Architecture.md sections 12, 17 and 18](Architecture.md#17-known-gaps).

**Engine** (added 2026-10-30: David is installing Unreal Engine 5.8.4)

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
- [ ] An ID redirect list, once an ID first has to change (deferred, see the decision log)

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
- [ ] Harvests by anything but the player's tools (robots, automation) reach `On Crop Harvested` too: when the first other harvester is built, move the event to the owner that sees every harvest (a change mod Blueprints would have to follow, so cheapest before mods exist)
- [ ] Blueprint-callable main actions and Blueprint-readable state (a tile's state, the clock, inventory counts), as extension points, once a mod or content Blueprint first needs them ([Architecture.md, section 12](Architecture.md#12-designing-for-mods), rule 4)
- [x] An interaction interface (`IVestigeInteractable`) for anything the player can use: bed, shipping bin, water source, robots
   - Done 2026-11-01 ([Systems.md, section 7](Systems.md#7-interaction), decision log): a list of options per interactable, the first used until there is a way to choose; a `UVestigeInteractionComponent` on the pawn that finds the nearest interactable on its own collision channel; interacting as an ability on F, with a prompt on the HUD. The first interactable is the bed ([section 4.3](#43-stamina-and-late-nights))
- [ ] A way to choose between an interactable's options (a menu, or a key per option), when the first object offers two actions at once; until then the interact key uses the first ([Systems.md, section 7](Systems.md#7-interaction))
- [ ] A marker or outline on the focused interactable in the world, as a presentation component, if the HUD prompt alone doesn't make clear what F will use

**Player data**

- [x] Inventory and money move into an inventory component on the player state, so they outlive the character
   - Done 2026-10-31 without money: `UVestigeInventoryComponent` on `AVestigePlayerState` ([Systems.md section 8](Systems.md#8-inventory-and-items)). Seeds and harvests are items, and old saves upgrade. Nothing earns or spends money yet, so it comes with the shipping bin ([section 4.5](#45-automation-and-selling))
- [x] Rename the general `Farm` types to the `Vestige` prefix (decision log, 2026-10-31): the HUD and its widget, settings and messages, the save subsystem and save game, the feedback subsystem, the day cycle, the content base, asset manager, tool and item data and the item fragments, the rest and movement components, the farmer character, and their save structs and delegates. Farm types (tile, grid, crops, the farming component, targeting, the toolbar) keep `Farm`. In its own commits, with `[CoreRedirects]` for every class and struct that assets or save files name, a save test that loads a save naming the old classes, and the documents updated
   - Done 2026-11-01 (decision log). The test showed that a redirect doesn't load an old save's class, so the old save game name stays as an empty `UFarmSaveGame` subclass. The seed fragment, `EFarmToolAction` and the old `EFarmTool` keep `Farm` as farm types
- [x] Whether the data names change too, while no mods exist: the `FarmTool` and `FarmItem` asset types, the `Farm.` tag root (before the stamina tags in [Systems.md section 9](Systems.md#9-abilities-attributes-and-effects-gameplay-ability-system) are made) and `Content/Farm/`
   - Done 2026-11-01 (decision log): David chose to change all three. The asset types are `VestigeTool` and `VestigeItem` (`FarmCrop` stays), the tag root is `Vestige.`, and our content lives in `Content/Vestige/`, moved by a headless editor script with no redirectors left
- [x] Seed and harvest counts become private, with a restore function for loading
- [x] Tunable settings on the tile, grid and day cycle become protected or private

**Logging** (chosen 2026-10-30: build on Unreal's logging instead of a logging tool of our own, which would rebuild what the engine has and lose the Output Log's filters and colours, the log files, crash reports and the `log` console command)

- [x] A logging guide in [Systems.md](Systems.md), written from Epic's logging documentation and Lyra's source, approved by David before any code (AGENTS rule 13). It covers:
   - categories per system, named `LogVestige<System>` (`LogVestigeSave`, `LogVestigeHud`), or `LogFarm<System>` for farm systems, each declared in its layer's folder
   - which verbosity to use when (Error, Warning, Display, Log, Verbose, VeryVerbose)
   - a small macro on top of `UE_LOGFMT` that adds the sending object's name and function to every line
   - log volume: nothing logged every frame, `Verbose` for tracing
   - setting a category's level with the `log` console command and in the `[Core.Log]` section of `DefaultEngine.ini`
   - reading our lines in the Output Log: its filters, and Category Colorization Mode (Editor Preferences > General > Output Log), which gives each category its own colour
   - the Visual Logger (Tools > Debug > Visual Logger) for debugging actors and gameplay over time
   - Done 2026-11-02 ([Systems.md, section 21](Systems.md#21-logging), decision log), approved by David on 2026-11-01: `LogFarm` for the farm, which is one system, instead of `LogFarm<System>`
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
- [ ] A redirected ID test, with the redirect list

**Performance** ([Architecture.md section 10](Architecture.md#10-performance-by-design))

- [x] Confirm the target hardware and the frame budget
- [x] Set the rendering settings and the four scalability levels for the target: hardware ray tracing and other expensive features only at Epic
- [x] The day cycle broadcasts the end of the day, so the farming component stops polling it every frame
- [x] A shipping-safe highlight for the targeted tile, replacing the debug box
- [x] The HUD keeps its references instead of searching the world every frame
- [x] Trace scopes on the new day, loading and saving
- First performance baseline: moved to [4.0](#40-structure-from-the-code-review) (code review, 2026-11-04)
- [ ] Add a performance analyst agent (`vestige-performance`) by the end of milestone 2, when real art and bigger fields arrive: it profiles the packaged build with Unreal Insights and CSV captures, compares the frame against the budget in [Architecture.md, section 10](Architecture.md#10-performance-by-design) and [PerformanceBaseline.md](PerformanceBaseline.md), and finds what costs the most ([CodeReview.md, H8](CodeReview.md#h8--rendering-is-unreals-maximum-feature-set-unmeasured-on-the-target)). Made like the other agents, with David's yes on its instructions (asked by David, 2026-11-12)

### 4.2 Farming depth

- [x] Three or four crops for the season, each with its own meshes
   - Done 2026-11-13: potato (6 days, yield 3) and cabbage (10 days, yield 1) join the turnip, decided by David; their seeds are in the starting bag (4 and 2, on hotbar keys 4 and 5). A crop's few growth stage meshes spread over its days to mature, with a ripe mesh of its own and a small size step each night it grew ([Systems.md, section 22](Systems/22-FarmFields.md)). The mutated crop below makes the fourth
   - 2026-11-13, after David's play-test (the cabbage didn't look grown after 4 sleeps): each crop's data sets the day each stage starts instead of the even spread, the cabbage has 5 growing stages, the potato and cabbage change shape after the first night, and seedlings are sized for the game camera (at least about 20 cm across), decided by David
- [x] Move the turnip's three materials to instances of `M_Crop` (`MI_Crop_Turnip<Part>`, with redirects or fixed-up references), so every crop reads custom data the same way; before contamination or water cues need it (art review, 2026-11-13)
   - Done 2026-11-13: the turnip's slots use `MI_Crop_TurnipLeaf`, `MI_Crop_TurnipRoot` and `MI_Crop_TurnipTop`, and the importer's `M_Turnip*` instances are deleted
- [x] The turnip's leaves (`add_leaf` in `Art/Blender/make_farm_assets.py`) are two coincident sheets joined by a zero-area rim, as the potato's and cabbage's were before their review; build them as a reversed back copy, and give Stage0 and Stage1 more distinct outlines (2 then 3 leaves read alike), in the turnip's first art improvement pass; the turnip meshes and `SM_Soil_Tilled` also carry a generated convex collision hull they don't need, and reimporting them needs the Interchange pipeline override the potato and cabbage import used, or it makes material instances and collision again (art review, 2026-11-13)
   - Done 2026-11-13, with the turnip rebuilt for the game camera after David's play-test: leaves built as a reversed back copy, every stage a new outline, the convex hull gone on the reimport
- [ ] One mutated crop that grows in irradiated soil
- [ ] Normal crops can mutate in irradiated soil *(decide how often)*
- [ ] Radiation level on each tile: slows or stops growth, and can be cleaned
- [ ] A way to clean radiation, such as a tool, fertilizer or a crop that absorbs it
- [ ] Water level on each tile, so watering is a state the tile tracks
- [ ] Clean water as a resource: a water source on the farm that the watering can fills from
- [ ] Dirty water: easy to get, but raises tile radiation
- [x] A 28-day season with the date and the days left shown on the HUD
- [ ] Seeds show their days to harvest, and planting a crop that can't ripen before the season ends gives a warning
- [ ] Out-of-season crops wither at 06:00 on the first day of the next season
   - `OnSeasonStarted`'s listeners run in no set order, so this item decides the order of its messages after the season message ([Systems/04-Time.md](Systems/04-Time.md), pitfalls)
- [ ] A test for the "Welcome back" message's values (`Vestige.Feedback.Save.Loaded`, season and day of the season); the save subsystem builds that report privately, so the test needs a hook (code review, 2026-11-14)

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
   - Done 2026-11-03: David play-tested the numbers and kept them all (100 stamina, the tool costs, the 0.4 s use, 10 per game hour after midnight, the 75% refill, half speed while E is held)
- [ ] Starting stamina from data instead of the constant in `UVestigeStaminaSet` (`VestigeStamina::StartingMaxStamina`), once a second kind of character (a robot) needs other numbers or tuning needs it without a build: for example a starting-values effect granted by the ability set, or Epic's attribute table (`UAttributeSet::InitFromMetaDataTable`). The global game data (`DA_GameData`) can name a starting-values effect
- [ ] When tools get animations, a use's time comes from its montage, played at half speed at zero stamina, instead of `UseSeconds` alone (Systems.md, section 9)
- [x] Show on the HUD that the player is out of stamina, for example the stamina line in red (David, 2026-11-01). The HUD follows the `Vestige.State.OutOfStamina` tag (`RegisterGameplayTagEvent` on the ability system) rather than comparing the number itself, so the rule stays in `UVestigeStaminaSet`, and it changes the line's colour only when the tag changes, never in a per-frame binding
   - Done 2026-11-02 as described, in `AVestigeHUD` and `SVestigeHudWidget` ([PlayTest.md, Pending checks](PlayTest.md#2-pending-checks))
- [x] Holding E repeats the tool use, so walking along a row while holding it works the row (David, 2026-11-01). E is bound to the press only today. Repeat only after a use that changed the farm, so a use that changes nothing (hoeing tilled soil) stops the repeat quietly instead of posting its message every frame; the next use still waits for `UseSeconds`
   - Done 2026-11-02, changed with David: stopping at the first use that changes nothing would stop after a tile or two, because the next use usually lands on the tile just worked. Instead the next use starts when the last ends or the target tile changes, and a use that would change nothing is skipped quietly until the next tile. Only the press posts its message
   - Walking slows to half speed while E is held (David, after play-testing on 2026-11-03), which at 0.4 s per use works every tile of a row; at zero stamina a use takes 0.8 s and every other tile is still skipped
   - Tools don't work in the air, and jumps with E held are slowed too (David, 2026-11-03): otherwise jumping along a row got round the slower walk
- [x] A bed in the farmhouse to sleep in, instead of a key that works anywhere
   - Done 2026-11-01 without the farmhouse: `AVestigeBed` (`BP_Bed`), a placeholder box along the near edge of the field in `L_Farm`, used with F. N stays as a test key, "Skip day (test)", until the cheat manager (decided with David). The bed moves into the farmhouse when there is one, and gets its mesh with [4.6](#46-interface-and-input)
- [ ] Hostile robots at night wait until combat exists ([milestone 3](#combat-and-the-wasteland)). When they come, the midnight warning says "it's dangerous out now" as [GameDesign.md, section 5](GameDesign.md#5-time-days-and-seasons) has it
- [ ] No sleeping before an earliest bedtime, 18:00 as David suggests (2026-11-07), so the player can't skip days one after another *(decide: the hour, and whether the bed hides Sleep before then or offers it and says it's too early)*
   - The rule belongs in `UVestigeRestComponent` with the other late-night hours (an `EarliestSleepHour` setting beside `LateWarningHour` and `LateNightHour`), and `AVestigeBed` asks it in `GetInteractionOptions`, so the bed keeps no rules of its own
   - Times after midnight count as late, not early: the clock's time of day runs past 24 until the day ends at 06:00
   - The `SkipDay` cheat calls `Sleep` too and must skip the rule. The rest tests that sleep in the morning move the clock past the hour first, and a new test covers sleeping too early
   - Add the rule to [GameDesign.md, section 5](GameDesign.md#5-time-days-and-seasons) and its decision log, since it limits "your pace"

### 4.4 The caretaker robot

- [ ] Hob *(working name)* in the game: placeholder body first, then a Blender model
- [ ] Simple data-driven dialogue
- [ ] Characters as data assets, so more robots and humans can be added the same way
- [ ] One relationship track with Hob, who starts as a dutiful servant rather than a sceptic; trust grows through talking, gifts and working together, and the slice arc is in [Hob's character sheet](Spoilers/Characters/Hob.md#vertical-slice-arc)
- [ ] Hob guides the player through the first days as a tutorial

### 4.5 Automation and selling

- [ ] Placing objects on the farm grid
- [ ] A sprinkler that waters nearby tiles each morning from the farm's water store
- [ ] A shipping bin: crops put in it are sold overnight
- [ ] The currency (brass gear coins, name still open) on the HUD
- [ ] A simple way to buy seeds, with one cheap basic seed always for sale
- [ ] Starting seeds and a small purse for a new game
- [ ] A simple budget for an ordinary and a poor season before prices are set

### 4.6 Interface and input

- [ ] The hotbar and the bag on screen, built in UMG with C++ base classes: the hotbar as a row of slots with the selected one marked (its rules are built since 2026-11-07, [4.0](#40-structure-from-the-code-review), H2), and an inventory screen for moving items between slots
   - Item icons come with it: a soft texture on `UVestigeItemData` in an `Icons` bundle of its own, which the content catalogue never loads, preloaded by the row and the screen with a handle each owns while it shows them. Design there the empty or unknown list (no handle), icons already in memory (the callback still comes a frame later) and a callback for a replaced handle ([Systems.md, section 2](Systems.md#2-content-and-the-asset-manager), the content catalogue; M5, 2026-11-08)
- [x] Harvesting ripe crops with F, the interact key, with an "F Harvest" prompt on the faced ripe tile; E only uses the item in hand, and an empty hand is no tool (David, after the hotbar play-test, 2026-11-07)
   - Done 2026-11-07 after three rounds of review by a senior Unreal C++ reviewer agent, whose approval counts as David's ([Systems.md, section 7](Systems.md#7-interaction), harvesting with F; decision log): the farm grid is the interactable for its tiles, the harvest ability is granted all the time (`HarvestToolId`), and the interaction focus goes by facing: the faced tile within 50 degrees of the aim, the bed only when the facing ray hits it. Tests `Vestige.Farming.NextToTheBedTheFacedRipeTileWins`, `NextToTheBedAnUnripeTargetFocusesNothing`, `InteractingWithARipeTileHarvestsIt`, `AnEmptyHandUsesNothing`, `Vestige.Interaction.AnOptionWithoutALocationIsFacedOnlyWhenTheFacingRayHitsItsShape` and the others in the design
- [ ] Holding F to harvest along a row of ripe crops, as holding E repeats a tool (found with harvesting with F, 2026-11-07)
- [ ] A marker on what F would use (an outline on the bed, a highlight on the ripe tile), if the prompt and the tile frame aren't enough in play
- [x] Stepping through the hotbar with the mouse wheel and a gamepad button, with the input assets ([4.0](#40-structure-from-the-code-review), H6); slots beyond the ninth have no key until then
   - Done 2026-11-07 with H6 ([Systems.md, section 8](Systems.md#8-inventory-and-items), the hotbar): the mouse wheel and the gamepad shoulders step to the next or previous hotbar slot, wrapping around and counting empty slots (`UVestigeInventoryComponent::StepHotbarSlot`). Tests `Vestige.Inventory.SteppingTheHotbarWrapsAround`, `SteppingWithoutAHotbarDoesNothing` and `SteppingTheHotbarPutsTheNextItemInHand`
- [ ] A held mesh for items that aren't tools (a turnip in the hand), with the meshes for the new crops
- Input actions as assets: moved to [4.0](#40-structure-from-the-code-review) and decided as now (code review, 2026-11-04)
- [x] A project material for the tile highlight frame (`M_TileHighlight`, unlit), replacing the engine material it uses today
   - Done 2026-10-31, earlier than planned: in 5.8 the engine material drew as the default grey material (Architecture.md, decision log)
- [ ] Ripe crops show that they are ready to harvest, for example small glowing particles hovering around the crop (David, 2026-11-07). The field draws its tiles as instanced meshes with no actor per tile ([4.0](#40-structure-from-the-code-review), C2), so not a particle system per crop: one Niagara system per field fed the ripe tiles' positions when the field changes, or a glow in the crop material driven by per-instance custom data, as the tiles' colour and wetness are. Measure on the 20 by 20 benchmark field of ripe turnips. Design it in [Systems.md, section 15](Systems.md#15-visual-effects-and-feedback) first
- [ ] Meshes for the new crops, the sprinkler, the shipping bin, the water source and the bed
- [ ] A sun and a moon instead of one light that is both: at sunset today `AVestigeSkyLighting` jumps the one directional light from the western horizon to the eastern one and from warm to blue in a single frame, so the shadows and the sky snap at 20:00 (seen by David, 2026-11-06). Instead the sun carries on below the horizon for a real dusk in the sky atmosphere, and a second, dim directional light for the moon (the atmosphere's second sun light) fades in as the sun goes and crosses the sky through the night. Only the brighter of the two casts shadows, so the frame costs about what it does now; measure it against the baseline ([Architecture.md, section 10](Architecture.md#measuring)). Needs a moon light placed in `L_Farm` and tests of the fading
- [ ] The HUD clock shows every game minute instead of 10-minute steps (David, 2026-11-07). At 40 real seconds per game hour the shown time changes about every 0.67 seconds
   - Cheap: the HUD reads the time each frame and rebuilds the status line only when the shown time changes, so the change is `VestigeHudClock::StepMinutes` in `VestigeHUD.cpp` set to 1, a short text rebuilt about 1.5 times a second. Since [M7](CodeReview.md#m7--game-time-events-run-on-real-time) (2026-11-11) the clock broadcasts `OnMinuteStarted`, so with this item the HUD sets the status line from that event and the day events instead of from `AVestigeHUD::Tick`, and stops ticking
   - The 10-minute step was chosen so the clock doesn't flicker every game minute, so check in play that the moving minutes don't distract. Update the clock line in [PlayTest.md](PlayTest.md)

## 5. Milestone 3 and later

Roughly in the order from [GameDesign.md section 13](GameDesign.md#13-scope-and-milestones). Each group becomes its own milestone with a short plan when we get to it.

### Seasons and weather

- [ ] All four seasons (spring, summer, autumn, winter) of 28 days each
- [ ] Crops that don't fit the new season die when it changes
- [ ] More crops for each season
- [ ] Weather: clear, rain that waters crops, dust storms, and radiation storms that raise tile radiation
- [ ] A forecast for the next day, read by the robot weather broadcaster
- [ ] One festival per season in town
- [ ] Crops that regrow, or span several seasons
- [ ] An end-of-season recap screen when a season ends (David, 2026-11-14)
- [ ] A title card that shows the new season on its first morning, taking over from the morning message as the season's announcement (David, 2026-11-14)

### Energy and the land

- [ ] Simulate a best-case and a worst-case season for land health and energy before building them ([GameDesign.md section 6](GameDesign.md#land-health-and-the-haze))
- [ ] Solar panels that charge power cells, with output depending on time of day, weather and haze
- [ ] Machines that draw charge and stop when it runs out
- [ ] Land health for each map, raised by healthy farming and lowered by pollution
- [ ] The haze: thinner with healthy land, giving more solar power and a brighter sky
- [ ] An easy-to-read land health meter
- [ ] "Dirty" shortcuts (dirty water, polluting machines) that are faster now and costlier later

### The robot town

- [ ] Separate maps with loading between them: the farm, the town, later the wasteland zones. Opened with `OpenLevel` as [Architecture.md, section 2](Architecture.md#2-unreals-gameplay-framework-where-things-go) decides: an owner for travel in the game instance, zones and hubs as data assets naming their maps, and a loading screen
- [ ] The robot town map
- [ ] Add a level designer agent (`vestige-level-designer`) with the second map: it lays out maps from the design (paths, landmarks, where things are placed, sight lines from the game camera), within the performance budget and the content rules. Made like the other agents, with David's yes on its instructions (asked by David, 2026-11-12)
- [ ] Townsfolk with schedules, likes and dislikes, and their own backstories (starting with the administrator, the clinic robot, the mail carrier, the engineer and the weather broadcaster)
- [ ] Gifts, requests and personal events
- [ ] Romance, dating and marriage with robots and humans; a partner moves to the farm
- [ ] Recruiting befriended robots to work on the farm
- [ ] Town restoration, the main story: repair the water pump, power, clinic and market, each unlocking something new
- [ ] Shops

### Progression

- [ ] Skills that level up by doing: farming, engineering, scavenging, combat
- [ ] Tool tiers: rusty, pre-war and high-tech, unlocked through the tech tree
- [ ] The tech tree, as data assets, with branches for automation, tools, water and soil machines, energy and robot upgrades
- [ ] Automation from sprinklers to robot farmhands to a fully automated farm

### Building and crafting

- [ ] Buildings and machines on the farm grid: greenhouse, water purifier, rain collector, robot charging station, storage
- [ ] Crafting stations and recipes, as data assets
- [ ] Decorating the house

### Survival needs

- [ ] Hunger, thirst and radiation sickness, kept in check by eating crops and drinking clean water
- [ ] Neglected needs lower stamina and work speed, but never kill the player
- [ ] The late-night drain rate as an attribute that gameplay effects can modify, built when the first such modifier is designed. Example: eating a healthy meal during the day applies a `GE_WellFed` effect, lasting until the next morning, that drains 30% less that night. Today the rest component sets a minute's share of its `LateNightDrainPerGameHour` setting on each game minute's drain, so nothing else can change the rate. The fix: the rate becomes an attribute (for example `LateNightDrainRate` on `UVestigeStaminaSet`, starting at 10 per game hour), and `GE_LateNightDrain`'s magnitude comes from a small magnitude calculation class (`UGameplayModMagnitudeCalculation`) that reads the current rate on every application and takes a minute's share of it, as the rest component does now. Meals, perks and robot upgrades then each become a `GE_` asset with a modifier on the rate, and stack with no new code. The quicker alternative, a `Vestige.State.WellFed` tag that the rest component scales its rate by, fits one or two cases but needs code for each new one (David, 2026-11-07; [Systems.md, section 9](Systems.md#9-abilities-attributes-and-effects-gameplay-ability-system))
- [ ] A setting to switch survival needs off, with nothing else depending on them

### Combat and the wasteland

- [ ] Simple real-time melee combat, starting with a wrench; ranged weapons later
- [ ] Hostile robots on the farm at night that attack the player, never crops or buildings
- [ ] Losing a fight: wake at home with a penalty
- [ ] The first wasteland zone
- [ ] Scavenging for materials
- [ ] More zones, each with a reason in the history: a ruined city, a research facility, a Cinderium mine, a dam

### Story

- [ ] Add a lore and story agent (`vestige-lore-keeper`) for the documents in `Docs/Spoilers/`: it keeps the lore, the story timeline, the characters, the factions and the dialogue consistent with each other and with `Docs/GameDesign.md`, writes and edits them in the project's tone, and turns [LoreAudit.md](Spoilers/LoreAudit.md) findings into fixes. Lore stays out of the code flow (it skips the steps in "How work is done"), but story content that reaches the game (dialogue, item texts, quests) is written or checked by this agent before it goes through the flow. It also keeps spoilers out of everything that must stay spoiler-free (the other docs, code comments and commit messages, as [Spoilers/README.md](Spoilers/README.md) says). Made like the other agents, with David's yes on its instructions (asked by David, 2026-11-12)
- [ ] The family mystery: letters, recordings and Hob's returning memories
- [ ] The humans, about four to six, each found as a story moment
- [ ] The parents' sealed message, delivered when the farm is whole again
- [ ] The opening: waking from the cryo pod under the farmhouse

### Later still

- [ ] Animals: mutated livestock
- [ ] Other camera angles, such as a higher angled view
- [ ] Mod support: base content as a Game Feature plugin, loading mods from a folder, load order, a mod kit, and letting mods override base content
- [ ] Co-op multiplayer, which needs its own design and changes to the architecture
- [ ] Gamepads that aren't Xbox-style (Switch-style pads such as HORI's, the Stadia controller and other plain HID pads) through Unreal's GameInput plugin, which maps a raw HID pad by its USB vendor and product ID. Until then only XInput pads work on Windows, as in most PC games, and Steam Input covers the rest for players. A nice to have, not needed yet (David, 2026-11-08, after the gamepad key hints)

### Open problems
> Anything minor we have been putting off.

- [x] F9 seems to be a hotkey for taking screenshots in the editor, noticed it when starting new game.
   - Done 2026-11-04: F9 is no longer bound; a new game is the `NewGame` console command ([4.0](#40-structure-from-the-code-review))

## 6. Before a public release

- [ ] When localization starts, run the text gather once and re-enter in the editor any display name it misses: the potato's and cabbage's crop and item names were set from editor Python, and it isn't known whether those texts got localization keys like the ones typed in the Details panel (code review, 2026-11-13)
- [ ] Raise the art to release quality in the game's one stylized style ([GameDesign.md, section 12](GameDesign.md#12-presentation)): improve the low-poly first pass in place, keeping content IDs, sizes and pivots. Paying artists for it needs David's yes on the money and a contract, checked by a lawyer, that gives him the copyright (or an exclusive license) and accepts public release of the sources
- [ ] Decide with the final art whether Epic gets hardware ray tracing back (`r.RayTracing`, off since 2026-11-04 while the game is developed). It costs about 50 MB of shaders and longer cooks; measure Epic on the target and on a high-end PC first ([Architecture.md, decision log](Architecture.md#16-decision-log))
- [ ] Replace the Epic template code and content (character, animations, input) with our own
- [ ] Measure the performance baseline on target-class hardware (an RTX 2060 or RX 5600 XT PC with a 6-core CPU and 16 GB, [Architecture.md, section 10](Architecture.md#budget)) when one is available; until then the target's numbers are estimates from David's PC, which has about three times its GPU ([CodeReview.md, H8](CodeReview.md#h8--rendering-is-unreals-maximum-feature-set-unmeasured-on-the-target))
- [ ] Scale the sun's step with the shadow quality: a console variable for the step, set per `ShadowQuality` level in `Config/DefaultScalability.ini` (0.05 at High and Epic, larger at Low and Medium, where small jumps are the price of cheaper shadows), with `SunStepDegrees` on the day cycle as the level's default. Part of the graphics settings, not an option of its own (David's idea, 2026-11-05)
- [ ] Main menu, settings and save slots. Its New game asks for confirmation before it replaces a save ([CodeReview.md, H5](CodeReview.md#h5--f9-deletes-both-saves-with-one-key-in-a-shipped-build))
   - Key rebinding in the settings, with Enhanced Input's user settings ([Systems.md, section 17](Systems.md#17-settings)): `bEnableUserSettings` on, the screen calls `SaveSettings()` after each remap, and the key hints ask Enhanced Input for the player's current key instead of the assets' default mappings and are rebuilt on a remap event ([Systems.md, section 5](Systems.md#5-input); found with H6, 2026-11-07)
- [ ] Add a UI designer agent (`vestige-ui-designer`) with the first menu: it designs screens and the HUD for the player (layout, readability at a distance, gamepad navigation, accessibility, the pre-war interface style in [GameDesign.md, section 12](GameDesign.md#12-presentation)) before the architect designs them in UMG and Common UI. Made like the other agents, with David's yes on its instructions (asked by David, 2026-11-12)
- Moving the test keys F9 and N into a cheat manager: moved to [4.0](#40-structure-from-the-code-review), because F9 deletes the saves (code review, 2026-11-04)
- [ ] Turn save compatibility on at the first public build: a version per saved owner, with upgrades from then on ([CodeReview.md, H4](CodeReview.md#h4--save-upgrade-code-for-a-game-nobody-else-has-played))
- [ ] Music and sound effects
- [ ] Add an audio designer agent (`vestige-audio`) with the first sounds: it plans the sound and music for each system (feedback responses, gameplay cues, ambience), the mixing and the audio budget, from [Systems/14-Audio.md](Systems/14-Audio.md), and checks licenses for every sound. Made like the other agents, with David's yes on its instructions (asked by David, 2026-11-12)
- [ ] Settle how many feedback messages the HUD shows at once (Project Settings > Game > Vestige HUD; five while the game is being built, 2026-10-30)
- [ ] Final names for the game, the world, the town, the coin, the caretaker and the robot makers
- [x] License headers (an SPDX line) in our own source files
- [ ] A lawyer's check of the licensing setup
- [ ] Make the GitHub repository public, so contributors can work on it as the licensing intends
- [ ] Packaged builds and store pages
- [ ] Add a release engineer agent (`vestige-release`) before the first public build: it packages and checks builds (cook warnings, Shipping-only problems, save compatibility, the licensing files that must ship with the build) and keeps the release checklist. Made like the other agents, with David's yes on its instructions (asked by David, 2026-11-12)

## 7. Open decisions

Collected from the other documents. Each one is settled in its own document and decision log.

- **Land health:** one value per map or varying across it, how fast it changes, and whether damage spreads. ([GameDesign.md](GameDesign.md#15-open-questions))
- **Energy balance:** how much charge panels give and machines use. ([GameDesign.md](GameDesign.md#15-open-questions))
- **Mutated crops:** which effects are good and which are risky, and how often normal crops mutate. ([GameDesign.md](GameDesign.md#15-open-questions))
- **Knock-out penalty:** what exactly is lost. ([GameDesign.md](GameDesign.md#15-open-questions))
- **Multiplayer:** shared farm or separate farms, shared clock and sleep. ([GameDesign.md](GameDesign.md#15-open-questions))
- **Names:** the game, the world, the town, the coin, the caretaker, the robot makers. ([Lore.md](Spoilers/Lore.md#14-open-questions))
- **Story:** the sibling's survival path, role and whether the story ever says who struck first. ([Lore.md](Spoilers/Lore.md#14-open-questions))
