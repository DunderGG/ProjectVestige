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
  1. The rest of [section 4.3](#43-stamina-and-late-nights): tuning stamina and the late-night numbers in play. The late-night rules, the red stamina line and holding E are done (2026-11-02); starting stamina from data and montage timing wait for their triggers. Logging, Blueprint events, the interaction interface and the bed are done (2026-11-01 and 2026-11-02); the rest of [section 4.1](#41-foundations) waits for a packaged build (the performance baseline) or for the need that starts it (the ID redirect list, more Blueprint extension points, a second interaction option).

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
- [ ] Stop loading the tile highlight's material by path in C++: `UFarmTargetHighlightComponent`'s constructor finds `/Game/Vestige/Materials/M_TileHighlight` with `ConstructorHelpers`, which [CodingGuidelines.md, section 12.4](CodingGuidelines.md#124-assets-and-loading) allows only for engine placeholders, and which made the editor refuse to move the folder until the path changed (found in the `Vestige` rename, 2026-11-01). Set `HighlightMaterial` on `BP_PlayerCharacter`'s highlight component instead, keep the engine cube as the C++ default, and keep the test that checks the material's instanced mesh flag
- [ ] Rename the Details panel categories of the general types: the day cycle, rest component, farmer and content data still group their settings under `Farming` (`Farming|Time`, `Farming|Lighting`), and [CodingGuidelines.md, section 6](CodingGuidelines.md#6-uproperty-ufunction-and-blueprint-exposure) still says every category starts with `Farming`. Decide the rule (for example `Time`, `Lighting`, `Input` without a project prefix, and `Farming` only for farm types), then change the code and the guide together. Categories are display text only, so nothing in assets or saves changes. The Blueprint extension points (`On New Day`, Get Day Cycle, Get Content Id) use the same categories, so their place in the Blueprint menus changes with it
- [ ] Update the class names in the lore drafts (`Docs/Spoilers/Characters/`): they name `UFarmCharacterData`, which the planned design now calls `UVestigeCharacterData` ([Systems.md](Systems.md))

## 4. Milestone 2: vertical slice

**Goal:** one full 28-day season that proves the farming core and a first relationship with the caretaker robot. The town, humans, exploration, combat and survival needs are tested in later milestones. See [GameDesign.md section 13](GameDesign.md#13-scope-and-milestones).

**Done when** a player can start a new game, meet the caretaker robot, grow three or four crops (one of them mutated) in soil with water and radiation to manage, run out of stamina, pay for staying up late, set up a sprinkler, sell the harvest, and play through to the end of the season, and playtests meet the experience criteria in [GameDesign.md section 13](GameDesign.md#next-vertical-slice).

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
- [ ] First performance baseline, measured in a packaged build on the target hardware and recorded in Architecture.md

### 4.2 Farming depth

- [ ] Three or four crops for the season, each with its own meshes
- [ ] One mutated crop that grows in irradiated soil
- [ ] Normal crops can mutate in irradiated soil *(decide how often)*
- [ ] Radiation level on each tile: slows or stops growth, and can be cleaned
- [ ] A way to clean radiation, such as a tool, fertilizer or a crop that absorbs it
- [ ] Water level on each tile, so watering is a state the tile tracks
- [ ] Clean water as a resource: a water source on the farm that the watering can fills from
- [ ] Dirty water: easy to get, but raises tile radiation
- [ ] A 28-day season with the date and the days left shown on the HUD
- [ ] Seeds show their days to harvest, and planting a crop that can't ripen before the season ends gives a warning
- [ ] Out-of-season crops wither at 06:00 on the first day of the next season

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
- [ ] Tune the stamina numbers in play: 100 stamina, the tool costs and the 0.4 s use time ([PlayTest.md, Stamina](PlayTest.md#2-pending-checks)), and the late-night drain (10 per game hour) and the 75% exhausted refill, which are settings on the farmer's Rest component
- [ ] Starting stamina from data instead of the constant in `UVestigeStaminaSet` (`VestigeStamina::StartingMaxStamina`), once a second kind of character (a robot) needs other numbers or tuning needs it without a build: for example a starting-values effect granted by the ability set, or Epic's attribute table (`UAttributeSet::InitFromMetaDataTable`)
- [ ] When tools get animations, a use's time comes from its montage, played at half speed at zero stamina, instead of `UseSeconds` alone (Systems.md, section 9)
- [x] Show on the HUD that the player is out of stamina, for example the stamina line in red (David, 2026-11-01). The HUD follows the `Vestige.State.OutOfStamina` tag (`RegisterGameplayTagEvent` on the ability system) rather than comparing the number itself, so the rule stays in `UVestigeStaminaSet`, and it changes the line's colour only when the tag changes, never in a per-frame binding
   - Done 2026-11-02 as described, in `AVestigeHUD` and `SVestigeHudWidget` ([PlayTest.md, Pending checks](PlayTest.md#2-pending-checks))
- [x] Holding E repeats the tool use, so walking along a row while holding it works the row (David, 2026-11-01). E is bound to the press only today. Repeat only after a use that changed the farm, so a use that changes nothing (hoeing tilled soil) stops the repeat quietly instead of posting its message every frame; the next use still waits for `UseSeconds`
   - Done 2026-11-02, changed with David: stopping at the first use that changes nothing would stop after a tile or two, because the next use usually lands on the tile just worked. Instead the next use starts when the last ends or the target tile changes, and a use that would change nothing is skipped quietly until the next tile. Only the press posts its message
   - Walking slows to half speed while E is held (David, after play-testing on 2026-11-03), which at 0.4 s per use works every tile of a row; at zero stamina a use takes 0.8 s and every other tile is still skipped
- [x] A bed in the farmhouse to sleep in, instead of a key that works anywhere
   - Done 2026-11-01 without the farmhouse: `AVestigeBed` (`BP_Bed`), a placeholder box along the near edge of the field in `L_Farm`, used with F. N stays as a test key, "Skip day (test)", until the cheat manager (decided with David). The bed moves into the farmhouse when there is one, and gets its mesh with [4.6](#46-interface-and-input)
- [ ] Hostile robots at night wait until combat exists ([milestone 3](#combat-and-the-wasteland)). When they come, the midnight warning says "it's dangerous out now" as [GameDesign.md, section 5](GameDesign.md#5-time-days-and-seasons) has it

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

- [ ] An inventory or hotbar for several seed and crop types, built in UMG with C++ base classes
- [ ] Input actions as assets once there are more keys than the prototype's handful *(decide when)*
- [x] A project material for the tile highlight frame (`M_TileHighlight`, unlit), replacing the engine material it uses today
   - Done 2026-10-31, earlier than planned: in 5.8 the engine material drew as the default grey material (Architecture.md, decision log)
- [ ] Meshes for the new crops, the sprinkler, the shipping bin, the water source and the bed

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

### Energy and the land

- [ ] Simulate a best-case and a worst-case season for land health and energy before building them ([GameDesign.md section 6](GameDesign.md#land-health-and-the-haze))
- [ ] Solar panels that charge power cells, with output depending on time of day, weather and haze
- [ ] Machines that draw charge and stop when it runs out
- [ ] Land health for each map, raised by healthy farming and lowered by pollution
- [ ] The haze: thinner with healthy land, giving more solar power and a brighter sky
- [ ] An easy-to-read land health meter
- [ ] "Dirty" shortcuts (dirty water, polluting machines) that are faster now and costlier later

### The robot town

- [ ] Separate maps with loading between them: the farm, the town, later the wasteland zones
- [ ] The robot town map
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
- [ ] A setting to switch survival needs off, with nothing else depending on them

### Combat and the wasteland

- [ ] Simple real-time melee combat, starting with a wrench; ranged weapons later
- [ ] Hostile robots on the farm at night that attack the player, never crops or buildings
- [ ] Losing a fight: wake at home with a penalty
- [ ] The first wasteland zone
- [ ] Scavenging for materials
- [ ] More zones, each with a reason in the history: a ruined city, a research facility, a Cinderium mine, a dam

### Story

- [ ] The family mystery: letters, recordings and Hob's returning memories
- [ ] The humans, about four to six, each found as a story moment
- [ ] The parents' sealed message, delivered when the farm is whole again
- [ ] The opening: waking from the cryo pod under the farmhouse

### Later still

- [ ] Animals: mutated livestock
- [ ] Other camera angles, such as a higher angled view
- [ ] Mod support: base content as a Game Feature plugin, loading mods from a folder, load order, a mod kit, and letting mods override base content
- [ ] Co-op multiplayer, which needs its own design and changes to the architecture

### Open problems
> Anything minor we have been putting off.

- [ ] F9 seems to be a hotkey for taking screenshots in the editor, noticed it when starting new game.

## 6. Before a public release

- [ ] Realistic art direction: Lumen, Nanite and Megascans for the world, custom Blender work for robots, machines and crops. Replace the low-poly placeholders
- [ ] Replace the Epic template code and content (character, animations, input) with our own
- [ ] Main menu, settings and save slots
- [ ] Move the test keys, F9 (new game) and N (skip day), into a `UVestigeCheatManager` or the menus ([Systems.md, section 19](Systems.md#19-debugging-and-cheats))
- [ ] Music and sound effects
- [ ] Settle how many feedback messages the HUD shows at once (Project Settings > Game > Vestige HUD; five while the game is being built, 2026-10-30)
- [ ] Final names for the game, the world, the town, the coin, the caretaker and the robot makers
- [x] License headers (an SPDX line) in our own source files
- [ ] A lawyer's check of the licensing setup
- [ ] Make the GitHub repository public, so contributors can work on it as the licensing intends
- [ ] Packaged builds and store pages

## 7. Open decisions

Collected from the other documents. Each one is settled in its own document and decision log.

- **Land health:** one value per map or varying across it, how fast it changes, and whether damage spreads. ([GameDesign.md](GameDesign.md#15-open-questions))
- **Energy balance:** how much charge panels give and machines use. ([GameDesign.md](GameDesign.md#15-open-questions))
- **Mutated crops:** which effects are good and which are risky, and how often normal crops mutate. ([GameDesign.md](GameDesign.md#15-open-questions))
- **Knock-out penalty:** what exactly is lost. ([GameDesign.md](GameDesign.md#15-open-questions))
- **Multiplayer:** shared farm or separate farms, shared clock and sleep. ([GameDesign.md](GameDesign.md#15-open-questions))
- **Names:** the game, the world, the town, the coin, the caretaker, the robot makers. ([Lore.md](Spoilers/Lore.md#14-open-questions))
- **Story:** what the sealed message says, whether the sibling exists, and whether the story ever says who struck first. ([Lore.md](Spoilers/Lore.md#14-open-questions))
