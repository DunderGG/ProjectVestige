# Code and architecture review

**Reviewed:** 2026-11-04  
**Scope:** A critical review of the game's C++ code and architecture, done as a senior Unreal Engine game developer would review a new team's codebase. It judges the code against Unreal practice first, and against this project's own rules ([Architecture.md](Architecture.md), [CodingGuidelines.md](CodingGuidelines.md), [Systems.md](Systems.md)) second, and says where a project rule is itself the problem. Earlier reviews are in [ArchitectureAudit.md](ArchitectureAudit.md) and [CodingGuidelinesAudit.md](CodingGuidelinesAudit.md).

Every fix is a roadmap item in [Roadmap.md, section 4.0](Roadmap.md#40-structure-from-the-code-review), and the changes to project rules are in [section 3](Roadmap.md#3-housekeeping). When a finding is fixed, add a "**Status: Resolved (date).**" line under it and tick its roadmap item.

## Summary

The line-level code is good, better than most indie Unreal code: small classes, rules in plain structs, 181 behaviour-named tests, content as data assets with validated IDs, and events instead of ticking. The problems are structural, and two of them get more expensive with every feature built on top. The game's whole state lives inside one level while the design needs several maps (C1), and the farm is one actor per tile while roadmap 4.2 is about to add water and radiation to every tile (C2). Both should be settled before more farming features go in.

The second theme is that effort is pointed at the wrong level. A lot of care goes into micro rules (a 5 Hz scan, cached HUD strings, save upgrades for saves only the maintainer has) and into documents that are now about twice the size of the code, while the big decisions (world structure, tile representation, rendering budget, how tools work) have been deferred. Several choices were also shaped by what an AI agent can make from code (input, the HUD and gameplay effects all built in C++ to avoid editor assets) rather than by normal Unreal practice.

| ID | Finding | Severity | Fix when |
|---|---|---|---|
| [C1](#c1--all-game-state-lives-in-the-level-but-the-game-needs-several-maps) | All game state lives in the level, but the game needs several maps | Critical | Before 4.2 |
| [C2](#c2--one-actor-per-tile-each-with-two-meshes-and-two-dynamic-materials) | One actor per tile, each with two meshes and two dynamic materials | Critical | Before 4.2 |
| [H1](#h1--the-gameplay-ability-system-gates-tools-but-doesnt-run-them) | The Gameplay Ability System gates tools but doesn't run them; tool actions are a fixed enum | High | Before more tools |
| [H2](#h2--ufarmingcomponent-does-seven-jobs-and-can-plant-only-one-seed) | `UFarmingComponent` does seven jobs and can plant only one seed | High | With the 4.2 crops |
| [H3](#h3--the-save-subsystem-knows-every-system-by-name) | The save subsystem knows every system by name | High | With C1 |
| [H4](#h4--save-upgrade-code-for-a-game-nobody-else-has-played) | Save upgrade code for a game nobody else has played | High | Now |
| [H5](#h5--f9-deletes-both-saves-with-one-key-in-a-shipped-build) | F9 deletes both saves with one key in a shipped build | High | Now |
| [H6](#h6--input-is-built-in-c-with-hard-coded-keys) | Input is built in C++ with hard-coded keys | High | Before 4.6 |
| [H7](#h7--the-hud-is-raw-slate-written-in-c) | The HUD is raw Slate written in C++ | High | Before 4.6 |
| [H8](#h8--rendering-is-unreals-maximum-feature-set-unmeasured-on-the-target) | Rendering is Unreal's maximum feature set, unmeasured on the target hardware | High | Before the art scales up |
| [M1 to M8](#medium) | The day cycle mixes clock and lighting, a pawn component ends the world's day, the linear layer rule, gameplay code writing UI text, per-inventory item loading, template code still load-bearing, real-time timers for game time, the documentation load | Medium | Milestone 2 |
| [L1 to L5](#low) | Two class prefixes, unused data fields, home-made GAS idioms, synchronous saving, no CI | Low | When working nearby |

Severity says how expensive a problem gets if it is left: **Critical** gets more expensive with every feature built on top, **High** should be fixed in milestone 2, **Medium** when working nearby, **Low** is a judgement call.

## Critical findings

### C1 — All game state lives in the level, but the game needs several maps

The clock is a placed actor (`AVestigeDayCycle`), the farm is a placed actor that spawns tile actors (`AFarmGrid`), and saving and player messages are world subsystems. Opening another map destroys all of it. The design needs a farm, a robot town, wasteland zones and interiors ([Roadmap.md, section 5](Roadmap.md#the-robot-town): "Separate maps with loading between them"), crops that keep growing while the player is in town, sprinklers and robots that work the farm overnight, and land health for each map computed from its tiles. None of that can work while the farm only exists when its level is loaded. The planned fix in [Architecture.md, section 18](Architecture.md#18-expected-changes) (move only the active save slot to a game-instance subsystem) is not enough.

**Fix.** Decide the world structure now, as a decision log entry. Recommended: keep the simulation outside the level. A `UGameInstanceSubsystem` owns the clock and each map's farm data (an array of tile states per map) and advances them whether or not the map is loaded. Levels become views: `AFarmGrid` reads the farm data and draws it. This works with separate maps and with a single streamed world, so it doesn't lock in the level layout.

**Status: World structure decided (2026-11-04).** David chose separate maps opened with `OpenLevel`. The clock and each map's farm data go into game-instance subsystems, the player's data crosses a map change as an in-memory snapshot built with the save registry (H3), and interiors are part of their outdoor map. Moving the clock and the farm data is the next item in [Roadmap.md, 4.0](Roadmap.md#40-structure-from-the-code-review). Reasons in [Architecture.md, decision log](Architecture.md#16-decision-log).

**Status: Clock and farm data moved (2026-11-04).** `UVestigeClockSubsystem` holds the day and the clock and ticks with whichever game world is open, with its speed and hours in `UVestigeClockSettings`. `UFarmFieldSubsystem` holds one `UFarmField` per field ID and grows every field on each new day, loaded or not. `AFarmGrid` and its tile actors are views of their field, and `AVestigeDayCycle` keeps only the lighting (the rest of M1 is its own item). The save holds one entry per field. Loading still happens each time a world begins play, which H3 moves to the game instance before any travel. Reasons in [Architecture.md, decision log](Architecture.md#16-decision-log).

### C2 — One actor per tile, each with two meshes and two dynamic materials

`AFarmGrid` spawns a full `AFarmTile` actor for every tile, grass included. Each has a root, two `UStaticMeshComponent`s and two `UMaterialInstanceDynamic`s created in `BeginPlay`, even when real art meshes are used. The 8 x 8 test field is 64 actors, 128 mesh components and 128 unique materials. A real farm ("the biggest map", [GameDesign.md](GameDesign.md)) is thousands of tiles. Unique dynamic materials also stop Unreal from merging those draws.

[Architecture.md, section 10](Architecture.md#design-rules) says to change this only when a profile says so. That rule is wrong here: the cost is known in advance, C1 needs tile state as data anyway, and 4.2 (water and radiation on every tile) and 4.5 (placing objects on the grid) are about to build on whatever representation exists. A profile of an 8 x 8 field will always say tiles are fine.

**Fix.** The grid (or the C1 subsystem) owns a `TArray<FFarmTileState>`. Tiles are drawn with one instanced static mesh component per mesh (soil states, crop growth stages), and colour or wetness goes into per-instance custom data instead of dynamic materials. Actors are spawned only for things that need to be actors, such as a placed sprinkler. `FFarmTileState` already keeps the rules apart from the visuals, so the rules and their tests survive unchanged.

**Status: Measured (2026-11-05).** In the packaged build, 400 tile actors with a crop each cost the GPU about 0.4 ms more than 64 and the render thread nothing measurable; the first baseline's slow render thread was waiting for the GPU's occlusion results. As this finding predicted, a field this small doesn't show what thousands of tiles cost, so the fix stands on the reasons above. Numbers in [Architecture.md, section 10](Architecture.md#measuring).

**Status: Resolved (2026-11-05).** Tiles are data drawn with instances: `UFarmFieldMeshesComponent` on the grid has one instanced static mesh component per mesh and material and puts the placeholder colour in per-instance custom data, with no actor, component or dynamic material per tile. Watered soil stays a separate instance set until 4.2 makes water an amount. Guide in [Systems.md, section 22](Systems.md#22-farm-fields-and-tiles), reasons in [Architecture.md, decision log](Architecture.md#16-decision-log). On a 60 by 60 field of ripe turnips the frame is unchanged (Nanite already drew the actors together), building the field takes 23 to 33 ms of game thread instead of 237 to 270, and the new day 13 ms instead of 11 ([Architecture.md, section 10](Architecture.md#measuring)).

## High findings

### H1 — The Gameplay Ability System gates tools but doesn't run them

The farming component activates the use-tool ability by tag, the ability calls back into the farming component (`UseToolOnTarget`), and the component switches on the hard-coded `EFarmToolAction` enum. So the ability system is a cooldown and a cost wrapper around a switch statement, and the two classes know each other in a circle. The project pays the ability system's complexity without its main benefit. It also breaks the modding goal ([Architecture.md, section 12](Architecture.md#12-designing-for-mods)): a mod can add a hoe with new numbers, but not a tool that does something new, and [GameDesign.md](GameDesign.md) already plans one (a tool that cleans radiation).

The gameplay effects are C++ classes because editor scripting couldn't author effect modifiers (decision log, 2026-11-01). That was a reason about the agent's tooling, not about the game: designers and modders can't tune C++ effects.

**Fix.** Each tool data asset names the ability that is its behaviour (`TSubclassOf<UGameplayAbility>`), granted while the tool is in hand. The abilities do the tile work through a small interface on the grid. Holding the key to repeat lives inside the ability (wait for the input release, loop) instead of re-activating from the component's `OnAbilityEnded` handler. Tunable effects become `GE_` data assets. `EFarmToolAction` and the switch go away.

### H2 — `UFarmingComponent` does seven jobs and can plant only one seed

In about 450 lines it owns the toolbar, the seed choice, tile targeting, the tool rules, hold-to-repeat, inventory transactions, player messages and the Blueprint events. It plants whatever its one `SeedItemId` names, so the three or four crops of 4.2 can't all be planted.

**Fix.** Split it: a targeting component (the aim and the target tile, which the highlight already listens to), the toolbar folded into the inventory hotbar (already planned in 4.6), and tool behaviour moved into abilities (H1). Planting reads the seed from the item in hand.

### H3 — The save subsystem knows every system by name

`UVestigeSaveSubsystem` finds the farming component, the inventory, the ability system, the rest component, the grid and the day cycle one by one, and `UVestigeSaveGame` has one field per system plus one global version number. Every new system means editing both, raising the version and adding an upgrade step, and a mod can't save anything. Loading also runs after every actor's `BeginPlay`, so the inventory first adds its starting items and then has them replaced.

**Fix.** A save registry. Each saveable owner implements a small interface: a stable key, writing its struct and reading its struct. The save holds a map from key to that owner's data (an `FInstancedStruct` or bytes) with a version per key, and keeps keys it doesn't recognise, so mod data survives. The save is read into a game-instance subsystem before the world starts (C1), and owners pull their part when they initialise. This also removes the reason `Save/` has to sit above every other layer (M3).

**Status: Resolved (2026-11-06).** `UVestigeSaveRegistry`, a game-instance subsystem in `Core/`, holds one record per key; the clock, the fields, the inventory, the stamina, the rest and the tool in hand implement `IVestigeSaveable`, each with its own key and version, and read their part when they register, so the inventory no longer adds its starting items before a save replaces them. Records keep their struct as bytes, so a part nobody reads (a mod's) is written back unchanged. `UVestigeSaveSubsystem` moved to the game instance, reads the file once per game and names no owner; `Save/` now only needs `Time/` for the clock's new day. The player's data crosses a map change through the registry, taken as the old map starts tearing down, with a test. Design and rules in [Systems.md, section 1](Systems.md#1-saving-and-loading), reasons in [Architecture.md, decision log](Architecture.md#16-decision-log).

### H4 — Save upgrade code for a game nobody else has played

There are five save versions, an upgrade chain, byte-for-byte backups, legacy fields kept on the save class and an empty `UFarmSaveGame` subclass, all so the maintainer's development saves keep loading. That is code to maintain and test for zero players. Compatibility matters from the first public build, not before.

**Fix.** Delete the upgrades, the legacy fields, `EFarmTool` and `UFarmSaveGame`, restart at version 1, and delete the development saves. Turn compatibility on at the first public build ([Roadmap.md, section 6](Roadmap.md#6-before-a-public-release)). Keep the two alternating slots and the refusal to load or overwrite a save from a newer build; those are right.

**Status: Resolved (2026-11-04).** The upgrades and their backups, the version 0 fields, `EFarmTool`, the version 1 counts, `UFarmSaveGame` and the `[CoreRedirects]` only old saves needed are deleted, and `CurrentVersion` is 1. A save of an older version is refused like a corrupt file and left untouched. Deleting the development saves is a manual step in [PlayTest.md](PlayTest.md#2-pending-checks).

### H5 — F9 deletes both saves with one key in a shipped build

F9 (new game: delete both slots and reopen the map) and N (skip the day) are in the normal mapping context with no Shipping guard. A playtester who presses F9 loses everything with no confirmation, and F9 already clashes with the editor's screenshot key ([Roadmap.md, Open problems](Roadmap.md#open-problems)). Moving them to a cheat manager is planned for section 6, which is too late for a key that destroys data.

**Fix.** Now: a `UVestigeCheatManager` with exec functions, compiled out of Shipping and reached from the console. A real "New game" comes with the main menu, behind a confirmation.

**Status: Resolved (2026-11-04).** F9 and N are no longer bound. `UVestigeCheatManager` has the console commands `NewGame` and `SkipDay`, the template player controller sets it as its cheat class outside Shipping, and the engine never creates a cheat manager in Shipping. The New game in the main menu stays with [Roadmap.md, section 6](Roadmap.md#6-before-a-public-release).

### H6 — Input is built in C++ with hard-coded keys

`AVestigePlayerCharacter::CreateInput` makes the input actions and the mapping context in code, with E, F, N, F9 and 1 to 9 fixed, "so no input assets are needed yet". That blocks key rebinding (Enhanced Input's user settings are built around assets), gamepad and handheld mappings, and accessibility. In Unreal, input assets are the normal way and take minutes to make in the editor.

**Fix.** `IA_` and `IMC_` assets, with a small input config data asset that maps each action to a gameplay tag (the pattern of Epic's Lyra sample), so code binds by tag. Roadmap 4.6 says "decide when": decide now, before more keys arrive.

### H7 — The HUD is raw Slate written in C++

Slate is the engine's own UI layer; game HUDs are normally UMG widgets, with Common UI for menus and gamepad navigation. Raw Slate means every layout or colour change is a recompile, nobody can lay out the UI in the editor, and mods can't touch it. The event-driven texts in `AVestigeHUD` are good and can stay as the data side.

**Fix.** Before the 4.6 hotbar, move the HUD to UMG with a C++ base class or a view model (the MVVM plugin, which [Systems.md, section 6](Systems.md#6-user-interface) already names), and adopt Common UI with the first menu.

### H8 — Rendering is Unreal's maximum feature set, unmeasured on the target

`Config/DefaultEngine.ini` turns on hardware ray tracing project-wide, Lumen for lighting and reflections, virtual shadow maps, mesh distance fields and Substrate, and the default performance level stays at Maximum. The target is an RTX 2060 at 60 fps, 1080p High ([Architecture.md, section 10](Architecture.md#budget)). On top of that the sun moves every frame (`AVestigeDayCycle::Tick`), and a moving directional light makes virtual shadow maps redraw their cached shadows every frame, one of the most expensive things to do with them. The first performance baseline is still an open item.

**Fix.** Measure a packaged build of `L_Farm` with a few hundred crop meshes on the hardware closest to the target now, not after the art. Move the sun in small steps (for example every 0.25 degrees) instead of every frame. Turn off `r.RayTracing` unless the Epic tier is a real product goal, because it costs memory and build time even when High doesn't use it. Then decide whether High uses Lumen at all.

**Status: Baseline measured (2026-11-04).** A packaged Development build with a 20 by 20 field of ripe turnips (the console command `BenchmarkField 20 Base.Turnip`) runs at 7.05 ms per frame at 1080p High on an RTX 4070 Ti Super, with the render thread ahead of the GPU; on an RTX 2060 the GPU alone is estimated near 18 ms. The numbers, the method and what they say are in [Architecture.md, section 10](Architecture.md#measuring). The ray tracing decision is recorded below.

**Status: Sun steps done (2026-11-04).** `AVestigeDayCycle` turns the sun only once the clock has moved it `SunStepDegrees` (0.25 degrees, tuned to 0.05 on 2026-11-05 because larger steps showed as jumps), so virtual shadow maps keep their cached shadows between turns; brightness, colour and exposure still change every frame, because the engine's shadow cache only resets on a change of the light's direction. The shadow depths are measured again in the follow-up baseline item.

**Status: Ray tracing off, Lumen kept at High (2026-11-04).** David decided hardware ray tracing isn't needed while the game is being developed: `r.RayTracing` is off for the whole project and Lumen runs in software at every level. The baseline showed ray tracing cost nothing measurable at runtime at High (0 ms, 0.2 MB of geometry), so the gain is in shader compiles, cook time and package size, not frame time. High keeps Lumen, about 0.6 ms of the 6.0 ms GPU frame; the volumetric clouds (1.4 ms) are the next GPU item in [Roadmap.md, 4.0](Roadmap.md#40-structure-from-the-code-review). Whether Epic gets it back is decided with the final art ([Roadmap.md, section 6](Roadmap.md#6-before-a-public-release)). Reasons in [Architecture.md, decision log](Architecture.md#16-decision-log).

**Status: Clouds cheaper below Epic (2026-11-04).** The engine's scalability levels don't touch volumetric clouds, so they cost the same at every level. `Config/DefaultScalability.ini` now caps their samples at High (48 along the view ray and 5 toward the sun, against the engine's 96 and 10), and lower at Medium and Low; Epic is unchanged. In the packaged build, off screen, the cloud pass fell from 0.83 ms to 0.31 ms at High, and screenshots of mornings and evenings show the same clouds with slightly grainier edges. The level's clouds stay volumetric. The windowed baseline measured them at 1.37 ms, so the next windowed run in [Roadmap.md, 4.0](Roadmap.md#40-structure-from-the-code-review) measures them again. Reasons in [Architecture.md, decision log](Architecture.md#16-decision-log).

**Status: Baseline finished (2026-11-05).** A second set of runs of the same scene, now off screen at 1920x1080 High: the frame is 6.65 ms and waits on the GPU (6.0 ms), not on the render thread as the first reading said. The clouds cost 0.74 ms at High (1.37 before; the off-screen runs had rendered at 1280x720), and the sun's steps cut the shadow depths from 0.92 to 0.68 ms at the real clock speed. The 266 ms stall after the new day didn't come back. Video memory is 2.6 GB, and the budget now sets 4 GB of video memory and 8 GB of memory. Measuring on target-class hardware is still open ([Roadmap.md, section 6](Roadmap.md#6-before-a-public-release)). Numbers in [Architecture.md, section 10](Architecture.md#measuring).

## Medium

### M1 — The day cycle is both the clock and the lighting

`AVestigeDayCycle` owns the game's time (state) and moves the sun, sky light and exposure (presentation): the same mix of jobs that was caught when lighting sat on `AFarmGrid`. It is also found or spawned on demand by a static `Get`, a service locator on a placed actor.

**Fix.** The clock moves to the C1 subsystem; a separate sky actor in each level listens to it and owns the lights.

**Status: Resolved (2026-11-05).** The clock moved with C1 (2026-11-04), and the static `Get` went with it. The actor that is left is renamed `AVestigeSkyLighting` for its job, with a class redirect for `L_Farm`, and the hours of sunrise and sunset moved to the clock's settings, so every map's sky follows the same day. Reasons in [Architecture.md, decision log](Architecture.md#16-decision-log).

### M2 — A pawn component ends the world's day

`UVestigeRestComponent`, on the player's pawn, handles `OnDayEndReached` and calls `StartNextDay`. If there is no pawn at 06:00 (a cutscene, a knock-out respawn, a menu, later co-op), the clock stops and the day never ends. It also sits in `Farming/`, though it has nothing to do with farming (M3).

**Fix.** The day transition is a world rule, so the game mode (or the clock subsystem) owns it. The rest component asks for sleep and applies the stamina effects.

**Status: Resolved (2026-11-06).** The clock says when a day has ended (`OnDayEnded`, at 06:00 or early through `EndDay`) and keeps how it ended; `AVestigeGameMode` starts the next day, with or without a pawn (test `Vestige.GameMode.TheNextDayStartsAtTheDayEndWithoutAPlayer`). The rest component goes to bed through `EndDay` and wakes the player on `OnNewDay`. The rest component then moved to the player state, so a night without a pawn still refills stamina and sets exhaustion (test `Vestige.Rest.ANightWithoutABodyStillRefillsStaminaAndSetsExhaustion`). Reasons in [Architecture.md, decision log](Architecture.md#16-decision-log).

### M3 — The strict linear layer order is the wrong shape

Ten folders in one fixed order ([Architecture.md, section 14](Architecture.md#14-dependencies)) mean every folder may use everything below it and nothing above it. That forces odd placements: rest in `Farming/` because it needs `Time/`, interaction below content, and saving above everything, which is why it has to know every system (H3). Real dependencies form a tree, not a line. The source-reading test `Vestige.Architecture.IncludeDirection` is clever, but it rebuilds, weakly, what Unreal modules give for free, and it only checks includes.

**Fix.** Let each feature folder depend on `Core/` plus the features it declares, and when a boundary has settled, make it a real module (a `VestigeCore` runtime module is the obvious first one).

### M4 — Gameplay code writes the player's text

The farming component posts "Tilled the soil." and "Watered." on every successful use, the stamina attribute set posts the out-of-stamina message, and saving posts "Game saved." every morning. Holding E fills the five message boxes in a few seconds.

**Fix.** Gameplay reports results and failure reasons (a gameplay tag or an enum); presentation turns them into text, sound or effects. Successes get a sound and an animation, not a sentence.

### M5 — Every inventory loads and keeps every item definition

`UVestigeInventoryComponent::LoadItems` loads all items synchronously and keeps its own map. Chests, the shipping bin and robot inventories would each do it again, and once items have icons, every icon loads at start.

**Fix.** One content catalogue, loaded once (in `UVestigeAssetManager` or a game-instance subsystem), with icons in their own bundle, loaded when a UI needs them.

### M6 — The template code is load-bearing

`AVestigeGameMode` (which sets the HUD and the player state) and `AVestigeCharacter` (the player's base class, with Epic's camera, style and mobile touch controls) live in `Template/` under Epic's licence terms. M2 needs a game mode of our own anyway.

**Fix.** Our own game mode and player controller in a `Game/` folder now; reparent the character to `ACharacter` when the template animations are replaced.

**Status: Game mode and player controller done (2026-11-06).** `AVestigeGameMode` and `AVestigePlayerController` are ours, in `Game/`, with new Blueprints `BP_GameMode` and `BP_PlayerController`; the template's game mode and controller code, their Blueprints and `Content/ThirdPerson/` are deleted, and the touch controls are gone. `AVestigeCharacter` stays in `Template/` until the template animations are replaced.

### M7 — Game-time events run on real time

[Systems.md, section 4](Systems.md#4-time-days-and-seasons) says timers must not drive game-time events, but the late-night drain is a periodic effect whose amount is converted from `RealSecondsPerGameHour` when it starts. A time skip, a clock speed change or a pause that doesn't pause the world makes it drift.

**Fix.** Drain on the clock's own minute or hour events.

### M8 — The documents outweigh the code

`Docs/` is about 508 KB against about 273 KB of source (7,800 lines). [Architecture.md](Architecture.md) alone is 119 KB; its ownership table restates the class comments, and its decision log records tweaks such as Details panel category names and a Visual Studio plugin flag. From 24 October to this review, 41 commits touched `Docs/` and 15 touched `Source/`. Every change can update five documents, which slows work and invites drift. See rule 5 below.

## Low

- **L1 — Two class prefixes.** `Vestige` and `Farm` in one module make every new class a judgement call, and have already cost two renames. Use `Vestige` for everything new, and stop renaming existing `Farm` classes.
- **L2 — Fields nobody reads.** `UFarmCropData` has `Seasons`, `WaterNeed` and `ContaminationTolerance`, and items have `SellPrice`, all marked "not used yet". Design them with 4.2 and 4.5 instead of guessing their shape now.
- **L3 — Home-made ability idioms.** The use-tool ability waits on an `FTimerManager` timer instead of a wait-delay or montage task, to avoid one small task object per use. The saving is negligible, and standard ability tasks are what other Unreal developers and contributors expect to read.
- **L4 — Synchronous save on the game thread.** Fine at today's size. When saves grow, serialise on the game thread and write with `AsyncSaveGameToSlot`.
- **L5 — No CI.** The 181 tests only run when someone runs them on the maintainer's PC. A script that builds and runs them before each commit is enough for a solo project.

## Project rules that are themselves wrong

These are project rules this review would change. Each is a recommendation with its reason; the maintainer decides, and the roadmap items are marked *(decide)*.

1. **Strict performance is aimed at the wrong level.** The rule has produced careful micro work (every tick justified, cached HUD strings, a 5 Hz scan treated as a cost to defend) while the big costs are deferred: an actor per tile, the rendering feature set, a sun that invalidates shadows every frame. A game's performance is mostly decided by structure and content budgets, and found by profiling regularly. Better rule: a frame budget per system, a packaged-build profile at every milestone, and micro rules only for code that runs per tile or per frame.
2. **"Change structure only when a profile says so"** ([Architecture.md, section 10](Architecture.md#design-rules)) is right for small optimisations and wrong for structural choices whose cost is known up front, like C2.
3. **"No system code without an approved guide"** ([Systems.md](Systems.md#how-to-use-this-document)) is waterfall. Games find their design by playing, so a guide written before anything runs guesses at answers a one-day prototype would give. Better: prototype on a branch, play it, then write the guide and the clean version. Keep the approval step for adopting an engine framework (the Gameplay Ability System, Common UI, StateTree, Mass), where a wrong choice is expensive.
4. **Save compatibility from the first commit** (H4). Turn it on at the first public build.
5. **Every change updates Architecture.md in the same commit, and every choice gets a decision log entry** (M8). Keep `Architecture.md` to principles and a one-page system map, keep ownership in the class comments (already thorough), and log only decisions that are hard to undo.
6. **Avoiding editor assets because the agent can't make them.** Input in code "so no input assets are needed", a Slate HUD "so it needs no widget assets", C++ gameplay effects because scripting can't author modifiers, `BlueprintType` on fragments so Python can create them. Each made sense for an AI working through scripts, and each goes against normal Unreal practice. Better rule: build what is right for the game; when an asset is needed, the maintainer makes it in the editor or the agent writes exact steps for it.
7. **Strict linear layers** (M3) and **two class prefixes** (L1).

## Positive foundations to keep

- **Rules in plain structs.** `FFarmTileState`, `FVestigeInventory` and `FFarmToolbar` hold the rules apart from actors, so they are tested without a level and survive the C2 change unchanged.
- **Tests.** 181 CQTest methods named for behaviour (`AFullBagLeavesTheCropInTheGround`), with the rules and save round trips covered.
- **Content as data.** Primary data assets with namespaced IDs, ID format checks in `IsDataValid`, a startup duplicate-ID check that reads registry tags without loading, soft references with a `Visuals` bundle, and item fragments.
- **The player's data on the player state.** The inventory and the ability system outlive the pawn, and the pawn hands out the ability system through `IAbilitySystemInterface`.
- **Event-driven code with clean teardown.** Almost nothing ticks, delegates are removed in `EndPlay`, and weak pointers are used where the other side can go first.
- **Interaction.** A Blueprint-implementable interface, a dedicated collision channel and a cheap timed scan, as experienced teams do it.
- **Saving basics.** Two alternating slots and refusing to load or overwrite a newer save; keep them when H4 deletes the rest.
- **Localisation-ready text and profiling scopes.** `FText` with `LOCTEXT` throughout, and `TRACE_CPUPROFILER_EVENT_SCOPE` on the bulk work.

## Recommended correction order

The order of [Roadmap.md, section 4.0](Roadmap.md#40-structure-from-the-code-review):

1. Quick wins: H5, H4, then the H8 measurement and sun steps.
2. Before 4.2: decide C1, then C1 with M1, C2, H3, and M2 with M6.
3. Before more tools and 4.6: H1, H2, H6, H7, M4, M5, M7.
4. The rule changes (M3, M8 and the rules above) as housekeeping decisions.
5. The low findings when working nearby.

## Review limits

The review read every non-test source file in `Source/Vestige/` (about 7,800 lines), skimmed the tests and the config, and read the relevant parts of the design documents, on `main` at commit `c78fcee` (2026-11-03, "Update the class names in the lore drafts"), which includes the 4.3 late-night rules, the logging split and the section 3 housekeeping. Blueprints, levels and other binary assets were not opened, and nothing was built or run.
