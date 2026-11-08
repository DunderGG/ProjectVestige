# Project Vestige system guides

How each game system is built. [Architecture.md](Architecture.md) says how the systems fit together and who owns what; [CodingGuidelines.md](CodingGuidelines.md) says how the code looks. This document goes one level down: for each system the game has or will have, what Epic recommends, what the Lyra Starter Game does, what we adopt, and the mistakes to avoid.

## How to use this document

- **Read the system's guide before building or changing it.** The guide's rules apply on top of the general ones.
- **No system without a guide.** Before the first code for a system that has no guide here (or only a "planned" one), write or finish its guide: read Epic's documentation for the engine feature, read how Lyra does it (the Lyra source is free on Fab and installs as a project from the Epic launcher), and write down what we adopt, what we skip and why. David approves the guide before the code. Adopting an engine framework (the Gameplay Ability System, Common UI, StateTree, Mass) is also a [decision log](Architecture.md#16-decision-log) entry.
- **Update the guide when the system changes.** The guide describes the system as built plus the rules for growing it. When code and guide disagree, fix one of them in the same commit.
- **Lyra at our scale.** Lyra is built for a large team and networked multiplayer. We take its structure (components, data assets, tags, subsystems) and skip what only multiplayer or a large team needs (replication, prediction, Experiences, most of its plugin setup), as [Architecture.md, section 11](Architecture.md#11-patterns-borrowed-from-lyra) says. Each guide names what we skip.
- Performance rules are in [CodingGuidelines.md, section 12](CodingGuidelines.md#12-performance) and apply to every system; each guide only adds what is specific to it.

Each guide has the same parts: **Status** (built, partly built or planned, and the roadmap milestone), **References**, **Rules**, **Pitfalls**.

Contents

1. [Saving and loading](#1-saving-and-loading)
2. [Content and the Asset Manager](#2-content-and-the-asset-manager)
3. [Gameplay tags](#3-gameplay-tags)
4. [Time, days and seasons](#4-time-days-and-seasons)
5. [Input](#5-input)
6. [User interface](#6-user-interface)
7. [Interaction](#7-interaction)
8. [Inventory and items](#8-inventory-and-items)
9. [Abilities, attributes and effects (Gameplay Ability System)](#9-abilities-attributes-and-effects-gameplay-ability-system)
10. [Robots: AI, routines and crowds](#10-robots-ai-routines-and-crowds)
11. [Dialogue and relationships](#11-dialogue-and-relationships)
12. [Camera](#12-camera)
13. [Animation](#13-animation)
14. [Audio](#14-audio)
15. [Visual effects and feedback](#15-visual-effects-and-feedback)
16. [Placing objects and building](#16-placing-objects-and-building)
17. [Settings](#17-settings)
18. [Localization](#18-localization)
19. [Debugging and cheats](#19-debugging-and-cheats)
20. [Maps, loading screens and the main menu](#20-maps-loading-screens-and-the-main-menu)
21. [Logging](#21-logging)
22. [Farm fields and tiles](#22-farm-fields-and-tiles)

---

## 1. Saving and loading

**Status:** built as a save registry (2026-11-06, [Roadmap.md, 4.0](Roadmap.md#40-structure-from-the-code-review), [CodeReview.md, H3](CodeReview.md#h3--the-save-subsystem-knows-every-system-by-name)), designed as below, reviewed by a senior Unreal C++ reviewer agent and approved by David. Five owners save through it: the clock (`Base.Clock`), the fields (`Base.Fields`), the inventory with the hotbar and the item in hand (`Base.Inventory`), the stamina (`Base.Stamina`) and the rest (`Base.Rest`). `Base.Farming`, which held the tool in hand until the hotbar joined the inventory (2026-11-07, [section 8](#8-inventory-and-items)), is retired: older saves still carry its record, which the registry keeps as it keeps any key it doesn't know, so the key is never reused (a new owner of it would read the old bytes). The safety rules in [Architecture.md, section 6](Architecture.md#6-saving) still hold: a file version (3), two alternating slots, a save that can't be read left untouched with saving off. The decision of what to load (`UVestigeSaveSubsystem::DecideLoad`) is a static function, so its rules are tested without touching the player's save files.

**References:** Epic's "Saving and Loading Your Game" (`USaveGame`, `UGameplayStatics::SaveGameToSlot` and `AsyncSaveGameToSlot`); `UScriptStruct::SerializeItem` with `FMemoryWriter` and `FObjectAndNameAsStringProxyArchive` (what `SaveGameToMemory` uses) for writing one struct as tagged properties, which tolerate added and removed fields; `FPackageFileVersion` and `FCustomVersionContainer`, the engine's serialization versions, which `FSaveGameHeader` stores for the same reason; `TStructOpsTypeTraits` with `WithSerializer` for a struct that serializes itself; `FWorldDelegates::OnPostWorldInitialization` (a map is initialised, before its actors begin play) and `OnWorldBeginTearDown` (a map starts closing, before any actor is destroyed). Lyra only saves settings (`ULyraSettingsShared`, a `USaveGame` written with `AsyncSaveGameToSlot`), not the world, so this guide relies on Epic's documentation more than on Lyra. Keeping the player's data in a game-instance snapshot is the usual way to carry it across `OpenLevel`, where seamless travel's `CopyProperties` doesn't apply.

**How it works:**

- **The snapshot.** `UVestigeSaveRegistry` (game-instance subsystem, `Core/`) holds the game's saved state in memory: one `FVestigeSaveRecord` per key, and the owners registered with it. A record is the owner's layout version and its save struct as tagged-property bytes, with the engine serialization versions the bytes were written with. It serializes itself natively, so the bytes are copied as one block. The registry knows no system and no file, so every layer can use it.
- **Owners.** Anything with state to save implements `IVestigeSaveable` (`Core/`): a stable key (`Base.Clock`), its layout version, writing its record, reading its record, and starting as in a new game. Its save struct stays next to it (`FVestigeClockSaveData` in `Time/`) with a `static constexpr int32 Version`, and `FVestigeSaveRecord::Write` and `Read` turn the struct into bytes and back.
- **Registering pulls.** Game-instance owners (the clock, the fields) register in `Initialize`; world owners (the inventory, the rest) register in `BeginPlay`, and the player state registers its ability system's saved attribute sets (the stamina), which have no lifetime of their own. Registering hands the owner its record at once, or, when the snapshot has none, has it start as in a new game: the inventory adds its starting items only then.
- **Map changes.** When the open map starts tearing down, before any actor is destroyed, the registry asks every world owner for its record and forgets them; the new map's owners read theirs as they begin play. It has to be then: the engine unpossesses the pawn before any actor's `EndPlay`, so a pawn component could no longer reach the player state's ability system there. A world owner destroyed while its map stays open (a new body for the same player) writes its record in `EndPlay`.
- **Files.** `UVestigeSaveSubsystem` (game-instance subsystem, `Save/`) reads the two slots once per game, when the game's first map is initialised and before any of its actors begins play, and hands the records to the registry, which passes each registered owner its part. It tells the player the outcome once that map has begun play and the HUD listens. On the clock's `OnNewDayFinished` it asks the registry for every record (each registered owner writes its own; records nobody registered are kept as they were read) and writes them to the slot. Nothing is saved before the game has been loaded or started this way, and nothing is loaded or saved while automation tests run, so tests never touch the player's files. When a main menu arrives ([section 20](#20-maps-loading-screens-and-the-main-menu)), its Continue loads instead of the first map, with the gameplay game mode doing it for Play in Editor started on a farm map.
- **New game.** Deletes both slots, empties the snapshot, has every game-instance owner start a new game, and forgets the open map's owners, which would otherwise give the old game's state as the map closes. Then it opens the map again.
- **Versions.** The file has a format version (`UVestigeSaveGame::CurrentVersion`), raised only when the file's own layout changes (3, for records by key), and each record has its owner's version. A record older than its owner's version is dropped with a warning before the first public build: that owner starts as in a new game, the rest loads, and the player is told some saved things were left out. From the first public build the owner migrates it instead. A record newer than its owner understands, or written by a newer engine, isn't read, and saving stays off for the session, as with a newer file.
- **Unknown keys are kept.** A record whose owner isn't registered (a mod that isn't installed, or the player's data while a map has no player) stays in the snapshot and is written back unchanged.

**Rules:**

- **Each owner saves itself** through `IVestigeSaveable`. Its plain `USTRUCT` save data (`FVestigeClockSaveData`) and the typed functions that fill and restore it (`GetSaveData`, `RestoreFromSaveData`) stay on the owner, for tests and developer commands. The registry and the save subsystem never name an owner ([Architecture.md, section 6](Architecture.md#6-saving)).
- **Keys are namespaced like content IDs** (`Base.Inventory`, a mod's `ModName.Thing`) and never change once a build has shipped. One owner per key: a second owner registering a key that is already registered gets a warning and isn't saved. A second inventory (a chest) needs a key of its own.
- **Save data is values and IDs:** numbers, enums, `FName` content IDs, grid coordinates. Never object pointers, asset paths, array positions or display text.
- **Version each record.** Raise the save struct's `Version` when it changes in a way that would load wrong (a field that changes meaning). Adding a field whose default suits older records needs no new version, since records are tagged properties. The file's format version only changes with the file's layout. Until the first public build there are no migrations (decision log, 2026-11-04); from it, the owner migrates each older version of its record, each with a test ([Roadmap.md, section 6](Roadmap.md#6-before-a-public-release)).
- **Register once the owner can restore.** A world owner registers in `BeginPlay` after what restoring needs is ready (the inventory after loading its item definitions), and unregisters at the start of `EndPlay`, before it tears anything down.
- **Owners don't depend on each other while restoring.** World owners restore in the order their actors begin play, which nothing guarantees ([Architecture.md, section 3](Architecture.md#3-lifetimes-and-initialization-order)). Each restores only its own state.
- **What a map opened mid-day needs is worked out at `BeginPlay`, not only from events.** A map opened after midnight never hears the hour the late-night drain starts at, so the rest component checks the clock when it begins play.
- **Write safely.** Write the new save to a second slot, check the write succeeded, then make it the current slot (two alternating slots, each with a sequence number, and load the newest one that reads cleanly). A failed write never touches the last good save.
- **Save at quiet moments** (the new day) and synchronously while it fits the new-day budget ([Architecture.md, section 10](Architecture.md#10-performance-by-design)); switch to `AsyncSaveGameToSlot` when it doesn't (it still builds the bytes on the game thread, so measure first), and never start a map change while a write is in flight.
- **Restore without side effects:** restoring sets state; it never replays actions or broadcasts gameplay events like `OnNewDay`.
- **Unknown content IDs are skipped with a warning** and the player is told; the rest of the save loads. Reading a record adds the entries it left out to a count the registry keeps.
- Saving the player's settings is separate ([section 17](#17-settings)).

**Pitfalls:**

- `UPROPERTY(SaveGame)` with archive serialization is tempting for "save every actor automatically", but it couples the save layout to class layouts and breaks on renames. Use it at most inside an owner's own save struct, not across the world.
- State that must survive a map change kept only in the world (an actor, a world subsystem) without registering it: opening a map destroys it.
- **Taking a pawn component's part in `EndPlay` when the map closes.** The pawn is unpossessed first, so whatever it reaches through the player state (the ability system, so exhaustion) is gone by then. The registry takes the parts when the map starts tearing down.
- A world owner registered as a game-instance owner, or the other way round: a new game would then keep the old game's player data, or forget the clock.
- **Bytes outlive the engine.** The tagged property format changes between engine versions, and a kept record (a mod's) can be older than the build writing it. Each record stores the engine and custom versions it was written with, and is read with them.
- **Byte arrays in tagged properties are written one byte at a time.** A record serializes itself natively, so its bytes are copied in one block.
- **Changing the default value of a property on `UVestigeSaveGame`.** Unreal leaves a property out of the save file when its value equals the default, so an old save would read the new default instead of the value it had. Records are written in full, so this only concerns the file's own fields.
- Testing only the round trip. Also test an old version, a corrupt file, a newer version, an unknown key and a map change.

## 2. Content and the Asset Manager

**Status:** built for crops, tools and items (`UVestigeContentData`, `UFarmCropData`, `UVestigeToolData`, `UVestigeItemData`). `UVestigeContentData::IsDataValid` checks the ID format, and `UVestigeAssetManager` reports duplicate IDs after the initial asset scan. The global game data is built (2026-11-07): `UVestigeGameData` (`DA_GameData`), loaded once at startup, names the gameplay effects code applies ([section 9](#9-abilities-attributes-and-effects-gameplay-ability-system)). The ID redirect list is deferred until an ID has to change.

**References:** Epic's "Asset Management" and "Data Assets" pages (`UPrimaryDataAsset`, primary asset types in Project Settings > Asset Manager, asset bundles, `UAssetManager::LoadPrimaryAsset`); Lyra's `ULyraAssetManager` (a project subclass that loads global game data at startup) and its data assets (`ULyraPawnData`, `ULyraExperienceDefinition`, `ULyraInventoryItemDefinition`).

**Rules:**

- One `UVestigeContentData` subclass per kind of content, one asset per piece of content, registered as a primary asset type in `Config/DefaultGame.ini` and found through the Asset Manager ([Architecture.md, sections 5 and 12](Architecture.md#5-data-and-content)).
- Definitions hold no runtime state. Per-instance state (a crop's growth, an item's durability) lives on the owner of the instance.
- Meshes, materials, sounds and effects in a content asset are soft references in an asset bundle (`Visuals`, later `Audio`), loaded with the asset when it's needed.
- **Data validation:** each content class overrides `IsDataValid` to check its ID format and required fields, so mistakes show up in the editor (right-click > Asset Actions > Validate Assets) instead of at play time.
- When the game needs global data at startup (lists of starting items, default settings), `UVestigeAssetManager` loads it once, as `ULyraAssetManager` does, rather than having each system load its own.

**Pitfalls:**

- Hard references from one data asset to another (`TObjectPtr<UFarmCropData>` inside an item) load the whole chain. Refer to other content by primary asset ID or soft reference.
- Forgetting to register a new type in the Asset Manager settings: the assets exist but the game never finds them, and they aren't cooked.

## 3. Gameplay tags

**Status:** in use for seasons (`Vestige.Season.*` in `Config/DefaultGameplayTags.ini`) and, as native tags in `AbilitySystem/VestigeGameplayTags.h`, for the stamina system ([section 9](#9-abilities-attributes-and-effects-gameplay-ability-system)) and input (`Vestige.Input.*`, [section 5](#5-input)).

**References:** Epic's "Gameplay Tags" page; Lyra's native tags (`LyraGameplayTags.h/.cpp`, declared with `UE_DECLARE_GAMEPLAY_TAG_EXTERN` and defined with `UE_DEFINE_GAMEPLAY_TAG_COMMENT`).

**Rules:**

- Tags are for open-ended categories and states (seasons, item categories, input actions, ability states), not for individual pieces of content, which have `ContentId`s ([Architecture.md, section 5](Architecture.md#5-data-and-content)).
- Our tags live under `Vestige.`; mods add their own root.
- Tags that C++ refers to are **native tags**, declared in one `VestigeGameplayTags.h/.cpp` with `UE_DEFINE_GAMEPLAY_TAG_COMMENT`, so a typo is a compile error and every tag has a comment. Tags only content refers to stay in the `.ini`.
- Use `FGameplayTagContainer` and `HasTag` / `HasAny` queries; restrict editor pickers with `meta = (Categories = "Vestige.Season")`.

**Pitfalls:**

- `FGameplayTag::RequestGameplayTag(TEXT("..."))` scattered through code: a renamed tag fails at run time. Use the native tag.
- Tags as a replacement for enums that code switches on exhaustively ([Architecture.md, section 12](Architecture.md#12-designing-for-mods), rule 2).

## 4. Time, days and seasons

**Status:** day and clock built (`UVestigeClockSubsystem`, a game-instance subsystem, with `UVestigeClockSettings`; 2026-11-04), and the lighting that follows it (`AVestigeSkyLighting`). Seasons and weather are milestone 2 and 3.

**References:** no single Epic system; the rules here follow [Architecture.md, sections 15 and 18](Architecture.md#15-the-day-flow). Lighting follows Epic's "Sky Atmosphere", "Exponential Height Fog" and "Lumen" pages.

**Rules:**

- One owner of game time (`UVestigeClockSubsystem`), in the game instance so it keeps time across maps; its speed and hours, sunrise and sunset included, are project settings (`UVestigeClockSettings`), never a level's. Everything else reads it or listens to its events (`OnNewDay`, `OnNewDayFinished`, `OnHourStarted`, `OnDayEnded`). Nothing keeps its own clock: something that happens at a time of day listens to `OnHourStarted`, which carries the hour since the start of the day's midnight (22 for 22:00, 24 for the next midnight), rather than watching the clock or running a timer of its own.
- The game mode (`AVestigeGameMode`) owns the end of the day. The clock says when a day has ended (`OnDayEnded`: it reached the day's end, or something such as going to bed asked for it with `EndDay`) and stops; the game mode starts the next day. Whatever reacts to how the night went (the rest component's exhaustion) does it on `OnNewDay`, reading the clock's `GetLastDayEnd`, so the save made after it holds the result. Nothing but the game mode starts a day from `OnDayEnded`: a listener after it may already be in the new day.
- Blueprints, and so mods, hear of a new day through `On New Day` (`OnNewDayBlueprint`), the extension point broadcast after `OnNewDay` and before the save ([Architecture.md, section 12](Architecture.md#extension-points-today)). A Blueprint finds the clock with **Get Vestige Clock**.
- Game time is separate from real time and from `UWorld::GetTimeSeconds`. Pausing and menus stop game time: the clock ticks with the open world, so pause stops it and time dilation (`slomo`) speeds it.
- Daily changes happen in `OnNewDay` listeners that finish synchronously ([Architecture.md, section 15](Architecture.md#15-the-day-flow)).
- Seasons are gameplay tags and change on a day event; weather is chosen once per day from data (per-season weights), so it's saved as a value, not re-rolled on load.
- Lighting, fog and sky react to time and weather through the sky lighting actor (`AVestigeSkyLighting`, one per outdoor level), which reads the clock and its sunrise and sunset hours and never changes them; nothing else moves the sun. The sun turns in small steps, not every frame, so virtual shadow maps keep its cached shadows between them; brightness and colour may change every frame.

**Pitfalls:**

- Timers (`FTimerManager`) for game-time events: they run on real time and don't save. Schedule game-time events against the clock.
- Comparing hours as floats for equality; compare against ranges or minute counts.

## 5. Input

**Status:** built as assets bound by tag ([CodeReview.md, H6](CodeReview.md#h6--input-is-built-in-c-with-hard-coded-keys), 2026-11-07, approved by a senior Unreal C++ reviewer agent after three rounds, whose approval counts as David's). The farming actions are `IA_` assets in `Content/Vestige/Input` with one mapping context, `IMC_Farming`; `DA_InputConfig_Player` (`UVestigeInputConfig`) names the action for each native input tag (`Vestige.Input.*`), and `AVestigePlayerCharacter` binds by tag and adds its contexts while possessed. The template's moving and looking (`IA_Move`, `IA_Look`, `IA_MouseLook`, `IA_Jump`, `IMC_Default`, `IMC_MouseLook`) are still bound by the template base class until it is replaced ([CodeReview.md, M6](CodeReview.md#m6--the-template-code-is-load-bearing)). Rebinding waits for the settings menu ([section 17](#17-settings)).

**References:** Epic's "Enhanced Input" pages (input actions, mapping contexts, triggers, modifiers, player-mappable key settings, user settings); Lyra's `ULyraInputConfig` (a data asset mapping input actions to input tags), `ULyraInputComponent` and `ULyraHeroComponent`, which bind actions by tag to abilities.

**Take, skip, change from Lyra:**

- **Take:** the input config data asset with a list of native actions by tag (`FindNativeInputActionForTag`, with a flag for whether a missing tag is an error); native input tags in C++; the pawn adding its mapping contexts and registering them with the Enhanced Input user settings; player-mappable key settings on each action.
- **Skip:** the second list, ability input actions (abilities activated by input tag). None of our keys goes straight to an ability: E goes to `UFarmingComponent::StartUsingTool`, which first follows the inventory, grants the tool in hand and announces the held key for the walk speed, and then presses the spec's input by handle; F goes to `UVestigeInteractionComponent::Interact`, which activates by ability tag. Ability input tags come when the first key maps straight to an ability with no owner logic in between. Skip the custom input component (`ULyraInputComponent`): a small private function on the character finds each action and binds it, so the input component class stays the engine's. Skip `ClearAllMappings` on possession, which would also drop contexts menus add.
- **Change:** contexts are added in `SetupPlayerInputComponent` (as before) and removed in `UnPossessed`, not added in `NotifyControllerChanged`; the input component is made and set up on each possession by the local controller (`PawnClientRestart`), and adding a context that is already applied only changes its priority, so neither path stacks it.

The assets, all Boolean:

| Action | Mappable name (the input tag) | Keyboard or mouse | Gamepad |
| --- | --- | --- | --- |
| `IA_UseTool` ("Use tool") | `Vestige.Input.UseTool` | E | Face button left (X) |
| `IA_Interact` ("Interact") | `Vestige.Input.Interact` | F | Face button right (B) |
| `IA_HotbarSlot1` to `9` | `Vestige.Input.HotbarSlot.1` to `.9` | 1 to 9 | none: the gamepad steps |
| `IA_HotbarNext` | `Vestige.Input.HotbarNext` | Mouse wheel down | Right shoulder |
| `IA_HotbarPrevious` | `Vestige.Input.HotbarPrevious` | Mouse wheel up | Left shoulder |

Gamepad A stays jump, from the template's `IMC_Default`. The gamepad layout is provisional: interact will likely move to A when jumping moves with the template's replacement ([Roadmap.md](Roadmap.md), the M6 item). In each action the keyboard or mouse mapping comes first and the gamepad one second, the order the user settings fill an action's key slots in. `BP_PlayerCharacter` names `DA_InputConfig_Player` and the contexts `IMC_Default` (0), `IMC_MouseLook` (0) and `IMC_Farming` (1), as hard references: they are tiny and needed the moment the pawn is possessed, so nothing loads during play. A temporary headless commandlet, not committed (as for the `GE_` assets), made the assets, set `BP_PlayerCharacter`'s properties and re-saved `BP_PlayerController` without its old contexts.

Tests (`Source/Vestige/Tests/VestigeInputTests.cpp` and others): `Vestige.Input.ThePlayerBindsEveryConfigEntry` (the config has an action for every tag the character binds; on an input component every entry is bound, the use key on `Started` and `Completed`, and the template's move, look, mouse look and jump stay bound), `EveryFunctionHasAKeyboardAndAGamepadKey` (keyboard or mouse first; the slot actions keyboard only), `NoKeyDoesTwoThings` (across the three contexts), `EveryFarmingActionHasAUniqueMappableName`, `ThePlayersConfigValidates` and `AConfigWithADuplicateTagIsInvalid`; `Vestige.PlayerCharacter.WithoutAnInputConfigTheHintsAreEmpty` and `UnpossessingReleasesTheUseKey`; `Vestige.Inventory.SteppingTheHotbarWrapsAround`, `SteppingWithoutAHotbarDoesNothing` and `SteppingTheHotbarPutsTheNextItemInHand`. That contexts don't stack on re-possession is a [PlayTest.md](PlayTest.md) check (`showdebug enhancedinput`), since the actor test spawner has no local player.

**Rules:**

- Input actions (`IA_`) and mapping contexts (`IMC_`) are assets in `Content/Vestige/Input`. Mapping contexts are added and removed by context (on foot, in a menu, driving a machine) with explicit priorities. The pawn owns the contexts of its body (`MappingContexts` on the character); menus will add their own on the player controller. Every context add has a matching remove: the character adds its contexts in `SetupPlayerInputComponent` with `FModifyContextOptions::bNotifyUserSettings` set, so they register with the player's Enhanced Input user settings, and removes them in `UnPossessed` with the default options, so they stay registered for the next possession.
- Code binds by native input tag (`VestigeGameplayTags::Input_*`), never by asset: the character asks its input config for the action of each tag. A missing config logs an error and binds nothing (the contexts are still added); a tag missing from the config logs an error. The config validates in the editor: every entry has an action and a tag under `Vestige.Input`, no tag or action twice, every action is Boolean, and an action with player-mappable settings has a mapping name.
- The character binds input to calls on the components that own the action (`UFarmingComponent::StartUsingTool` on the use key's `Started`, `StopUsingTool` on its `Completed`); input code holds no game rules. Holding a key to repeat an action is the owner's rule, driven by events rather than by `Triggered`, which fires every frame: the press activates the tool's ability with its spec's input pressed, and the ability itself starts the next use when the last one ends or the target tile changes, and only if the use would change the tile, so a held key never posts a message every frame ([section 9](#9-abilities-attributes-and-effects-gameplay-ability-system), tools as abilities). While it is held the farmer walks and jumps at half speed, a setting on the movement component (`ToolUseSpeedScale`), so a row is worked evenly. No tool works in the air (the tool ability waits for the farmer to walk again while the key is held), and a held key resumes on landing, so jumping can't get round the slower walk. Keys 1 to 9 select the inventory's hotbar slots, through the character's `SelectHotbarSlot`, and the mouse wheel and the gamepad shoulders step through them, through its `StepHotbarSlot`, which forwards to the inventory's `StepHotbarSlot` ([section 8](#8-inventory-and-items), the hotbar). The character also releases a held use key in `UnPossessed`, before the input component and its release binding are destroyed, so the walk speed is never left halved. E uses the item in hand and nothing else; F interacts with the bed and harvests the ripe tile the farmer faces ([section 7](#7-interaction), harvesting with F). Movement input also aims the targeting component, `UFarmTargetingComponent`, at a neighbouring tile (`AimAt`), and the walk eases in from the press, read from the move action's elapsed time (`FInputActionInstance::GetElapsedTime`), so the template's `IA_Move` needs no extra trigger or modifier. Turning belongs to the farmer's movement component, `UVestigeCharacterMovementComponent`, which turns toward the aimed tile when there is no input.
- Keys bind natively to the owner of the action, never to an ability by input tag, while every key goes through owner logic first (the use key, interact). When a key first maps straight to an ability with no owner logic in between, the input config gains Lyra's ability input actions and that ability activates by input tag.
- Key hints on screen come from the mappings, never hard-coded text ([ArchitectureAudit A-14](ArchitectureAudit.md)). The action hints are every action of the input config with an `ActionDescription`, in the config's order; the hotbar hints are the slot actions' keys. A key is the first mapping of the action in the pawn's contexts whose key is neither a gamepad, touch nor gesture key. Gamepad hints, and hints that follow the last used device, are their own [Roadmap.md](Roadmap.md) item in 4.0, now that the HUD is in UMG. Reading the assets' default mappings is right while keys can't be rebound; with rebinding, the hints ask Enhanced Input for the player's current key and the HUD rebuilds them on a remap event ([Roadmap.md](Roadmap.md), the settings item). The HUD builds the hints only on events, so the lookups (about fifteen entries, three contexts) never run per frame.
- Player rebinding uses Enhanced Input's player-mappable key settings and its user settings save game ([section 17](#17-settings)). Every action has player-mappable key settings whose `Name` is its input tag (`Vestige.Input.UseTool`), one naming scheme, and a display name. The user settings stay off (`bEnableUserSettings`) until the settings menu.
- Gamepad support is designed in from the first asset-based mapping: every function has a gamepad key. The direct slot keys 1 to 9 are keyboard conveniences; the gamepad reaches the same slots by stepping.
- The template's actions have no mappable names and are bound through the template's own properties; they join the input config when the template goes ([CodeReview.md, M6](CodeReview.md#m6--the-template-code-is-load-bearing)).

**Pitfalls:**

- Adding a mapping context every time a pawn is possessed without removing the old one. Every add has a matching remove.
- Using the legacy input system (`BindAxis`, `BindAction` on `UInputComponent`) in new code.
- Hotbar stepping binds `Started`, so several wheel notches in one frame are one step; fast scrolling can skip fewer slots than notches.
- Renaming a mappable name once players can save remaps: it is the key the remap is saved under ([section 17](#17-settings)).

## 6. User interface

**Status:** the HUD is built in UMG ([CodeReview.md, H7](CodeReview.md#h7--the-hud-is-raw-slate-written-in-c), 2026-11-08, approved by a senior Unreal C++ reviewer agent after two rounds, whose approval counts as David's): `AVestigeHUD` builds the texts and pushes them to `WBP_Hud` (`UVestigeHudWidget`), with each feedback message in a `WBP_HudMessage` (`UVestigeHudMessageWidget`); `BP_HUD` names the widget and `BP_GameMode` names `BP_HUD`. The Slate widget is gone. Menus, the hotbar row and an inventory screen in UMG are milestone 2; Common UI comes with the first menu.

**References:** Epic's "UMG UI Designer", "Common UI" and "UMG Viewmodel" (MVVM plugin) pages; Lyra's UI: `UGameUIManagerSubsystem` and `UGameUIPolicy` (CommonGame plugin), a `UPrimaryGameLayout` with layers (game, game menu, menu, modal) that widgets are pushed onto, and the UIExtension plugin (`UUIExtensionSubsystem`), which lets features add widgets to named HUD slots without the HUD knowing them. Lyra's `ALyraHUD` hosts no widgets (its comment says it exists for debug drawing; the experience's "Add Widgets" action pushes `ULyraHUDLayout`), and its HUD parts are `UCommonUserWidget` C++ bases with `meta = (BindWidget)` parts and `EditAnywhere` style properties (`ULyraPerfStatWidgetBase`, `ULyraReticleWidgetBase`).

**The UMG HUD (H7).** The HUD of the Slate version, looking the same, laid out in Widget Blueprints instead of Slate code. Nothing new is shown; the hotbar row is [Roadmap.md, 4.6](Roadmap.md#46-interface-and-input).

*Take, skip, change from Lyra and Epic's UMG guidance:*

- **Take:** C++ base classes with `BindWidget` parts and style properties the Blueprint sets, so layout, fonts and colours are edited in the UMG designer without a recompile; one root HUD layout widget per player, added with `AddToPlayerScreen` as Lyra's `UGameUIPolicy` does. From Epic's "UMG Best Practices" tech blog: an abstract C++ `UUserWidget` base per widget whose `BlueprintImplementableEvent`s are the contract with the Blueprint, never `BlueprintNativeEvent`; widgets set up again with new data rather than made anew (pooled); no Blueprint `Tick`, no property bindings, and few animations in a HUD. From Epic's "UMG Best Practices" documentation page: event-based updates, every Widget Blueprint authored at 1920x1080 and DPI scale 1.0 under the project's one DPI curve, Auto slot sizing for parts sized by their content, plain brushes rather than materials for UI art, and reusable parts as their own User Widgets.
- **Skip:** CommonGame's UI manager, policy and layered `UPrimaryGameLayout`, Common UI and the UIExtension plugin. The HUD is one layer with no menus yet; Common UI comes with the first menu (reparenting the bases to `UCommonUserWidget` then is cheap), and named HUD slots with the first feature or mod that adds a HUD element (rules below). Skip the MVVM plugin too: its bindings are authored in the designer's View Bindings panel, the HUD has six fields, and the guide already says viewmodels pay off once screens have many fields. Skip Invalidation and Retainer Boxes: the HUD is about a dozen widgets, and one can be added if a profile asks for it.
- **Change:** our `AHUD` hosts the layout widget, since there is no experience to push it. The blog's Blueprint fills its widgets from a data object when an event fires; ours has the C++ base set its `BindWidget` parts and then fire the event, so the Blueprint needs no graph to work but can restyle from the event (a hybrid: our assets are made by a commandlet, which can build a widget tree but not reliably a graph). The blog's Blueprint also creates the child widgets; ours are made by the C++ base from a widget class the Blueprint names.

*Classes (all in `UI/`):*

- **`AVestigeHUD`** stays the data side, as the code review suggests, and the exception to the rule that a widget's C++ base binds to its own events: it follows the clock, the pawn's interaction component, the player state's inventory and ability system and the feedback subsystem exactly as today, builds each text when its event fires, and **pushes** it to the widget (`HudWidget->SetBagText(...)`, each push checking `HudWidget`). It is an acceptable data object for six texts; it doesn't grow with new HUD parts (see the hotbar boundary below). The widget never reads the HUD, so no per-frame bindings remain.
   - New `UPROPERTY(EditDefaultsOnly, Category = "HUD") TSubclassOf<UVestigeHudWidget> HudWidgetClass`. At `BeginPlay`, only when `PlayerOwner` has a local player and the world has a game viewport (today's guard: `CreateWidget` on a controller without a player logs a PIE error and returns null), it creates the widget (`CreateWidget<UVestigeHudWidget>(PlayerOwner, HudWidgetClass)`, held in a `UPROPERTY() TObjectPtr`), adds it with `AddToPlayerScreen`, and only then starts following, so each first refresh reaches the widget. Without a widget class it logs an error (`LogVestige`), follows nothing and turns its tick off.
   - At `EndPlay` it unbinds as today, calls `RemoveFromParent` and clears `HudWidget` (the widget's outer is the game instance, which outlives the map). A map change with `open` destroys the HUD and makes a new one. Seamless travel would carry the HUD over (`GetSeamlessTravelActorList` adds it), so this needs revisiting if the game ever uses it.
   - The clock is the one value that changes without an event, and [Roadmap.md](Roadmap.md) rules out a per-minute clock event. So the HUD's `Tick` (an `AHUD` ticks by default, `HUD.cpp`), with the comment [CodingGuidelines.md, section 12.1](CodingGuidelines.md#121-ticking) asks for, compares the clock's day and shown minute with the ones last pushed and calls `RefreshStatusText` only when one changed. The item in hand's event calls `RefreshStatusText` directly. The tick is turned off (`SetActorTickEnabled(false)`) when there is no widget or no clock. The mutable getter and the public text getters go.
   - The messages model (`FVestigeHudMessages`), its one expiry timer and `UVestigeHudSettings` stay as they are; when the list changes the HUD pushes the whole list (`ShowMessages(Messages.GetMessages(), MessageSeconds)`).
- **`UVestigeHudWidget : UUserWidget`** (`Abstract`, `meta = (DisableNativeTick)`, which `UUserWidget` declares on itself and doesn't inherit, so it is set here), the base of `WBP_Hud`.
   - Parts (`BindWidget`): `UTextBlock` `StatusText`, `StaminaText`, `BagText`, `PromptText`, `KeyHintsText`; `UWidget` `PromptBox` (the backdrop around the prompt, collapsed while the prompt is empty); `UVerticalBox` `MessageList` (concrete, since the C++ sets its slots).
   - Style (`EditDefaultsOnly`, category **Style**): `StaminaColor` (white), `StaminaWarningColor` (today's red), `MessageSpacing` (6, the gap above every message box but the first), `TSubclassOf<UVestigeHudMessageWidget> MessageWidgetClass`.
   - Functions (C++): `SetStatusText`, `SetStamina(Text, Stamina, MaxStamina, bOutOfStamina, bExhausted)`, `SetBagText`, `SetPromptText`, `SetKeyHintsText`, `ShowMessages(const TArray<FVestigeHudMessage>&, double Lifetime)`. Each sets its parts first, then fires its event.
   - Events (`BlueprintImplementableEvent`, category **HUD**), for restyling without C++; an event nobody implements costs nothing: `OnStaminaChanged(int32 Stamina, int32 MaxStamina, bool bOutOfStamina, bool bExhausted)` (a stamina bar could be drawn from the numbers), `OnPromptChanged(bool bShown)`. The HUD passes the numbers it already reads for the stamina text.
   - The constructor sets the widget's default visibility to `HitTestInvisible` (with the protected `SetVisibilityInternal`, as `UUserWidget`'s own constructor does, so no visibility event fires on the class default object), so the HUD never takes the mouse and stays out of the hit-test grid. In the constructor rather than `NativeOnInitialized`, which doesn't run for a widget made without a player unless the Blueprint's `bCanCallInitializedWithoutPlayerContext` is set.
   - `ShowMessages` shows message *i* in box *i*, oldest first, and collapses the boxes beyond the list. Boxes are made from `MessageWidgetClass` and added to `MessageList` (Auto height, centred, `MessageSpacing` above all but the first) only when the list is longer than the boxes made so far, and are never removed: at most `MaxMessages` boxes for the session, made once. No local is named `Slot`, which would shadow `UWidget::Slot`.
- **`UVestigeHudMessageWidget : UUserWidget`** (`Abstract`), the base of `WBP_HudMessage`, one feedback message in its own box. Part: `UTextBlock` `MessageText`. `ShowMessage(Text, StartTime, Lifetime)` sets the text, keeps the times, sets the starting opacity, shows the box and fires `OnMessageShown` (`BlueprintImplementableEvent`); `Hide` collapses it. `NativeTick` calls the public `UpdateOpacity(Now)`, which sets the whole widget's render opacity to `FVestigeHudMessages::GetOpacity(StartTime, Now, Lifetime)` (now from `GetWorld()`, checked), so the backdrop and the text fade together, as today.
   - Why a tick and not a UMG animation: Slate ticks only what it paints, so a collapsed box costs nothing and at most five boxes tick while their messages show; `SetRenderOpacity` invalidates paint only when the value changes. A running animation also keeps the widget ticking, adds sequence evaluation and would need its length scaled to the configurable `MessageSeconds`, and the blog asks for few animations in a HUD.
- **The hotbar boundary (4.6, recorded now so the HUD doesn't grow).** The hotbar row will be its own widget base that finds the inventory component once (`GetOwningPlayerState()`) and binds to `OnInventoryChanged` itself, as Lyra's HUD parts find their own data (but without Lyra's `NativeTick` polling in `ULyraWeaponUserInterface`). Its slot widget gets `SetupSlot(const FVestigeInventorySlot&, bool bSelected)` and an `OnSlotSet` event, one pooled widget per slot, like the blog's `SetupOffer` and `OnOfferSet`. `AVestigeHUD` gains nothing for it.

*Assets:*

- `Content/Vestige/UI/WBP_Hud` (parent `UVestigeHudWidget`), authored at 1920x1080 and DPI scale 1.0: an overlay root, as the Slate HUD's `SOverlay`, so every part is placed by its slot's alignment and padding, sized by its content (Auto): the status panel top left, 24 from the edges (a border, black at 45 %, padding 16 by 10, with the status line in Roboto Bold 20, then stamina and bag in Roboto Regular 16, 4 apart); the prompt box centred, 160 below the middle (padding 16 by 8, Bold 18), collapsed; the message list centred at the bottom, 80 up; the key hints centred at the bottom, 24 up (Regular 13, light grey). Every text has a black shadow offset by 1, 1 (a UMG text's shadow is transparent by default, while the Slate style's was black). Fonts are the engine's `/Engine/EngineFonts/Roboto`, the same faces Slate used. The texts' preview strings are culture-invariant, so they aren't gathered for translation. `MessageWidgetClass` is `WBP_HudMessage`.
- `Content/Vestige/UI/WBP_HudMessage` (parent `UVestigeHudMessageWidget`): a border (black at 45 %, padding 16 by 8) around `MessageText` (Bold 18, warm yellow).
- `Content/Vestige/Blueprints/BP_HUD` (parent `AVestigeHUD`) with **HUD Widget Class** `WBP_Hud`; `BP_GameMode`'s **HUD Class** becomes `BP_HUD`. `AVestigeGameMode` keeps the native `AVestigeHUD` as its C++ fallback. The references are hard ([Architecture.md, section 8](Architecture.md#8-c-and-blueprints)): the widgets are small and needed the moment play starts, so nothing loads during play, where a soft class in settings would only add a load at the start of play.
- The assets are made by a temporary editor commandlet, not committed, as the input assets were. That goes against the code review's rule change 6 (the maintainer makes assets in the editor) only in who clicks: the play-test opens both widgets in the designer and changes one there. The commandlet:
   - creates each widget with `FWidgetBlueprintOperationUtils::CreateWidgetBlueprint` (which copies the project's `bCanCallInitializedWithoutPlayerContext` and sets up the tree), not a bare `FKismetEditorUtilities::CreateBlueprint`;
   - builds the tree with `WidgetTree->ConstructWidget<T>(Class, Name)`, sets `bIsVariable` on the parts and calls `OnVariableAdded(Name)` for every widget, since the compiler ensures each widget has a GUID;
   - sets every slot's alignment, padding and size rule, every text's font, colour and shadow colour, and every border's brush colour and padding;
   - compiles with `FKismetEditorUtilities::CompileBlueprint` and fails unless the status is up to date (a missing `BindWidget` part is a compile error);
   - sets the class defaults (`MessageWidgetClass`, `HudWidgetClass`, `BP_GameMode`'s `HUDClass`), then registers the new assets with the asset registry and saves the packages.

*Module:* `UMG` joins the private dependencies; `Slate` and `SlateCore` stay (UMG needs them, and `FSlateColor` is in the widget header). The Slate widget `SVestigeHudWidget` is deleted. Test-only getters: `AVestigeHUD::GetHudWidgetClass` and `UVestigeHudWidget::GetMessageWidgetClass`.

*Tests (`Source/Vestige/Tests/VestigeHudWidgetTests.cpp`, `Vestige.UI.*`), with widgets created by `CreateWidget` on the test world, which needs no viewport and binds the `BindWidget` parts:*

- `TheGameModeShowsTheHudWidget`: `BP_GameMode`'s HUD class is an `AVestigeHUD` with a widget class, and that widget's message widget class is set.
- `OnlyTheMessageBoxesTick`: `WBP_Hud`'s class doesn't need a native tick and `WBP_HudMessage`'s does.
- `TheHudDoesNotTakeTheMouse`: a created `WBP_Hud` is `HitTestInvisible`.
- `EachMessageShowsInItsOwnBox`: a `WBP_Hud` shown three messages then one has three message boxes, the first visible with the newest list's text and the other two collapsed.
- `AMessageFadesInAndOut`: a message box's render opacity after `UpdateOpacity` follows `FVestigeHudMessages::GetOpacity` at its start, middle and end.
- `ThePromptHidesWhenEmpty`: the prompt box is collapsed after an empty prompt and visible after "F Sleep".
- `TheStaminaLineWarnsWhileOutOfStamina`: the stamina text takes the warning colour and back.
- The existing `Vestige.HudMessages.*` and `Vestige.HudSettings.*` tests stay unchanged.

*Play-test (PlayTest.md):* the HUD looks as before (status panel, stamina in red at zero, bag, prompt, messages fading one per box, key hints) at 1080p and in a smaller window; the mouse still turns the camera through the HUD; `WBP_Hud` and `WBP_HudMessage` open in the designer and a colour change there shows in the next Play without a build; the Widget Reflector (Ctrl+Shift+W) shows the HUD's widgets under the player's layer and no hit-testable HUD widget; a packaged build shows the HUD; a `BP_HUD` with no widget class logs the error and shows no HUD (the test spawner has no local player, so this is checked by hand).

*Docs in the same commit:* this section (status, rules); [Architecture.md](Architecture.md): section 7's "UI technology grows with the UI", the ownership rows of `AVestigeHUD` and `AVestigeGameMode`, the `UI/` folder row, the diagrams naming `SVestigeHudWidget`, and a decision log entry that reverses the one of 2026-10-25; [CodingGuidelines.md](CodingGuidelines.md): the `S` prefix example, the weak pointer example, the include example and section 12.6 (its example and the clock exception, which becomes "`AVestigeHUD::Tick` reads the clock"); the comment in `VestigeHudMessages.h`; [CodeReview.md](CodeReview.md): a status line under H7; [Roadmap.md](Roadmap.md): H7 ticked, with Common UI with the first menu and the gamepad key hints as their own items; [PlayTest.md](PlayTest.md).

*Not in this change:* gamepad key hints that follow the last used device stay their own roadmap item: they change what the hints say, and this change only moves the technology. Common UI stays with the first menu, also its own item.

**Rules:**

- **Presentation only.** Widgets read state and listen to events; they never change game state except by calling the owner's public functions in response to player input ([Architecture.md, section 7](Architecture.md#7-presentation-ui-audio-and-effects)).
- **UMG Widget Blueprints with C++ base classes** for the HUD, menus and the inventory: the abstract C++ base holds the logic and binds to events (the HUD's texts are the exception: `AVestigeHUD` pushes them), sets the parts it needs (`meta = (BindWidget)`), and fires `BlueprintImplementableEvent`s after it has, so a Blueprint can restyle without C++; never `BlueprintNativeEvent`. The Blueprint holds layout and style. Widgets showing a thing get a setup function (`SetupSlot`) and are set up again rather than made anew.
- **Authoring.** Every Widget Blueprint at 1920x1080 and DPI scale 1.0 under the project's one DPI curve; overlay and box panels with Auto sizing where they copy a simple layout; plain brushes rather than materials for UI art; Collapsed rather than Hidden for parts not shown; `HitTestInvisible` for everything that takes no input. No Blueprint `Tick`, no property bindings, and few animations in the HUD.
- **Event-driven updates.** Widgets update when an event fires, not through UMG property bindings, which run every frame ([CodingGuidelines.md, section 12.6](CodingGuidelines.md#126-user-interface)). The MVVM plugin's viewmodels are a good fit once screens have many fields.
- **Layers.** Once there is more than one menu, adopt Lyra's layer idea: one root layout with named layers, and screens pushed and popped on a layer stack. Common UI provides this plus gamepad navigation and input routing; adopt it when gamepad menus are needed, and log the decision.
- **HUD extension points.** For mods and for features that add HUD elements (stamina, weather), use named slots the way Lyra's UIExtension does, rather than editing the HUD for each feature.
- All text is `FText` ([section 18](#18-localization)); 24-hour clock everywhere.

**Pitfalls:**

- Widgets that cast to the character or search the world to find their data. Give them their data when they're created, or let them find it once.
- Creating and destroying widgets often (inventory slots on every refresh). Create once, update contents.

## 7. Interaction

**Status:** built (2026-11-01, milestone 2 foundations, [Roadmap 4.1](Roadmap.md#41-foundations)) as designed below, approved by David on 2026-11-01 with the option list and N kept as a test key: `IVestigeInteractable`, `UVestigeInteractionComponent` on the player's pawn, the interact ability, the F key and the HUD prompt. The first interactable is the bed ([Roadmap 4.3](Roadmap.md#43-stamina-and-late-nights)), one placed in `L_Farm`. Since 2026-11-07 the farm grid is one too, for harvesting a ripe tile with F, and the focus goes by facing (harvesting with F, below).

**References:** Epic's "Interfaces" page (`UINTERFACE`, `BlueprintNativeEvent` functions called through `Execute_`, `Implements<U...>()`), "Collision Overview" and "Collision Response Reference" (custom object channels and collision profiles in Project Settings > Collision), and `UWorld::OverlapMultiByObjectType`. Lyra's interaction (`Source/LyraGame/Interaction`): `IInteractableTarget` fills an `FInteractionOption` list (text, sub-text, the ability to run and the target's own ability system), `UAbilityTask_GrantNearbyInteraction` runs an overlap query around the player on a timer (every 0.1 s) on Lyra's interaction channel, `UAbilityTask_WaitForInteractableTargets_SingleLineTrace` picks the one the camera looks at, and `ULyraGameplayAbility_Interact` shows the prompt and activates the option's ability, which receives the target in its event data.

**What we take from Lyra and what we skip:**

- **Take:** one interface every usable thing implements; the option list, so an object can offer several actions without the interface (a mod API) changing when the first one does; a cheap overlap query around the player on a timer, on a dedicated collision channel; using it as a gameplay ability, so tags can block it later (in a menu, in dialogue).
- **Skip:** granting each target's ability to the player and running the option's ability (Lyra's `InteractionAbilityToGrant`): the interactable does the action itself. That machinery exists for networked play, where the server must run the ability.
- **Until there is a way to choose** (a menu, or a key per option), the interact key uses the first option (decided with David, 2026-11-01). An interactable lists its options with the one the player most likely wants first.
- **Skip:** the camera line trace. Our camera is a third-person follow camera over a farm, so the farmer's aim, not the camera, says what he means: since 2026-11-07 the focus is the nearest interactable he faces (harvesting with F, below), which the facing rule this line once promised became when the farm grid and the bed could stand side by side.

**Rules:**

- **One interface, `IVestigeInteractable`** (`UINTERFACE(Blueprintable)`), so C++ actors and Blueprint actors (and so mods) can both be interactable. Two `BlueprintNativeEvent` functions, called through `IVestigeInteractable::Execute_...`:
   - `GetInteractionOptions(Interactor, OutOptions)`: fills `OutOptions` with what can be done with it now, each an `FVestigeInteractionOption` (`BlueprintType` struct): `Text`, the prompt's verb (`Sleep`), and `Action`, an `FName` the interactable chooses to tell its own options apart (`Sleep`). No options means there is nothing to do right now, and the actor isn't offered. It must be cheap and change nothing: it is asked on every scan. Later fields (a second line, an icon) are added to the struct without changing the function.
   - `Interact(Interactor, Option)`: does the chosen option. The interactable owns what happens and asks the interactor's own components for anything about the player (the bed asks the interactor's `UVestigeRestComponent`, found on its player state with `UVestigeRestComponent::Find`, to `Sleep`). The interactor never knows the interactable's class: no `if` per class.
   - Both are extension points ([Architecture.md, section 12](Architecture.md#extension-points-today)) and listed in `Vestige.ExtensionPoints.*`.
- **Interactables are found by collision, not by class.** A new object channel, `Interactable` (default response Ignore, so nothing else notices it), and a collision profile `VestigeInteractable` (object type `Interactable`, queries only, ignoring every channel: a query by object type finds it without a response) for the shape that marks where an actor can be used from: a box component a little bigger than the bed. The actor's own blocking collision stays as it is.
- **`UVestigeInteractionComponent` on the player's pawn owns finding and using interactables.** Every `ScanSeconds` (0.2 s) a timer runs one `OverlapMultiByObjectType` for `Interactable` in a sphere of `InteractionRadius` (100 cm, one tile) around the pawn, keeps the overlapping actors that implement the interface and offer at least one option the pawn faces, and focuses the nearest of those (harvesting with F, below). It gathers the options into one array it reuses, so a scan allocates nothing once warmed up. It keeps the focus as a weak pointer with its first option, keeps the latest option (its location can change while its prompt stays), and broadcasts `OnFocusChanged` only when the focused actor or that option's action or text changes. No tick; the timer stops at `EndPlay`.
- **Interacting is a gameplay ability**, `UVestigeGameplayAbility_Interact`, granted by `DA_AbilitySet_Player` and activated by its asset tag `Vestige.Ability.Interact` (a native tag). The interaction component's `Interact` activates it; the ability asks the component to rescan once (so the focus is current, not up to 0.2 s old) and calls `Interact` on the focus with its first option, then ends. It is instanced per actor, has no cost and takes no time today; an interaction that takes time later (filling the watering can) holds it active as a tool ability does.
- **Input: F interacts** (`Interact`, an input action the character maps and binds to the interaction component, shown in the key hints like the others). E stays the tool, so a tile next to the bed can still be hoed. Since 2026-11-07 F also harvests the ripe tile the farmer faces, and E only uses the item in hand (harvesting with F, below). **The bed is how the player sleeps.** Sleeping is instant: it starts the next day at 06:00, so there is no sleeping state and nothing to wake from. **N stays as a test key** that skips to the next day anywhere (decided with David, 2026-11-01), with the key hint "Skip day (test)", and moves into the cheat manager with F9 before a public build ([section 19](#19-debugging-and-cheats)). Done 2026-11-04: N is now the `SkipDay` console command. The 02:00 pass-out stays until its own roadmap item removes it.
- **The prompt on the HUD** is presentation: `AVestigeHUD` listens to `OnFocusChanged` on the possessed pawn's interaction component and shows "F Sleep" (the interact key, read from the mapping context like the other key hints, and the first option's text) while something is focused. Nothing is drawn in the world yet; an outline or a marker over the focused actor is a later presentation component.
- **The bed, `AVestigeBed`** (in `Farming/`, next to the rest component it asks): a static mesh with blocking collision (an engine cube set in C++ as a placeholder until the bed mesh in [Roadmap 4.6](Roadmap.md#46-interface-and-input)) and the interaction box. It offers one option, `Sleep`, whenever the interactor has a rest component, and interacting with it calls that component's `Sleep`, so the rules of sleeping (the refill, later bedtime and exhaustion) stay with the rest component. `BP_Bed` in `Content/Vestige/Blueprints` is the bed levels place and will hold the real mesh; one lies along the near edge of the field in `L_Farm`, so a tile can be worked from beside it, until there is a farmhouse.
- **Layers:** the interface, the interaction component and the ability need nothing above `AbilitySystem/`, and everything usable (the bed in `Farming/`, the shipping bin, robots) sits above them, so they get a layer folder of their own, `Interaction/`, right above `AbilitySystem/`. The tag joins `AbilitySystem/VestigeGameplayTags`.
- **Saving:** nothing. The focus is recomputed, and an interactable that has state (a chest) saves it as its own owner ([section 1](#1-saving-and-loading)).
- **Performance:** one overlap query on a channel only interactables use, five times a second, and a loop over the few actors it returns, with one ray against one shape for each whose option has no location (the bed); no world search, no allocation once the options array has grown, no strings rebuilt unless the focused option changed, no tick. The query's cost goes in the first performance baseline.
- **Tests** (`Vestige.Interaction.*`): the nearest interactable with options is focused, one without options is skipped, one out of range isn't found, the focus event fires only on a change, interacting calls the focused actor with its first option and does nothing without one, and the ability is activated by its tag. The bed (`Vestige.Interaction.TheBed...`) sleeps the interactor (the day advances; the refill is the rest component's, tested in `Vestige.Rest`) and offers nothing to an actor without a rest component. The ability test uses `BP_PlayerState`, so it also fails if `DA_AbilitySet_Player` doesn't grant the interact ability, and a test checks the channel and profile in `DefaultEngine.ini`. The interface functions are listed in `Vestige.ExtensionPoints`.

**Harvesting with F** (design, 2026-11-07, decided with David after the hotbar play-test, approved after three rounds of review by a senior Unreal C++ reviewer agent, whose approval counts as David's; built the same day as described). Since the hotbar ([section 8](#8-inventory-and-items)) an empty slot or an item without a tool fragment counts as the hand, a tool (`HandToolId`, `DA_Tool_Hand`) whose ability harvests, so the player harvests by selecting an empty slot and pressing E. David found that unclear, and two harvest keys would be confusing, so: **F interacts with things in the world, and a ripe crop is one of them. E only uses the item in hand, and an empty hand isn't a tool.** Facing a ripe crop shows "F Harvest", the way the bed shows "F Sleep".

References: the interaction design above; Lyra's `IInteractableTarget::GatherInteractionOptions` and `FInteractionOption` (an option names its target object, `InteractableTarget`, so one actor can offer options about something else), and `UAbilityTask_WaitForInteractableTargets_SingleLineTrace`, which picks the target the player faces. The tool abilities and their grant: [section 9](#9-abilities-attributes-and-effects-gameplay-ability-system), tools as abilities. Engine behaviour below was checked in the 5.8 source.

| Part | Kind | Change |
| --- | --- | --- |
| `FVestigeInteractionOption` | Struct (`Interaction/`) | Gains `bHasLocation` and `Location`: where in the world the option is, when that isn't its actor's location (a tile of a large actor). Added to the struct, so the interface functions don't change |
| `UVestigeInteractionComponent` | Component on the pawn (`Interaction/`) | Focuses by facing instead of nearest (below), with the facing direction from a delegate its owner binds (`FacingDirection`), else the owner's forward vector. Keeps the latest focused option even when its prompt doesn't change, and rescans once after an interaction |
| `AFarmGrid` | Actor (`Farming/`) | Implements `IVestigeInteractable`: an `InteractionVolume` box over the field and one tile around it, and the option "Harvest" while the interactor faces a ripe tile of this grid |
| `UFarmingComponent` | Component on the pawn (`Farming/`) | `HandToolId` becomes `HarvestToolId` (`VestigeTool:Base.Hand`), with a property redirect: the harvest ability is granted all the time, apart from the tool in hand, and `Harvest()` activates it once. The tool in hand is only ever the item's tool: an empty slot or an item without a tool fragment is no tool |
| `AVestigePlayerCharacter` | Character (`Player/`) | Binds the interaction component's `FacingDirection` to the targeting component's `GetAimDirection` |
| `DA_Tool_Hand` | Tool asset | Unchanged: the harvest's ability (`UFarmGameplayAbility_Harvest`), cost (1) and use time (0.4 s). It is no longer in anyone's hand, only the data of harvesting by hand |

How it works:

- **The option.** `AFarmGrid::GetInteractionOptions(Interactor)` finds the interactor's `UFarmTargetingComponent`; if its grid is this grid and its target tile holds a ripe crop (`FFarmTileState::IsRipe`), it adds `{Text "Harvest", Action "Harvest", bHasLocation true, Location the tile's centre (GetTileTransform)}`; otherwise nothing. That is a component lookup and a tile read, five times a second while the farmer is near the field, with no allocation (the options array is the scan's, reused). The scan adds one ray against one shape for each candidate whose option has no location (today the bed, when near it). The prompt follows the focus as for the bed.
- **Where it can be used from.** `InteractionVolume`, a box with its own extents: in X and Y the field plus one tile on every side, so a farmer standing just off the field and facing in is found; in Z from 50 cm below the grid to 200 cm above it, so the scan's sphere (100 cm around the capsule's centre, about 96 cm above the feet) overlaps it on uneven ground and mid-jump, as the bed's volume has a height of its own (`InteractionExtent`, `InteractionLocation`). Its collision is set in the constructor: the `VestigeInteractable` profile (which ignores `Pawn`, so it doesn't affect navigation), overlap events off and `SetCanEverAffectNavigation(false)`. `UpdateBounds` sizes it with `Bounds`, and `BeginPlay` now calls `UpdateBounds` too, since `L_Farm`'s saved grid predates the component and `OnConstruction` doesn't run for it in a cooked game.
- **Facing, not nearest.** Today the scan focuses the nearest interactable by actor location, and next to the bed that sleeps (instant, and it ends the day) when the player meant to harvest. The scan now asks, for each candidate's first option, whether the pawn faces it, and focuses the nearest faced one:
   1. **An option with a location** (`bHasLocation`, the crop) is faced when its location is within `FrontHalfAngleDegrees` (50) of the facing direction on the ground plane, or within 1 cm of the pawn. Its distance is to that location.
   2. **An option without a location** (the bed) is faced only when the facing ray hits the shape the scan found: `Overlap.GetComponent()->LineTraceComponent(...)` from the pawn along the facing direction, `2 * InteractionRadius` long, at the height of the shape's centre (`Bounds.Origin.Z`) so any shape height works. In 5.8 `UPrimitiveComponent::LineTraceComponent` traces that one body directly, with no channel filter, so the query-only `VestigeInteractable` profile is hit; it is one ray against one shape, with no world query and no allocation. Its distance is to the hit point. A ray that starts inside a shape isn't relied on (the farmer's capsule, 42 cm in radius, can't bring his centre inside the bed's box, which is 20 cm larger than the bed).
   3. **Among faced options the nearer wins.** Not the smallest angle: a ray-faced bed has an angle near 0 and would beat a ripe crop standing between the farmer and the bed.

   Why the bed needs the ray: the bed (200 by 100 cm, its box 120 by 70 cm half-extents) lies along the field edge, and a farmer on the edge row has its centre about 92 cm to his side. Aiming along the edge with the bed's centre d cm ahead, its angle is atan(92 / d), under 50 degrees once d passes about 77 cm, while the scan still finds its box up to d of about 217 cm: beside the bed's foot half and up to a metre past it, an angle test would focus the bed (43 degrees at d = 100, 31.5 at d = 150) and F with an unripe target would sleep. Along the edge the facing ray passes 92 cm from the bed's centre, outside its 70 cm half-width, and misses; facing the bed's long side anywhere, or its end from the ring of tiles around the field, hits. Narrowing the cone wouldn't do: facing the bed's long side near one end, its centre is about 47 degrees off.

   The facing direction is the aim, not the actor's rotation: `FacingDirection` is a delegate returning a ground direction, which `AVestigePlayerCharacter` binds to `UFarmTargetingComponent::GetAimDirection` in `BeginPlay` and unbinds in `EndPlay`, and which falls back to the owner's forward vector when unbound or zero (other pawns). The aim is snapped to eight directions, and while walking the actor faces the raw movement direction, up to 22.5 degrees away, so the actor's rotation would put the target tile up to about 67.5 degrees off; measured from the aim, the target tile's centre is at most 45 degrees off for an orthogonal aim (the farmer at a corner of his tile) and about 27 degrees for a diagonal one, inside 50. The aim is snapped to the grid's eight directions everywhere in the level, so near an interactable away from the field the facing used can be up to 22.5 degrees off the farmer's visible facing while he walks; he turns to the aim when he stops, which is enough for the bed and for interactables to come, but a future interactable that needs finer facing should know it. This replaces the 2026-11-01 nearest-wins rule (decision log); an actor whose first option isn't faced is skipped, as an actor without options is today.
- **The focused option stays current.** `SetFocus` always copies the new option into `FocusedOption` and broadcasts `OnFocusChanged` only when the actor or `IsSameAs` (action and text) changes, so moving from one ripe tile to the next updates the option's `Location` without redrawing the prompt. `InteractWithFocus` rescans once after `Execute_Interact`, so the prompt clears as soon as the harvest is done instead of up to 0.2 s later.
- **Interacting.** `AFarmGrid::Interact(Interactor, Option)` does nothing unless `Option.Action` is `Harvest` (as the bed checks its `SleepAction`); then it calls the interactor's `UFarmingComponent::Harvest()`. `Harvest()` calls `FollowInventory()` first, as `UseTool` does (it also keeps the harvest grant current), then activates the harvest spec by its handle without pressing its input. Its input is never pressed, so `IsKeyHeld()` is false and the ability does one use: it reads the target tile from the targeting component, harvests (or says "No room in your bag."), commits the cost, reports `On Crop Harvested` and `On Tool Used` (with `DA_Tool_Hand` as the tool, as today), and stays active for the use time, then ends. It runs inside the interact ability's activation, which is allowed: `InternalTryActivateAbility`'s `ABILITYLIST_SCOPE_LOCK` only defers adding and removing specs, and the harvest spec already exists; the interact ability's `CommitAbility` has no cost or cooldown. While a tool ability is active, `Vestige.State.UsingTool` blocks the harvest: during a use's time, and also while a tool ability waits idly with E held. Then F does nothing although the prompt says "F Harvest"; the interact ability still ends. F isn't held to repeat; harvesting a row by holding F is a later item.
- **E with nothing to use.** `GetCurrentTool()` returns the item in hand's tool, or nullptr for an empty slot, an item without a tool fragment, or a tool that isn't loaded. `StartUsingTool` marks the key held (the slower walk) only when there is a tool in hand; with none and an inventory reachable, it says "Nothing in hand to use." on the press, and logs the missing-tool warning on the press when the item names a tool that isn't loaded (the hand-change event can't, since an empty slot and such an item are both nullptr). A press whose tool's grant is still queued finds no spec and does nothing quietly, as today. The HUD's status line still says "Empty hand" or the item's name.
- **The two grants.** The harvest grant has its own state, `HarvestSpec` and `HarvestAbilitySystem` (weak), apart from the tool grant's `GrantedSpec`, `GrantedAbilitySystem` and `GrantedTool`, since the tool grant's state empties whenever the hand does. `UpdateGrantedTool` first updates the harvest grant (when the reached ability system differs from `HarvestAbilitySystem`: release the old, give the new with `DA_Tool_Hand` as the source object), then the tool grant as today. `ReleaseGrantedTool` becomes `ReleaseGrant(FGameplayAbilitySpecHandle&, TWeakObjectPtr<UAbilitySystemComponent>&)`, used for both: an active spec is released and removed when it ends, otherwise `ClearAbility`, then the handle and pointer are reset. `EndPlay` and an ability system change release both. Selecting slots never touches the harvest grant. A pawn that reaches an ability system has the harvest granted, plus the tool's when the item in hand is a tool. [CodingGuidelines.md, section 12.3](CodingGuidelines.md#123-memory-and-allocation)'s grant exception gains the harvest's one ability object per possession.
- **The rename.** `DefaultEngine.ini` gets `+PropertyRedirects=(OldName="/Script/Vestige.FarmingComponent.HandToolId",NewName="/Script/Vestige.FarmingComponent.HarvestToolId")` under `[CoreRedirects]`, so `BP_PlayerCharacter`'s value loads; a temporary commandlet, not committed, re-saves the Blueprint with the new name (the redirect alone would load it). A test checks the Blueprint's `HarvestToolId`.
- **Layers.** `AFarmGrid` (`Farming/`) implements an `Interaction/` interface and asks `Farming/` components; the character wires the targeting component into the interaction component. The interaction component still knows no farming.
- **Saving.** Nothing changes.

Not in this change (into [Roadmap.md](Roadmap.md)): holding F to harvest along a row; an outline or marker on the focused tile or bed (the frame already shows the target); interacting with other tile contents (a placed sprinkler gets its own actor, [section 16](#16-placing-objects-and-building)).

Tests (`Source/Vestige/Tests/`):

- `Vestige.Interaction`: the fixture's interactor gets a root component so it can turn, and `AVestigeTestInteractable::Offer` an overload with a location. New: `OnlyAnOptionInFrontCanBeFocused` (an option with a location beside at 60 degrees: nothing focused), `AmongFacedOptionsTheNearerWins`, `AnOptionWithoutALocationIsFacedOnlyWhenTheFacingRayHitsItsShape` (a test interactable with a box beside the facing ray: not focused; on it: focused), `AnOptionsLocationIsUsedForItsAngle` (an actor whose actor location is behind but whose option's location is in front is focused), `TheFacingDelegateIsUsedOverTheActorsRotation`, and `MovingBetweenOptionsWithTheSamePromptUpdatesTheLocation` (the focused option's location follows; no second focus event). The existing focus tests turn the interactor to face their interactables.
- `Vestige.Farming`, with the Blueprint farmer, `AFarmGrid` and a real `AVestigeBed`: `FacingARipeTileOffersHarvest` (the grid's options, with the tile's location), `AnUnripeOrEmptyTileOffersNothing`, `NextToTheBedTheFacedRipeTileWins` (the review's case: the farmer at the back of an edge tile beside the bed, facing along the edge, and with the actor turned 22.5 degrees off the aim as while walking), `FacingTheBedFocusesTheBed` (facing its long side near one end, and its end from the ring), `NextToTheBedAnUnripeTargetFocusesNothing` (on the edge row aiming along the edge, with the bed's centre 60, 100 and 150 cm ahead), `InteractingWithARipeTileHarvestsIt` (through the interaction component's `Interact`: the yield in the bag, the tile tilled, the hand's cost, the events `CropHarvested` then `ToolUsed Base.Hand`, and no focus after the interaction's rescan), `HarvestingWorksWithAnyItemInHand` (the hoe and the seeds in hand), `FDuringAToolUseDoesntHarvest` (the interact ability ends; the crop stays), `AnEmptyHandUsesNothing` (E with an empty slot: no tile change, no cost, one message, the walk not slowed), `TheHarvestIsAlwaysGranted` (one harvest spec across slot changes, plus the tool's; a new pawn leaves neither behind), `TheGridsInteractionVolumeCoversTheFieldAndOneTileAround` (a real scan with the farmer standing, actor Z the grid's plus the capsule's half-height, on a tile of the ring around the field, facing in at a ripe edge tile) and `TheFarmerBlueprintNamesTheHarvestTool`.
- Changed: the harvest tests that held the hand (`HarvestingAddsTheCropsYieldOfItsItem`, `AFullBagLeavesTheCropInTheGround`, `HarvestingTellsBlueprintsTheCropThenTheTool`) call `Harvest()`; `AnEmptyHandOrAnItemWithoutAToolHarvests` is replaced by the tests above; `PlantingTheLastSeedLeavesTheHandEmpty` and its repeat variant expect only the harvest spec after the use; `ASecondHandChangeDuringTheFirstUseLeavesOneToolAbility` selects the watering can when the tool in hand becomes none (its grant is queued under the lock) and the hoe when it becomes the can (the can's queued grant is cleared), and expects the harvest and the hoe; `ANewToolNeedsOnlyItsAbility` uses a transient tool and a transient item with a tool fragment, given to the farming component with a new `AddToolForTests` hook; `GetToolAbilities()` tells the harvest spec (source `DA_Tool_Hand`) from the tool's, and every count of tool abilities (`SwitchingToolsDuringAUseWaitsForTheUseToFinish`, `ANewPawnLeavesNoToolAbilityBehind`, the grant tests) counts the tool's apart from the harvest's.
- `Vestige.Interaction.TheBed...` keep their checks with the interactor facing the bed.
- `Vestige.PlayerCharacter.HoldingTheUseKeyHalvesTheWalkingSpeed` and `HoldingTheUseKeySlowsJumpsToo` broadcast the farming component's held event instead of pressing E on a farmer with nothing in hand, which no longer slows the walk.

Docs in the same commit:

- This guide: this section (status, the "facing rule" line under what we skip, the rules for the option and the scan, the performance line); section 8 (the hotbar block's "empty hand harvests" bullets and tests marked superseded by this); section 9 (who owns what: the harvest grant); section 5 (F interacts with the bed and ripe crops; E uses the item in hand); section 22 (the `AFarmGrid` rules gain the interaction volume and the interface).
- [Architecture.md](Architecture.md): section 12's extension point row for `IVestigeInteractable` names the option's new `bHasLocation` and `Location`; section 13's rows for the grid, the interaction component, the farming component and the character; a decision log row that replaces the 2026-11-01 nearest-wins focus with facing and supersedes the hand-as-tool part of the hotbar entry.
- [CodingGuidelines.md, section 12.3](CodingGuidelines.md#123-memory-and-allocation): the grant exception covers the harvest grant.
- `Config/DefaultEngine.ini`: the property redirect.
- [Roadmap.md](Roadmap.md): a done item in 4.6, the "harvesting while holding a tool" item resolved, the empty-hand hint dropped from 4.6's hotbar item, the items above added.
- [PlayTest.md](PlayTest.md): the hotbar checks that harvest with the empty hand and "Can you tell how to harvest?" answered and superseded, the smoke test's harvest steps (the empty hand, 4) rewritten for F, and new checks for the prompt on a ripe crop, next to the bed, and E with an empty hand.

**Pitfalls:**

- One `if` per interactable class in the farmer's code: the reason the interface exists.
- Calling an interface function directly (`Cast<IVestigeInteractable>(Actor)->Interact(...)`): a Blueprint that implements the interface has no C++ interface behind it, so the cast returns null and the Blueprint is never asked. Always call `IVestigeInteractable::Execute_Interact(Actor, ...)` and test with `Actor->Implements<UVestigeInteractable>()`.
- Interactables on the `Visibility` or `Pawn` channels, or a scan with `OverlapMultiByChannel` on a shared channel: every query would also return the farm tiles and other actors. Keep the dedicated object channel.
- An options function with side effects or a load: it runs five times a second while the player is near.

## 8. Inventory and items

**Status:** milestone 2, in two steps. **Item definitions** (`UVestigeItemData` and its fragments) are built as designed below, approved by David on 2026-10-30, with four items so far: `Base.TurnipSeeds`, `Base.Turnip`, `Base.Hoe` and `Base.WateringCan`. **The inventory** (`UVestigeInventoryComponent` on `AVestigePlayerState`) is built as designed below, approved by David on 2026-10-31: seeds and harvests are items in it. **The hotbar** (its first nine slots, with the item in hand; tools are items with a tool fragment) is built as designed below, 2026-11-07 ([CodeReview.md, H2](CodeReview.md#h2--ufarmingcomponent-does-seven-jobs-and-can-plant-only-one-seed)), and the player starts with a hoe, 10 `Base.TurnipSeeds` and a watering can. Money waits for the shipping bin.

**References:** Epic's "Data Assets" and "Asset Management" pages, and `UAssetManager::InitializeAssetBundlesFromMetadata`, which also follows instanced subobjects, so a soft reference inside a fragment joins its item's asset bundles. Lyra's inventory (`Source/LyraGame/Inventory`): `ULyraInventoryItemDefinition` (an item made of **fragments**, small `DefaultToInstanced, EditInlineNew` objects each adding one aspect such as a pickup icon, a quick bar icon, stats or equipment, found with `FindFragmentByClass`), `ULyraInventoryItemInstance` (one item the player has, with stats), `ULyraInventoryManagerComponent` (the list) and a separate equipment manager.

**What we take from Lyra and what we skip:**

- **Take:** fragments for what only some items have; `FindFragmentByClass` as the one way to ask for one; definitions without runtime state; the inventory as a component apart from equipment (the tool in hand).
- **Skip:** definitions as Blueprint classes (Lyra's items are `TSubclassOf<ULyraInventoryItemDefinition>`). Ours are data assets of a primary asset type, found by `ContentId` like crops and tools, so saves and mods name them the same way ([section 2](#2-content-and-the-asset-manager)).
- **Skip until needed:** item instance objects and `OnInstanceCreated` (only for items with their own state, such as a tool's durability); stats as tag stacks (Lyra counts ammo that way; our stacks are an ID and a count); the replicated `FFastArraySerializer` list (single-player).

**Rules for item definitions:**

- **One asset per item:** `UVestigeItemData`, a `UVestigeContentData` subclass of primary asset type `VestigeItem`, named `DA_Item_<Name>` in `Content/Vestige/Items`, always cooked. Seeds, harvested crops, materials and later tools and food are items.
- **What every item has** sits on `UVestigeItemData`: the ID and display name from the base class, `MaxStackSize` (how many fit in one inventory slot; 1 for items that don't stack) and `SellPrice` (what selling one pays; unused until there is currency). Nothing else goes on the base class.
- **What only some items have is a fragment:** a subclass of the abstract `UVestigeItemFragment` (`DefaultToInstanced, EditInlineNew`), in the item's `Fragments` list (`UPROPERTY(EditDefaultsOnly, Instanced)`). Code asks with `FindFragment<T>()`, a linear search over the few fragments an item has. A new kind of item is a new fragment, never a subclass of `UVestigeItemData` and never a `switch` over item types.
- **The first fragment is `UFarmSeedFragment`:** its `CropId` (`FPrimaryAssetId`, limited to `FarmCrop`) is the crop that planting the item grows.
- **Crops name the item they yield:** `UFarmCropData::HarvestItemId` (`FPrimaryAssetId`, limited to `VestigeItem`). `Yield` stays on the crop, because how many grow is the crop's rule.
- **Prices belong to items, not crops.** `SeedPrice` and `SellPrice` leave `UFarmCropData` (both unused and zero on the turnip): the sell price is on the harvested item and on the seed item. What a shop charges is the shop's stock data, decided when shops are built, so two shops can price the same item differently.
- **Content refers to content by ID,** never by a hard pointer ([section 2](#2-content-and-the-asset-manager), pitfalls), so loading a seed item doesn't load its crop's meshes, and a missing crop is a skipped ID rather than a broken asset.
- **Fragments come with the system that reads them:** the tool fragment (`UVestigeToolFragment`, naming a `VestigeTool` ID) came with the hotbar (below), a food fragment with stamina, a placeable fragment with placement ([section 16](#16-placing-objects-and-building)), and an icon (a soft texture in the `Visuals` bundle) and description with the inventory screen. Item category tags wait for the shipping bin or gifts.
- **Data validation** (`IsDataValid`) on top of the ID check: `MaxStackSize` at least 1, no empty fragment entries, at most one fragment of each class, and each fragment checks its own fields (a seed's `CropId` is set and names a crop the Asset Manager knows). `HarvestItemId` on a crop is checked the same way.
- **Loading:** items have no meshes or textures yet, so a definition is a few hundred bytes. Which definitions are loaded, and when, is the inventory's decision; the rule from [CodingGuidelines.md, section 12](CodingGuidelines.md#12-performance) stands: an item that can enter the inventory during play (a harvest) has its definition loaded beforehand, never synchronously at that moment.

**Rules for the inventory:**

- **The inventory is a component on the player state,** so it outlives the pawn ([Architecture.md, section 2](Architecture.md#2-unreals-gameplay-framework-where-things-go)). `AVestigePlayerState`, in `Player/`, for now holds only `UVestigeInventoryComponent` (created in its constructor); the ability system component joins it with stamina ([section 9](#9-abilities-attributes-and-effects-gameplay-ability-system)). `AVestigeGameMode` sets it as the player state class, and `BP_PlayerState`, a Blueprint subclass in `Content/Vestige/Blueprints`, holds the values (the starting items), as the farmer Blueprint does for the farmer. `BP_ThirdPersonGameMode` names it.
- **The rules sit in a plain struct, `FVestigeInventory`,** as the tile's rules sit in `FFarmTileState`, so they are tested without a world. The component wraps it with loading, events and saving. Both live in the layer folder `Inventory/`, between `Time/` and `Farming/`: the farming component uses the inventory, and the inventory knows nothing about farming. Nothing below `Player/` knows `AVestigePlayerState`; the farming component and the save subsystem find the inventory with `FindComponentByClass` on the player's `APlayerState`, the same way the save subsystem finds the farming component on the pawn.
- **A fixed number of slots** (`SlotCount`, 24, on the component's defaults). A slot is empty or holds one item definition and a count from 1 to the item's `MaxStackSize`. A bigger bag later raises the count.
- **Adding and removing are all or nothing.** `TryAddItem(ItemId, Count)` first tops up stacks of the same item in slot order, then fills empty slots in order, and changes nothing if the whole count doesn't fit. `TryRemoveItem(ItemId, Count)` takes from the last stacks of that item first and changes nothing if there aren't enough. `GetItemCount(ItemId)` sums the stacks, and `CanAddItem` asks without changing anything. Callers that turn one thing into another (a ripe crop into items) ask first, so a full bag leaves the crop in the ground instead of destroying it. Partial adds wait for a caller that needs them (picking up a pile).
- **The API takes item IDs** (`FPrimaryAssetId` of type `VestigeItem`), the form content uses for references (`HarvestItemId`). An ID that names no item is refused with a warning.
- **Loading:** at `BeginPlay` the component loads every `VestigeItem` definition the Asset Manager knows, without bundles, and keeps them in a map by ID, so adding an item during play never waits on a load ([CodingGuidelines.md, section 12](CodingGuidelines.md#12-performance)). Definitions have no meshes or textures (an icon will be in the `Visuals` bundle, loaded by the inventory screen), so all of them together stay small. Revisit, with a measurement, if items ever get heavy or number in the thousands.
- **Starting items** (`StartingItems`, a list of `FVestigeItemStack` item IDs and counts on the component, 10 `Base.TurnipSeeds` on `BP_PlayerState`) are added at `BeginPlay`. Loading a save replaces them.
- **One event, `OnInventoryChanged`** (`FOnVestigeInventoryChanged`, a C++ multicast delegate with no parameters), broadcast after every change, including a restore. The HUD listens to it on the player state, not on the pawn.
- **Seeds and the harvest are items.** `UFarmingComponent` uses what is in hand and has no counts of its own. Planting plants the seed item in hand and removes one from its slot (the hotbar, below); harvesting adds the crop's `Yield` of its `HarvestItemId`, and if the bag can't take them, the crop stays and the player is told ("No room in your bag."). The abilities ask for the inventory on each use, never at `BeginPlay`.
- **The hotbar is the inventory's first slots** since 2026-11-07, and tools are items with a tool fragment; the toolbar the farming component had is gone (the hotbar, below; decision log).
- **Money is not built yet.** Nothing earns or spends it until the shipping bin and shops. It arrives with the first of them, as its own decision (a count on the inventory component or a small wallet component on the player state), rather than as an unused field now.
- **Saving:** the component returns an `FVestigeInventorySaveData` (in `Inventory/`) holding one entry per non-empty slot: the slot index, the item's `ContentId` and the count. Restoring clears the slots and puts each entry back in its slot. An entry naming an unknown item is skipped with a warning and counted as missing content. A count above the item's stack size (its `MaxStackSize` was lowered, or a mod changed it) fills its slot and the rest goes in as a normal add; what doesn't fit is dropped with a warning and counted as missing. A bad slot index goes in as a normal add.
- **The HUD** shows the non-empty slots as "Bag: Turnip Seeds 9   Turnip 2", by display name, rebuilt on `OnInventoryChanged`, until the UMG inventory screen replaces the line.
- **Performance:** no tick. Adding, removing and counting walk the 24 slots, only on a player action. Loading the definitions has a `TRACE_CPUPROFILER_EVENT_SCOPE`.
- **Tests** (`Vestige.Inventory.*`): stacking into existing stacks before empty slots, the stack size limit, all-or-nothing adds and removes, counts across stacks, unknown IDs, the change event, the save round trip with unknown items, lowered stack sizes, bad slot indexes and no room left, and `ThePlayerStartsWithTurnipSeeds`, which checks that the game mode's player state class has an inventory with the starting seeds, so a missing editor step fails a test. The farming tests (`Vestige.Farming`) plant from a seed item, plant nothing without seeds or a player state, harvest the crop's item, and leave the crop when the bag is full.

**The hotbar, the item in hand and targeting** (design for [CodeReview.md, H2](CodeReview.md#h2--ufarmingcomponent-does-seven-jobs-and-can-plant-only-one-seed), 2026-11-07, approved after three rounds of review by a senior Unreal C++ reviewer agent, whose approval counts as David's; built the same day as described). Today `UFarmingComponent` keeps a toolbar of its own (`ToolbarIds` loaded into an `FFarmToolbar`), the one seed the Seeds tool plants (`SeedItemId`), the aim and the target tile, the granted ability of the tool in hand, the use key and the extension points, and it saves the tool in hand under `Base.Farming`. So the toolbar is a second inventory next to the real one, and only one kind of seed can be planted, while [4.2](Roadmap.md#42-farming-depth) brings three or four. After this change the hotbar is the first slots of the inventory, the item in the selected slot is what the player holds, an item is a tool through a fragment, seeds are planted from the stack in hand, and the aim and the target tile are a component of their own.

References: Lyra's `ULyraQuickBarComponent` (`Source/LyraGame/Equipment`): a controller component with `NumSlots` (3) slots that point at inventory item instances and an `ActiveSlotIndex`. `SetActiveSlotIndex` unequips the old slot's item and equips the new one when it has a `UInventoryFragment_EquippableItem`, whose `EquipmentDefinition` the pawn's `ULyraEquipmentManagerComponent` equips, granting its ability sets with the item instance as their source. Epic's `APawn::ReceiveControllerChangedDelegate`, and `UAbilitySystemComponent::GiveAbility`, `ClearAbility` and `FindAbilitySpecFromHandle` while the ability list is locked (below). Engine behaviour below was checked in the 5.8 source.

What we take from Lyra and what we skip:

- **Take:** an active hotbar slot whose item is what the player holds; an item that is used through a fragment naming a separate definition of how it is used (Lyra's equippable fragment names an equipment definition, ours names a tool asset), so every seed item shares the one Seeds tool; equipping on the pawn by granting the definition's ability while the item is held and taking it back when it isn't.
- **Skip:** a quick bar with slots of its own that point into the inventory. Ours is the inventory's first slots, as in farming games, so the hotbar and the bag can't disagree and moving an item is one change. Skip the controller component: the selection lives with the slots on the player state, so it is saved with them and outlives the pawn. Skip replication and gameplay messages; C++ delegates as everywhere else.

| Part | Kind | Owns |
| --- | --- | --- |
| `UVestigeInventoryComponent` | Component on the player state (`Inventory/`) | Gains the hotbar: `HotbarSlotCount` (9 on its defaults, never more than `SlotCount`), the first slots, which keys 1 to 9 select; the selected hotbar slot; the item in hand (the item in that slot, or none) and `OnItemInHandChanged`. Saves the selected slot. Still knows nothing about tools or farming |
| `UVestigeToolFragment` | Item fragment (`Content/`), new | `ToolId` (`FPrimaryAssetId`, limited to `VestigeTool`): holding the item uses that tool. `IsDataValid` checks it the way the seed fragment checks `CropId` |
| `UVestigeToolData` | Data asset (`Content/`) | Unchanged: the ability, the cost, the use time and the held mesh. Tools stay assets of their own, so the four tool abilities, their tests and the spec's source object don't change |
| `UFarmTargetingComponent` | Component on the pawn (`Farming/`), new | The aim and the target tile, moved unchanged from the farming component: finding the grid, `AimAt`, `GetAimDirection`, `GetTargetTile`, `GetGrid` and `OnTargetTileChanged`, following the owner's root component from tile to tile and the grid's `OnFieldReady`, and the "No FarmGrid in the level" warning |
| `UFarmingComponent` | Component on the pawn (`Farming/`) | Using what is in hand: the tool of the item in hand, granting its ability, the use key's press and release, the held flag (for the walk speed), the extension points. Loses the toolbar, `SeedItemId`, targeting and its save. Gains `HandToolId` and loading the tools and crops |
| `AVestigePlayerCharacter` | Character (`Player/`) | Keys 1 to 9 select hotbar slots on its player state's inventory, movement aims the targeting component, and the key hints name the hotbar's items. Creates the targeting component next to the farming component |

How it works:

- **The hotbar is the first slots of the bag.** The hotbar is the first `GetHotbarSlotCount()` slots, which is `HotbarSlotCount` but never more than the slots there are (tests reset to small inventories). Adding items still fills empty slots in order, so a new item goes to the hotbar while it has room. `SelectHotbarSlot(Slot)` selects a hotbar slot, and returns false and changes nothing for a slot outside the hotbar. `GetSelectedHotbarSlot()` and `GetItemInHand()` (the selected slot's item definition, or nullptr when the slot is empty) read it. A new game, a reset and a restore start from slot 0. An inventory that isn't the player's (a chest, later) sets `HotbarSlotCount` to 0: it has no hand, its selected slot is `INDEX_NONE` and it never broadcasts the hand's event, which is simpler than a separate hotbar component for the one inventory that has one.
- **One event for the hand.** `OnItemInHandChanged` (a C++ multicast delegate without parameters, like `OnInventoryChanged`) fires when the selected slot changes or the item in it changes: the slot empties (the last seed planted), fills (a harvest lands in the empty selected slot), or a restore, a reset or `StartNewGame` replaces it. A change of count alone doesn't fire it. The component remembers the slot and item of its last broadcast and compares them after every change, and broadcasts after `OnInventoryChanged`, so a listener to both sees the slots already updated. It updates what it remembers before it broadcasts, so a listener that changes the inventory again (selects another slot from the handler) is compared against the latest state and the component never broadcasts a change twice. That doesn't order what listeners see: a multicast `Broadcast` calls a copy of its listener list from last to first (`MulticastDelegateBase.h`), and the list is compacted with `RemoveAtSwap`, so during a nested broadcast a listener the outer one hasn't reached yet is called for the newer change first and the older one after. So the event carries no value, and every listener reads `GetSelectedHotbarSlot()` and `GetItemInHand()` when it is called, which is always the latest state. The farming component's `OnToolChanged` loses its parameter for the same reason: the character's handler shows `Farming->GetCurrentTool()`'s held mesh instead of trusting a passed tool. The comparison is one index and one pointer.
- **Removing from the stack in hand.** `FVestigeInventory::TryRemoveFromSlot(Slot, Count)` removes from one slot, all or nothing, and `UVestigeInventoryComponent::TryRemoveItemFromSlot(Slot, Count)` wraps it and broadcasts. Both know nothing about farming; the plant ability uses them on the selected slot.
- **Tools are items.** The hoe and the watering can become items, `DA_Item_Hoe` and `DA_Item_WateringCan` (stack size 1), each with a tool fragment naming its tool (`VestigeTool:Base.Hoe`, `VestigeTool:Base.WateringCan`). `DA_Item_TurnipSeeds` gains a tool fragment naming `VestigeTool:Base.Seeds` next to its seed fragment, and so will every seed item. The item IDs reuse the tools' names (`VestigeItem:Base.Hoe`), which is allowed: IDs are unique per content type (`Vestige.Content.TheSameNameInTwoContentTypesIsNotADuplicate`). `BP_PlayerState`'s starting items become a hoe, 10 turnip seeds and a watering can, in that order, so keys 1, 2 and 3 stay the hoe, the seeds and the can as on the toolbar today.
- **The empty hand harvests.** Superseded on 2026-11-07 by harvesting with F ([section 7](#7-interaction)): an empty hand is no tool, and the hand's tool is the harvest's data, granted all the time as `HarvestToolId`. As built first: the hand isn't an item: `UFarmingComponent::HandToolId` (limited to `VestigeTool`, `VestigeTool:Base.Hand` on `BP_PlayerCharacter`) is the tool used when the item in hand has no tool fragment or the selected slot is empty. So an empty slot or a turnip in hand harvests. Without an inventory (a pawn without a player state) there is no tool.
- **Which tool is in hand.** `GetCurrentTool()` asks the inventory each time: the tool its item in hand's fragment names, looked up in the tools the component loaded (else the hand tool until harvesting with F; now no tool). The farming component reaches the inventory through its pawn's player state (`FindComponentByClass`, as the HUD does) and keeps only a weak pointer to the inventory it listens to, never a copy of the item. A tool item whose tool isn't loaded (its asset is missing) falls back to the hand, with a warning when it comes into hand.
- **Following the inventory.** One function, `FollowInventory()`, finds the inventory on the pawn's player state now and, if it isn't the one in the weak pointer, unbinds `OnItemInHandChanged` from the old one and binds it on the new one; then it runs `HandleItemInHandChanged`. It is called at the end of `BeginPlay` (after loading), from `FollowController` (bound to the pawn's `ReceiveControllerChangedDelegate`, which fires after `PossessedBy` sets and `UnPossessed` clears the player state, `Pawn.cpp`), and at the top of `StartUsingTool` and `UseTool`; `EndPlay` unbinds. `BeginPlay` matters because the pawn a map starts with is possessed before the world begins play (`UGameInstance::StartPlayInEditorGameInstance` spawns the play actor before `BeginPlay`, and so does `UEngine::LoadMap`), so its controller-changed event fires before the component binds to it. The player state's inventory may not have restored its slots yet when the pawn's `BeginPlay` runs; it then broadcasts the hand when it restores, and the component is already bound. The press calls cover a pawn given a player state without a controller. `HandleItemInHandChanged` runs `UpdateGrantedTool` and broadcasts `OnToolChanged` when the tool in hand differs from the last one it broadcast, which it remembers apart from `GrantedTool` (empty while there is no ability system), so a pawn without a player state still broadcasts no tool and its held mesh goes blank. The farming test fixture possesses the farmer with a player controller, as `ANewPawnLeavesNoToolAbilityBehind` does, so the tests run the event path.
- **Granting doesn't change, except one case.** `UpdateGrantedTool` and the spec's source object (the tool asset) work as H1 built them; only where the tool comes from changes. A switch to another tool while a use is under way still waits for the use to finish, and selecting a slot whose item uses the same tool (another stack of seeds, or a second kind of seed) changes nothing about the grant, so a held key goes on with the new item. The new case is the selected slot emptying inside a use: planting the last seed removes it inside `UseOn`, the inventory broadcasts, and the farming component takes the plant ability back and grants the hand's. That happens two ways:
   - **The first use** runs inside `TryActivateAbility`, which holds the ability system's ability list lock (`ABILITYLIST_SCOPE_LOCK` in `InternalTryActivateAbility`). `GiveAbility` then queues the new spec in `AbilityPendingAdds` and returns its handle, and it is added when the lock is released. `FindAbilitySpecFromHandle` with its default (`EConsiderPending::PendingRemove`) returns null for a queued spec, so a press in that moment finds no spec and does nothing. Never ask with `EConsiderPending::PendingAdd`: its loop over the queued specs in 5.8 doesn't compare handles and returns the first one.
   - **A repeat** runs from the `WaitDelay` callback, without the lock, so `GiveAbility` adds the spec at once and may reallocate the ability list; nothing keeps a spec pointer across it (H1's rule already).

   Taking a grant back must also work for a spec that is still queued: a second hand change inside the same locked first use (a Blueprint handler of `On Tool Used` or an `OnInventoryChanged` listener that selects another slot) releases the hand's queued grant, and `SetRemoveAbilityOnEnd` looks the spec up with the default and finds nothing, so it would stay granted forever. So `ReleaseGrantedTool` releases the input and calls `SetRemoveAbilityOnEnd` only when it finds the spec active, and otherwise calls `ClearAbility(GrantedSpec)`, which removes a queued spec from `AbilityPendingAdds`, marks an inactive one for removal while the list is locked, removes it at once otherwise, and does nothing for a handle it doesn't know. The plant ability, its input released, finishes its use's time and ends, so the repeat stops, as switching tools does; the next press harvests.
- **Planting from the item in hand.** The plant ability reads the selected slot and the item in hand from the inventory on the ability system's owner (`GetInventory()`), the item's `UFarmSeedFragment`, and the crop that names from the farming component (`FindCrop`). It removes one from the selected slot with `TryRemoveItemFromSlot`, so the seeds leave the stack the player holds and a second stack elsewhere stays as it is. When the item in hand has no seed fragment or its crop isn't loaded, `UseOn` says "Nothing to plant in hand." and `CanUseOn` is false. Two seed items in two slots each plant their own crop. A content test checks that every item with a seed fragment also has a tool fragment, since a seed without one would harvest instead of plant.
- **Loading.** At `BeginPlay` the farming component loads every `VestigeTool` and every `FarmCrop` the Asset Manager knows, with their `Visuals` bundles, in one batched `LoadPrimaryAssets` with one wait, as the inventory loads every item definition, and keeps them in maps by primary asset ID: holding or planting anything then never waits on a load. The asset manager keeps its handles, so a later pawn or map finds them in memory. Today that is the four tools and the one crop that `ToolbarIds` and `SeedItemId` loaded; 4.2 brings three or four crops. If crops ever number in the dozens, load the crops of the seeds in the bag instead, with a measurement first. The load has a `TRACE_CPUPROFILER_EVENT_SCOPE`. `FindCrop` and the tool lookup are map finds, and nothing loads after `BeginPlay`.
- **Targeting moves as it is.** `UFarmTargetingComponent` keeps the functions, the event and the rules the farming component has for them today, so `Vestige.Targeting` keeps its checks. The target highlight follows its `OnTargetTileChanged`, the character's `DoMove` calls its `AimAt` and `GetAimDirection`, and the use-tool ability finds it on the avatar when it activates, as it finds the farming component, and reads the target tile and the grid from it and listens to its event. The ability ends at once without a targeting component, as it does today without a farming component.
- **Input.** Keys 1 to 9 call the character's `SelectHotbarSlot(Slot)`, which forwards to the inventory on its player state and does nothing without one. Slots beyond the ninth have no key until hotbar input is designed with the input assets ([H6](CodeReview.md#h6--input-is-built-in-c-with-hard-coded-keys)).
   - Since H6 (2026-11-07, [section 5](#5-input)): the mouse wheel and the gamepad shoulders call the character's `StepHotbarSlot(+1 or -1)`, which forwards to `UVestigeInventoryComponent::StepHotbarSlot(Direction)`: which slot is selected is the inventory's rule. It selects the next or previous hotbar slot by the sign of `Direction`, wrapping around and counting empty slots, through `SelectHotbarSlot`, so `OnItemInHandChanged` broadcasts; from no selection it starts at the first slot (next) or the last (previous). It returns false, changing nothing, for a `Direction` of 0 or an inventory without a hotbar. Every slot of the hotbar is reached this way, the ones past the ninth included.
- **The HUD** stops following the farming component. The status line shows the item in hand's display name ("Day 3   18:40   |   Turnip Seeds"), or "Empty hand", rebuilt on the inventory's `OnItemInHandChanged`. The key hints name the items in the hotbar slots ("1 Hoe   2 Turnip Seeds   3 Watering Can"), empty slots left out, rebuilt on `OnInventoryChanged`, on `OnItemInHandChanged` and when the pawn changes: `AVestigePlayerCharacter::GetToolbarKeyHints()` becomes `GetHotbarKeyHints(const UVestigeInventoryComponent&)`, the mapped key of each slot's action with the item's display name. The "Bag:" line is unchanged. The HUD reads `GetItemInHand()` in its handler, never a passed value. The held mesh still follows the farming component's `OnToolChanged` (read with `GetCurrentTool()`), so seeds show the Seeds tool's mesh and the hand shows none.
- **Saving.** `FVestigeInventorySaveData` gains `SelectedHotbarSlot`; a saved slot outside the hotbar selects slot 0, with a warning. The field alone wouldn't need a new version, since 0 suits an older record ([section 1](#1-saving-and-loading)), but the layout goes to version 2 anyway: a version 1 bag holds no hoe or watering can items, so reading it would leave the player unable to till or water. The registry leaves an older part out (no upgrades before the first public build), so in a save from before this change the player gets the new starting items, with one warning in the log. Only development saves exist. The farming component saves nothing: it stops being `IVestigeSaveable`, and `FFarmFarmingSaveData` is deleted. Its key's record stays in older files, since the registry keeps and writes back keys it doesn't know, as mod data; section 1 lists `Base.Farming` as retired, never to be reused, since a new owner of that key would read the old bytes.
- **Layers.** `UVestigeToolFragment` sits in `Content/` with the seed fragment; `Inventory/` still knows nothing about tools; `Farming/` reads the inventory and the fragments, as it does today; the character (`Player/`) reaches the inventory. No new folder or dependency direction.
- **Performance.** No tick. Selecting a slot is a key press. After each inventory change the component compares one index and one pointer. Finding the tool in hand is a search over the item's few fragments and a map find, on a press or a hand change. The HUD rebuilds the hotbar hints on every inventory change, at most about three times a second while a held key plants or harvests, and the status line only when the item in hand changes. Loading happens once per pawn at play start; assets already in memory resolve at once.
- **Data.** A temporary editor commandlet, not committed (as for the `GE_` assets), makes `DA_Item_Hoe` and `DA_Item_WateringCan`, adds the tool fragment to `DA_Item_TurnipSeeds`, sets `BP_PlayerState`'s starting items and `BP_PlayerCharacter`'s `HandToolId`, and re-saves `BP_PlayerCharacter` without `ToolbarIds` and `SeedItemId`. [PlayTest.md](PlayTest.md) asks David to check the hotbar in play.

Not in this change (into [Roadmap.md](Roadmap.md)):

- Showing the hotbar as a row of slots with the selected one marked: 4.6's hotbar item, on the UMG HUD ([H7](CodeReview.md#h7--the-hud-is-raw-slate-written-in-c), done 2026-11-08) and following the hotbar boundary in [section 6](#6-user-interface).
- Moving items between slots (an inventory screen), and the mouse wheel or a gamepad button to step through the hotbar: 4.6, with the input assets. The stepping was built with H6 on 2026-11-07 (above, Input).
- A held mesh for items that aren't tools (a turnip in the hand).
- Harvesting while holding a tool or seeds: only the hand harvests, so the player selects an empty slot or a crop to harvest.
- Loading only the crops of the seeds the player carries, if crops become many.

Tests (`Source/Vestige/Tests/`):

- `Vestige.Targeting.*` keep their names and checks, on `UFarmTargetingComponent`; the highlight test finds it on the farmer.
- `Vestige.Inventory`: `TheFirstHotbarSlotIsInHandAtTheStart`, `SelectingAHotbarSlotPutsItsItemInHand`, `ASlotOutsideTheHotbarIsRefused`, `TheHotbarIsNeverLargerThanTheInventory`, `TheHandChangesWhenItsSlotEmptiesOrFillsButNotWhenItsCountDoes` (counts the event), `AHandlerThatSelectsAnotherSlotLeavesEveryListenerOnTheLatestHand` (one listener selects another slot from inside the event; a second listener, reading `GetItemInHand()` each time it is called, ends on the newly selected item, and the component broadcast once per change), `RemovingFromASlotTakesOnlyFromThatSlot` (all or nothing, other stacks of the item untouched), `TheSelectedSlotIsSaved` (a round trip, and a saved slot outside the hotbar selects slot 0), and `ThePlayerStartsWithTurnipSeeds` becomes `ThePlayerStartsWithAHoeSeedsAndAWateringCan` (in slots 0 to 2 of `BP_PlayerState`'s inventory, with slot 0 in hand).
- `Vestige.Item.AToolFragmentNeedsAKnownTool` (unset, another type and an unknown ID are invalid; `Base.Hoe` is valid), and `Vestige.Content.EverySeedItemNamesATool`.
- `Vestige.Farming`: the fixture possesses the farmer with a player controller, and its `Hold` puts an item in hotbar slot 0 with the bag's restore and selects it, so each tool's tests hold the tool's item, and the hand's hold an empty slot. `TheToolInHandGrantsItsAbility` becomes `TheToolOfTheItemInHandIsGranted` (one tool ability, with the tool asset as its source object, replaced when a slot with another tool is selected and kept when a slot with an item of the same tool is, without a press). New:
   - `AFarmerPossessedBeforeBeginPlayFollowsTheHand` (possessed, then begun play; selecting another tool's slot broadcasts `OnToolChanged` without a press).
   - `SelectingASlotOfAnotherToolBroadcastsOnToolChanged` (counts the event; none for a slot with the same tool).
   - `AnEmptyHandOrAnItemWithoutAToolHarvests`.
   - `PlantingUsesTheSeedInHand` (two seed items naming two crops, the second a transient crop given to the farming component with a test hook; each plants its own crop and removes one from its own slot, and a second stack of the same seed elsewhere is untouched).
   - `PlantingTheLastSeedLeavesTheHandEmpty` (one seed, one use: the use finishes and the ability ends, then exactly one tool ability is granted, the hand's), and its held-key variant `PlantingTheLastSeedOnARepeatLeavesTheHandEmpty` (two seeds and two tilled tiles; the second seed is planted by a repeat after the target changes, then exactly one tool ability, the hand's).
   - `ASecondHandChangeDuringTheFirstUseLeavesOneToolAbility` (one seed and the hoe in another slot; a listener on the farming component's `OnToolChanged` selects the hoe's slot when the tool in hand becomes the hand, so the hand's grant is already queued and the second change takes it back through `ClearAbility`; after the use ends exactly one tool ability is granted, the hoe's, and the farmer's held mesh is the hoe's). A listener on `OnInventoryChanged` wouldn't reach the case: it runs before the hand comparison, so the component would see one change, from the seeds straight to the hoe.

   `TheSeedItemPlantsTheCropItsFragmentNames`, `ANewPawnLeavesNoToolAbilityBehind` and the switching tests select slots instead of toolbar slots.
- `Vestige.PlayerCharacter.HotbarKeyHintsLeaveOutEmptySlots` replaces `ToolbarKeyHintsLeaveOutEmptySlots`.
- `Vestige.SaveRegistry.ThePlayersDataSurvivesReopeningTheMap` checks the selected slot and the tool it puts in hand instead of the saved tool.
- `FarmToolbarTests.cpp` is deleted with `FFarmToolbar`.

Docs in the same commit:

- This guide: section 1 (its status line's six owners and `Base.Farming`, "Registering pulls" and "Register once the owner can restore", which name the tool in hand and the toolbar, and `Base.Farming` listed as a retired key); section 5 (keys select hotbar slots, `AimAt` on the targeting component); this section's status, its toolbar, seeds and fragments bullets, and the pitfall about caching at `BeginPlay`, which gains "follow the player state, and bind at `BeginPlay` too, since the first pawn is possessed before it"; section 9's who-owns-what paragraph, the use-tool steps (the target from the targeting component) and the first "Not in this change" bullet of tools as abilities; section 20 (the loading screen waits on the tools and crops, not "the toolbar").
- [Architecture.md](Architecture.md): section 13's rows for the farming, targeting and inventory components, the tool fragment, the character, the highlight and `UVestigeSaveGame` (which names `FFarmFarmingSaveData`), with `FFarmToolbar` and `FFarmFarmingSaveData` removed; the `Farming/` row of section 14; a decision log row that supersedes the 2026-10-29 toolbar entry.
- [CodingGuidelines.md](CodingGuidelines.md): the folder listing for `Farming/` (the toolbar and its save struct go).
- [CodeReview.md](CodeReview.md) closes H2; [Roadmap.md](Roadmap.md) ticks the item, rewords 4.6's hotbar item and adds the items above.
- [PlayTest.md](PlayTest.md): the existing tool check that uses "the hand (4)" is rewritten for the hotbar, and new checks cover selecting slots, planting the last seed, two stacks of seeds, and whether the player can tell how to harvest now that empty slots have no hint.

**Pitfalls:**

- A field on `UVestigeItemData` that only some items use (a crop ID, a stamina value). It belongs in a fragment.
- Two fragments of the same class on one item: `FindFragment` would only ever see the first. Validation rejects it.
- One `UObject` per item in the inventory when a stack (ID and count) would do; instances only for items with their own state.
- Skip Lyra's replicated `FFastArraySerializer` list until multiplayer is real.
- Caching the inventory or the player state on the pawn at `BeginPlay`: a pawn spawned later begins play before it is possessed and has no player state yet. Ask the pawn for its player state when needed (an action), or follow the player state, and bind at `BeginPlay` too, since the first pawn of a map is possessed before it (the farming component's `FollowInventory`).
- Reading item definitions by loading them at the moment an item is added. Every definition is loaded at `BeginPlay`.

## 9. Abilities, attributes and effects (Gameplay Ability System)

**Status:** adopted 2026-10-29, starting with stamina (milestone 2, [Roadmap 4.3](Roadmap.md#43-stamina-and-late-nights)). David approved the stamina design below on 2026-10-29. Built: the setup, the stamina set, tool use as an ability with its cost and the slower uses at zero, the refill on sleep, the HUD line and saving (2026-11-01), the red HUD line at zero, and the late-night rules: the drain after midnight, exhaustion and the end of the pass-out at 02:00 (2026-11-02, agreed with David). Each tool's behaviour is its own ability, granted while the tool is in hand, and the stamina effects are `GE_` assets named by the game data (both 2026-11-07, below). Health and combat (milestone 3) build on the same setup.

**References:** Epic's "Gameplay Ability System" pages (ability system component, attribute sets, gameplay effects, gameplay abilities, ability tasks, gameplay cues); Lyra's setup: `ULyraAbilitySystemComponent` on the player state, `ULyraAbilitySet` data assets that grant abilities, effects and attribute sets together, attribute sets such as `ULyraHealthSet`, abilities with an activation policy, and gameplay cues for cosmetics.

**Rules:**

- **The ability system component lives on the player state** for the player (so attributes survive the pawn), and on the pawn for robots and creatures that don't have a player state. The pawn's avatar is set when it's possessed.
- **Attributes in attribute sets** (`UVestigeStaminaSet`, later `UVestigeHealthSet`), with clamping in `PreAttributeChange` and reactions in `PostGameplayEffectExecute`. Attributes change **only through gameplay effects**, never by setting the value from outside.
- **Gameplay effects as data**: costs (a tool's stamina cost becomes a cost effect), buffs, over-time drains (late-night drain), with magnitudes from data, not code.
- **Abilities granted from ability set data assets**, as Lyra does, so a tool, item or robot type grants its abilities by data.
- **Gameplay tags drive state**: blocking (`Vestige.State.Exhausted` blocks sprinting), cooldowns and activation rules, with native tags ([section 3](#3-gameplay-tags)).
- **Cosmetics through gameplay cues**, not inside abilities ([section 15](#15-visual-effects-and-feedback)).
- Single-player: no prediction keys or replication modes beyond the defaults; the ability system component uses minimal replication settings.
- Saving: save attribute base values and active long-lived effects by ID; restore them through effects or the set's restore function, not by writing attributes directly.

**Setup:** the Gameplay Abilities plugin is enabled in `Vestige.uproject`, and `GameplayAbilities` and `GameplayTasks` are private dependencies in `Vestige.Build.cs` (`GameplayTags` was already there). Our ability system code lives in its own layer, `Source/Vestige/AbilitySystem/`, right above `Core/`, so content (an item that grants abilities) can include it later; the folder isn't called `Abilities/` because the plugin's own headers are included as `Abilities/...`. Lyra calls `UAbilitySystemGlobals::Get().InitGlobalData()` from its asset manager; in Unreal Engine 5.8 that isn't needed, because the plugin calls it itself the first time the globals are asked for (`GameplayAbilitiesModule.cpp`). `DefaultGame.ini` names `/Game/Vestige/Abilities` as the folder for gameplay cue notifies, so the engine doesn't scan all of `/Game` for them.

**Native tags** are declared in `AbilitySystem/VestigeGameplayTags.h` (`VestigeGameplayTags::Ability_UseTool`, `State_UsingTool`, `Ability_Interact`, `Data_StaminaCost`, `Data_StaminaDrain`, `Data_StaminaRefill`, `State_OutOfStamina`, `State_Exhausted`), and so are the input tags (`Input_*`, [section 5](#5-input)), so code never builds a tag from a string. Tags only content uses stay in `DefaultGameplayTags.ini` ([section 3](#3-gameplay-tags)).

**First use: stamina.** The rules in [GameDesign.md, section 5](GameDesign.md#5-time-days-and-seasons) mapped onto the system:

| Rule | Built as |
|---|---|
| The player has stamina, shown on the HUD | Built. `UVestigeStaminaSet` with `Stamina` and `MaxStamina` (100 at the start, a named constant until characters get starting stats from data), on the ability system component of `AVestigePlayerState`. The HUD listens to both attributes' change delegates and shows "Stamina 85 / 100", in red while the ability system holds `Vestige.State.OutOfStamina`: the HUD follows the tag (`RegisterGameplayTagEvent`) rather than the number, so the set alone decides when stamina has run out, and it recolours the line only when the tag changes |
| Tool actions cost stamina | Built. Using a tool is a gameplay ability: the tool's own (`UFarmGameplayAbility_Till`...), on the shared base `UVestigeGameplayAbility_UseTool`. Its cost is `GE_StaminaCost`, named by the game data (`UVestigeGameData`), whose magnitude is the tool's `StaminaCost`, passed as a set-by-caller value tagged `Vestige.Data.StaminaCost`. A use that changes nothing on the farm (no target, nothing to water) costs nothing |
| Stamina never goes below zero; at zero, actions still work but take twice as long | Built. The set clamps `Stamina` to 0..`MaxStamina`. While it is 0, the set holds `Vestige.State.OutOfStamina` on the ability system as a loose tag, and tells the player once when it runs out. Each use takes the tool's `UseSeconds`, `OutOfStaminaTimeScale` (2) times as long while the tag is held; the ability stays active that long, so the next use can't start. Costs are still applied, and clamp at 0. The ability's `CheckCost` always passes, so a cost never blocks an action |
| Stamina drains faster after midnight | Built. There is no daytime drain, so this is a drain that starts at midnight: `GE_LateNightDrain`, an infinite periodic effect (one drain per real second, the effect's `Period`) whose amount per period is a set-by-caller value tagged `Vestige.Data.StaminaDrain`. The rest component applies it when the day cycle's `OnHourStarted` reaches its `LateNightHour` (24) and removes it just before the next day starts. It turns its `LateNightDrainPerGameHour` (10) into an amount per period with the clock's `RealSecondsPerGameHour`, so a faster clock doesn't change the drain per game hour |
| Going to bed late makes the player exhausted: next day at 75%, one level, cleared by a night in bed before midnight | Built. A night that ends at `LateNightHour` or later (sleeping then, or staying up until the day ends at 06:00) applies `GE_Exhausted`, an infinite effect with no modifiers that grants `Vestige.State.Exhausted` through a target tags component, unless the player is already exhausted. The refill is then `ExhaustedRefillFraction` (0.75) of `MaxStamina`. A night that ends earlier refills fully and removes the effect. The HUD shows "Exhausted" after the stamina while the tag is held |
| Sleep refills stamina | Built. `GE_StaminaRefill`, an instant effect that overrides `Stamina` with a set-by-caller amount tagged `Vestige.Data.StaminaRefill`: the rest component sets it to `MaxStamina` times the night's refill share and applies it just before it starts the next day, so it is saved state changed synchronously ([Architecture.md, section 15](Architecture.md#15-the-day-flow)). The set clamps the result |

Who owns what: `AVestigePlayerState` owns the ability system component and grants its ability sets in `PostInitializeComponents`, so the stamina set exists before any actor's `BeginPlay`; it makes the pawn the avatar when `OnPawnSet` fires, and cancels running abilities when the pawn changes. The pawn (`AVestigePlayerCharacter`) implements `IAbilitySystemInterface` and returns the player state's component, so lower layers find it with `UAbilitySystemGlobals::GetAbilitySystemComponentFromActor` without knowing the player state class. `UVestigeStaminaSet` owns the numbers, their limits and the out-of-stamina tag. The farming component finds the tool of the item in hand (the inventory's hotbar, [section 8](#8-inventory-and-items)) and grants its ability, and grants the harvest all the time ([section 7](#7-interaction), harvesting with F), and `UFarmTargetingComponent` keeps the target tile; the tool abilities own what a use does, what it costs and how long it takes (tools as abilities, below). `UVestigeRestComponent` on the pawn owns sleep, the late-night rules (the warnings at 22:00 and midnight, the drain), exhaustion and the refill, and its settings hold their numbers; the effects it applies live on the player state's ability system, so it removes the drain in its `EndPlay`. Abilities and the stamina set are granted from a `UVestigeAbilitySet` data asset (`Content/Vestige/Abilities/DA_AbilitySet_Player`, named on `BP_PlayerState`): a plain list of attribute set classes and ability classes, without Lyra's grant handles until something needs to take a set away. Saving stores the base value of `Stamina` (`FVestigeStaminaSaveData`) and restores it through the set's restore function, which sets the base value without replaying effects. Whether the player is exhausted is saved by the rest component (`FVestigeRestSaveData`), which applies the effect again on load. The drain needs no saving: saves are made at the start of a day, before midnight. Tests cover the clamp, the cost, a use that costs nothing, one use at a time, the zero-stamina tag and slowdown, the refill on sleep and the save, and the late-night rules: the drain starting at midnight with its rate per game hour and stopping on the new day, the warnings, exhaustion after a late night or a night awake, its one level, an early night clearing it, and exhaustion in the save.

Built differently from the approved design, decided on 2026-11-01 (decision log):

- **Effects are C++ classes, not `GE_` Blueprint assets.** Superseded on 2026-11-07: they are `GE_` assets now (effects as `GE_` assets, below). Their shape is fixed (which attribute, which operation) and every magnitude comes from data: the cost from the tool's `StaminaCost`, the refill from `MaxStamina`. In C++ they show in code review and tests, and the agents that build this project can't author gameplay effect modifiers with editor scripting. An effect with a number of its own to tune (the late-night drain rate, the 75% exhausted refill) gets that number from data too: a set-by-caller value from a settings or data asset, or a Blueprint subclass of a C++ effect.
- **The out-of-stamina tag is a loose tag the set keeps in step with `Stamina`, not an infinite effect.** It is derived from the number, so it can't drift from it, needs nothing in the save and no effect asset.
- **The refill's magnitude is an amount set by the caller** (2026-11-02), not `MaxStamina` read by the effect: an attribute-based magnitude's coefficient can't come from the caller, so a refill to 75% would need a second effect or a custom calculation class. The rest component multiplies `MaxStamina` by the night's share.
- **A tool use takes time.** "Twice as long" needs a duration, and there are no animations yet, so the tool's `UseSeconds` is how long a use lasts. Until 2026-11-07 the wait was a timer on the ability instance, to avoid an ability task object per use; since tools became abilities it is a `WaitDelay` task, as [CodeReview.md, L3](CodeReview.md#low) asked, and [CodingGuidelines.md, section 12.3](CodingGuidelines.md#123-memory-and-allocation) allows it. With animations, the duration becomes the montage's length and the play rate halves at zero stamina.

**Tools as abilities** (design for [CodeReview.md, H1](CodeReview.md#h1--the-gameplay-ability-system-gates-tools-but-doesnt-run-them), 2026-11-07, approved after two rounds of review by a senior Unreal C++ reviewer agent, whose approval David accepted; built the same day as described). Today one use-tool ability wraps a switch on `EFarmToolAction` in the farming component, and the two call each other. After this change, a tool's behaviour is its own ability class, named by the tool's data asset, so a new tool is a new ability and a data asset, with no change to the farming component.

References: Epic's "Gameplay Abilities" and "Ability Tasks" pages (`UAbilityTask_WaitDelay`, `UAbilityTask_WaitMovementModeChange`), the ability's input hooks (`UAbilitySystemComponent::AbilitySpecInputPressed` / `AbilitySpecInputReleased`, `UGameplayAbility::InputReleased`, `FGameplayAbilitySpec::InputPressed`), and the spec's source object (`FGameplayAbilitySpec::SourceObject`, `UGameplayAbility::GetCurrentSourceObject`). Lyra: `ULyraEquipmentInstance` grants its equipment's ability sets when it is equipped and takes them away when it is unequipped, and its weapon abilities find their weapon through the spec's source object. Engine behaviour below was checked in the 5.8 source.

| Part | Kind | Owns |
| --- | --- | --- |
| `UVestigeToolData` | Data asset (`Content/`) | Adds `Ability` (`TSubclassOf<UGameplayAbility>`): the ability that is the tool's behaviour, checked in `IsDataValid` (set, not abstract). Keeps `StaminaCost`, `UseSeconds` and `HeldMesh`. `Action` and `EFarmToolAction` are deleted. `Content/` sits below `Farming/`, so it names the engine's base class; a test checks every tool names a `UVestigeGameplayAbility_UseTool` |
| `UVestigeGameplayAbility_UseTool` | Abstract ability (`Farming/`) | What every tool use shares: one press, one activation; a use, its cost and its time; repeating while the key is held; no use in the air. Subclasses say what the tool does through two virtual functions (below). Not granted by `DA_AbilitySet_Player` any more |
| `UFarmGameplayAbility_Till`, `_Plant`, `_Water`, `_Harvest` | Abilities (`Farming/`) | One tool behaviour each: the rules that `UFarmingComponent::ApplyTool` and `WouldToolChange` switch on today, with the same messages. `Farm` prefix, as farming actions ([CodingGuidelines.md, section 2.2](CodingGuidelines.md#22-the-vestige-and-farm-project-prefixes)) |
| `UFarmingComponent` | Component on the pawn | Keeps the toolbar, the tool in hand, targeting, the seed item, the held flag (for the walk speed), the save and the extension points. Gains: granting the ability of the tool in hand and passing the use key's press and release to it. Loses: the tool rules, `UseToolOnTarget`, `WouldToolChange`, the repeat and its listeners (`OnAbilityEnded`, `LandedDelegate`). It includes the engine's ability system headers to grant, but none of our abilities |

How it works:

- **Granted while in hand.** The farming component keeps the spec handle it granted and the ability system it granted it on (a weak pointer), and one function, `UpdateGrantedTool`, makes them match the tool in hand and the ability system the pawn reaches now (the player state's): it takes back the old grant through the stored pointer and gives the new tool's ability with the tool asset as the spec's source object. It runs when the tool in hand changes, on the use key's press, and when the pawn's controller changes (`APawn::ReceiveControllerChangedDelegate`, which fires after the pawn's player state is set or cleared), and `EndPlay` takes the grant back through the stored pointer. The pointer matters: by the time a pawn's components end play the pawn has already been unpossessed and its player state cleared, so the pawn no longer reaches the ability system, and without it each new pawn would leave a grant behind. An ability in the middle of a use when its tool leaves the hand is first told the key is released and then marked to be removed when it ends (`SetRemoveAbilityOnEnd`, which removes at once an ability that isn't active), so switching tools can't cut a use's time short; the new tool's ability is blocked until it ends (below). Granting costs one ability object per tool switch (instanced per actor), made on a key press, never per frame, plus two log lines at the ability system's Log level.
- **One press, one activation.** The use key's `Started` calls `StartUsingTool`, which marks the key held for the walk speed, grants if needed, sets the spec's input as pressed (`AbilitySpecInputPressed`) and tries to activate the spec by its handle. `Completed` calls `StopUsingTool`, which sets it released (`AbilitySpecInputReleased`); the engine passes that to the active ability's `InputReleased`. **Whether the key is held is read from the spec's `InputPressed` at every decision the ability makes**, never from a flag of its own or the component's: a press while a use is finishing can't start a second activation, but it does set `InputPressed`, so the repeat goes on. `UseTool` (one use, for tests and console commands) activates without pressing. The farming component no longer activates by tag.
- **The loop inside the ability.** On activation, and each time a use finishes, the base ability tries the next use:
   1. In the air: if the key is held, wait for the farmer to walk again (`WaitMovementModeChange` to walking), then try again; otherwise end without a use.
   2. Read the target tile from the targeting component (`UFarmTargetingComponent`, since H2). No target: on the press only, say "Stand facing a farm tile."; then wait for the target to change if the key is held, or end.
   3. The first try after the press calls `UseOn` (which says why when nothing changes, as today); a try after a landing counts as a repeat. A repeat first asks `CanUseOn` and, when it would change nothing, waits quietly for the target to change.
   4. A use that changed the farm commits the tool's cost (`CommitAbilityCost`), reports `On Tool Used` through the farming component, and waits the use's time with `WaitDelay`: the tool's `UseSeconds`, `OutOfStaminaTimeScale` (2) times as long while the player is out of stamina, read before the cost so the use that empties stamina runs at full speed. Then step 1 again if the key is held, or end.
   5. A use that changed nothing ends the ability if the key isn't held, or waits for the target to change.

   The ability keeps which of these it is doing (using, waiting out the use, waiting for the target, waiting for a landing) and sets it before calling out. `UseOn` and the reports run other code (the field's change event, Blueprint handlers of `On Tool Used`) that can move the farmer, switch tools or end the ability, so after each call out it stops if it is no longer active, and a target change that arrives while it isn't waiting for one is ignored (the next try reads the target anyway). It never keeps a pointer to its spec across a call that can end it: with remove-on-end set, ending removes the spec. Waiting for the target means listening to the farming component's `OnTargetTileChanged`, through a weak pointer, removed in `EndAbility` before the base class's end. Releasing the key ends a waiting ability at once, and lets a use under way finish its time first. While the ability is active it owns `Vestige.State.UsingTool` (`ActivationOwnedTags`), and every tool ability is blocked by it (`ActivationBlockedTags`), which keeps one use at a time across tools. The tag means "a tool ability is active", which includes waiting idly with the key held, not "a use is under way", so animation must not key off it.
- **What a subclass writes.** `bool CanUseOn(const FIntPoint& Tile) const` (whether a use would change the tile now, changing nothing) and `bool UseOn(const FIntPoint& Tile)` (do it, say why when it can't, return whether the farm changed). The base gives them the field (`GetField`), the tool in use (`GetTool`, from the spec's source object), the inventory on the ability system's owner, the player state (`GetInventory`), and the farming component (`GetFarming`) for the seed item. The tile work goes through the field's rule functions (`UFarmField::Till`, `Plant`, `Water`, `Harvest` and `GetTile`), which already are the interface between farming actions and the tiles, so no new interface is added on the grid. When a tool targets something other than a tile (a placed sprinkler, [section 16](#16-placing-objects-and-building)), that target kind gets an interface then.
- **Extension points stay on the farming component**, so Blueprints bind them as before: the harvest ability calls `UFarmingComponent::ReportCropHarvested` and the base ability `ReportToolUsed`, which broadcast `On Crop Harvested` and then `On Tool Used`. The ability calls the component; the component never calls our abilities, which ends the circle H1 found.
- **Tasks instead of the timer.** The base uses `WaitDelay` and `WaitMovementModeChange` instead of an `FTimerManager` timer and the pawn's `LandedDelegate`, as [CodeReview.md, L3](CodeReview.md#low) asks: standard tasks are what other Unreal developers expect to read, and montage tasks replace `WaitDelay` when tools get animations. It costs one small task object per use, at most about three a second while the key is held, made in the last task's callback. That is an exception to [CodingGuidelines.md, section 12.3](CodingGuidelines.md#12-performance) (no new objects per action), recorded there: ability tasks are allowed per action, never per frame. A task that fires after its ability ended does nothing (it is marked garbage when the ability ends it).
- **Data.** The four tool assets name their ability (`DA_Tool_Hoe` Till, `DA_Tool_Seeds` Plant, `DA_Tool_WateringCan` Water, `DA_Tool_Hand` Harvest), and `DA_AbilitySet_Player` stops granting the use-tool ability. A headless editor script sets them. The save doesn't change: it stores the tool in hand by `ContentId`. `Ability` becomes a soft class reference in the tool's bundle once tool abilities can be Blueprints.

Not in this change:

- Planting still used the farming component's one `SeedItemId`, read by the plant ability. Done with [H2](CodeReview.md#h2--ufarmingcomponent-does-seven-jobs-and-can-plant-only-one-seed) (2026-11-07): the plant ability plants the seed item in hand ([section 8](#8-inventory-and-items), the hotbar).
- Abilities still write the player's text ([M4](CodeReview.md#m4--gameplay-code-writes-the-players-text)); the cost was the C++ cost effect until the `GE_` assets, below.
- **H1's modding half.** Mods are Blueprints and content ([Architecture.md, section 12](Architecture.md#12-designing-for-mods)), and `CanUseOn` and `UseOn` are C++, so this change lets a C++ ability add a tool, not a mod. A Blueprint tool needs both functions Blueprint-implementable and the field's rules callable from Blueprints; that is a roadmap item for when mod support is designed ([GameDesign.md, section 2](GameDesign.md#2-pillars): extension points only for a concrete use).
- Input tags ([section 5](#5-input), [H6](CodeReview.md#h6--input-is-built-in-c-with-hard-coded-keys)): the press goes to the granted spec by handle. Decided with the input config on 2026-11-07: the use key binds natively to `UFarmingComponent::StartUsingTool`, which runs owner logic (following the inventory, granting the tool in hand, the held key for the walk speed) before it presses the spec, so it stays a native binding; abilities activate by input tag only once a key maps straight to an ability with no owner logic in between.
- Holding E while switching tools stops the repeat; the new tool starts on the next press. Today the repeat carries on with the new tool.
- A landing in a movement mode other than walking (swimming, nav walking) doesn't resume a held use; releasing the key still ends it. Revisit with water or navigation.

Tests (`Source/Vestige/Tests/`): the existing farming tests keep their names and checks (each tool's rules, costs, one use at a time, the slowdown at zero stamina, holding and releasing the key, the air, the extension points). The test world isn't ticked by the engine, and ticking it would run the farmer's movement (he would fall, as the test level has no floor), so the tests that hold the key run as latent steps (`TestCommandBuilder`) that advance only the world's timer manager, once per step, until the use's time has passed. Single-use tests still end the use directly. "A use is under way" is read from `Vestige.State.UsingTool`. New: `Vestige.Farming.TheToolInHandGrantsItsAbility` (one tool ability granted, with the tool as its source object, replaced on a switch), `Vestige.Farming.ANewPawnLeavesNoToolAbilityBehind` (a player controller possesses the farmer and then a second farmer, so the controller-changed path runs, and exactly one tool ability stays granted), `Vestige.Farming.SwitchingToolsDuringAUseWaitsForTheUseToFinish`, `Vestige.Farming.APressDuringTheLastUsesTimeKeepsTheRepeat`, `Vestige.Farming.ANewToolNeedsOnlyItsAbility` (a test ability on a transient tool asset works the tile with no farming component change), and `Vestige.Content.EveryToolNamesAToolAbility`.

**Effects as `GE_` assets** (design for the second half of [CodeReview.md, H1](CodeReview.md#h1--the-gameplay-ability-system-gates-tools-but-doesnt-run-them), 2026-11-07, approved after two rounds of review by a senior Unreal C++ reviewer agent, whose approval counts as David's). The four stamina effects are C++ classes today (`UVestigeStaminaCostEffect`, `UVestigeStaminaRefillEffect`, `UVestigeStaminaDrainEffect`, `UVestigeExhaustedEffect`), so a designer or a modder can't change what they do: give the drain another period, make exhaustion also lower the maximum, or add a gameplay cue. After this change they are gameplay effect Blueprints, as Epic's documentation and Lyra make them, and code finds them through one global data asset. The numbers that are rules stay where they are: the tool's `StaminaCost`, the rest component's drain per game hour and refill share, `MaxStamina` in the set.

References: Epic's "Gameplay Effects" page (effects are data: a Blueprint class whose parent is `UGameplayEffect`, edited in its Class Defaults; set-by-caller magnitudes; gameplay effect components such as the target tags component), and Lyra's `ULyraGameData` (one `UPrimaryDataAsset` of global data, `DefaultGameData`, that names the set-by-caller damage and heal effects) loaded once by `ULyraAssetManager` from a config path (`LyraGameDataPath`) at startup, with an always-cooked primary asset type so the cook includes it. Engine behaviour below was checked in the 5.8 source.

| Part | Kind | Owns |
| --- | --- | --- |
| `GE_StaminaCost`, `GE_StaminaRefill`, `GE_LateNightDrain`, `GE_Exhausted` | Gameplay effect Blueprints, parent `UGameplayEffect` (`Content/Vestige/Abilities/`) | The shapes the C++ classes have today, unchanged: cost is instant, adds `Vestige.Data.StaminaCost` to `Stamina`; refill is instant, overrides `Stamina` with `Vestige.Data.StaminaRefill`; the late-night drain is infinite with a period of 1 s, not executed on application, adds `Vestige.Data.StaminaDrain` each period; exhausted is infinite with no modifiers and a target tags component granting `Vestige.State.Exhausted` |
| `UVestigeGameData` | `UPrimaryDataAsset` (`Content/`), one asset: `Content/Vestige/DA_GameData` | Global data game code needs that isn't one piece of content. For now the four effects game code applies, as `TSubclassOf<UGameplayEffect>` (`StaminaCostEffect`, `StaminaRefillEffect`, `LateNightDrainEffect`, `ExhaustedEffect`). `Get()` returns the loaded asset. `IsDataValid` reports an unset effect and a drain that isn't infinite with a period above 0 |
| `UVestigeAssetManager` | Asset manager (`Content/`), now `Config = Game` | Gains `GameDataPath` (a `TSoftObjectPtr<UVestigeGameData>` set in `DefaultGame.ini`) and loading it once in `StartInitialLoading`, after the base class, keeping it in a `Transient` `UPROPERTY` so it stays loaded. `GetGameData()` |
| `UVestigeRestComponent`, `UVestigeGameplayAbility_UseTool` | (`Farming/`) | Apply the game data's effects instead of the C++ classes, with the same set-by-caller values as today |

How it works:

- **Loaded at startup, never during play.** The game data holds hard class references, so loading it loads the four effects with it, a few kilobytes that every game needs. Lyra holds soft references and resolves them with `UAssetManager::GetSubclass`, which loads synchronously on first use, during play; hard references avoid that ([CodingGuidelines.md, section 12](CodingGuidelines.md#12-performance)). `StartInitialLoading` runs once per process (from `UEngine::InitializeObjectReferences`), before any world, in the game, the editor and commandlets, so tests and Play in Editor find it loaded; native gameplay tags are registered by then, and the engine loads its own Blueprint game singleton at the same point. The one exception is `-SkipAssetScan`, which skips it: the game then runs without the effects, as below. The load is synchronous and happens once, at startup.
- **Missing data doesn't crash.** If `GameDataPath` doesn't load, the asset manager logs an error (`LogVestigeContent`) and `Get()` returns the class default object, whose effects are all unset; Lyra stops the game instead, but a broken mod or a half-made asset shouldn't. `Get()` is built on `UAssetManager::GetIfInitialized()` and a `Cast` (never `UAssetManager::Get()`, which stops the engine when there is no asset manager), falling back to the class default object, so it also works with another asset manager class or none. The cost, the refill and the drain check the spec they make (`MakeOutgoingSpec` returns an invalid handle for a null class); exhaustion makes no spec, so the rest component checks the class before applying it and takes its class default object with `TSubclassOf::GetDefaultObject`, never `GetDefault<UGameplayEffect>(Class)`, which crashes on null. With an effect unset nothing changes stamina and nothing logs every night: uses cost nothing, the night doesn't drain or exhaust, sleeping doesn't refill. `Get()` is a cast and a pointer read, cheap enough per tool use.
- **Cooked.** `DefaultGame.ini` gets a primary asset type `VestigeGameData` (base class `UVestigeGameData`, the one specific asset `/Game/Vestige/DA_GameData`, `CookRule=AlwaysCook`), as Lyra registers `LyraGameData`: a path in config alone isn't found by the cook. The asset is a primary asset by its class name, which `UPrimaryDataAsset` gives it.
- **The cost.** A C++ ability can't name a Blueprint effect in its constructor without a hard-coded path, and its class default object is made before the game data loads, so `CostGameplayEffectClass` stays unset. `ApplyCost` makes its spec from the game data's `StaminaCostEffect` (with `MakeOutgoingGameplayEffectSpec`, as today), and `GetCostGameplayEffect` is overridden to return that effect's class default object, so engine code and debug tools that ask an ability for its cost see it. `CheckCost` still always passes.
- **The drain's period comes from the effect.** The rest component reads the period from the spec it made (`FGameplayEffectSpec::GetPeriod`, which is the effect's `Period` at the spec's level) instead of the `PeriodSeconds` constant, so the amount per period keeps the drain at `LateNightDrainPerGameHour` whatever period the asset has. The drain must be infinite and periodic: the rest component removes it itself when the day ends, and a drain with a duration would expire on its own while the component still holds its handle and believes it runs, so it would never be applied again that night. A drain that isn't infinite (`Spec.Def->DurationPolicy`) or has no period (`GetPeriod()` not above `UGameplayEffect::NO_PERIOD`) isn't applied, and the rest component logs an error. `IsDataValid` checks the same in the editor first. [M7](CodeReview.md#m7--game-time-events-run-on-real-time) later replaces the real-time period with the clock's events.
- **Exhaustion** is applied with the game data's `ExhaustedEffect` (`ApplyGameplayEffectToSelf` with its class default object, after checking the class is set) and removed with `RemoveActiveGameplayEffectBySourceEffect` on the same class, so the rest component, its save restore and `StartNewGame` all name one class.
- **The tags are the contract.** `Vestige.Data.StaminaCost`, `StaminaRefill` and `StaminaDrain` stay native tags: code sets them on every spec, and an effect asset reads them or not. A designer who changes an effect's magnitude to something else (a scaled attribute, a curve) just stops reading the tag: setting a value an effect doesn't read is silent. The other way round is an error: an effect that reads a set-by-caller tag nobody set logs one (`LogGameplayEffects`) and uses 0, so a new tag needs the code that sets it.
- **Saves don't change.** They store the base value of `Stamina` and whether the player is exhausted, never effects, so no version changes. No asset or save names the C++ effect classes, so they are deleted without redirects.
- **Making the assets.** The five assets are made by a temporary editor commandlet run headless, which is not committed (the assets are the result; the modifier's attribute is a field path, which editor Python can't set). [PlayTest.md](PlayTest.md) asks David to open each in the editor once and check what it shows.

Docs in the same commit: a decision log row that reverses choice (1) of the 2026-11-01 stamina entry (designers and modders can tune effects only as assets, and a headless commandlet removes the tooling reason), with the 2026-11-02 drain entry pointing to it; [Architecture.md](Architecture.md) section 13 replaces the C++ effects' row with `UVestigeGameData` and the `GE_` assets and renames them in the rest component's and the use-tool ability's rows, and section 14's diagram gains the rest component and the use-tool ability depending on the game data; in this guide, the stamina table names the assets, the "Effects are C++ classes" bullet is marked superseded, and section 2's status says the global game data is built; [CodeReview.md](CodeReview.md) closes H1; the roadmap item is ticked.

Not in this change: per-tool cost effects (a tool whose use costs something other than stamina gets a `CostEffect` on its data asset when one exists), starting stamina from data (its own roadmap item, which may become a starting-values effect in the game data), and gameplay cues.

Tests: the stamina, rest, farming and save tests that apply effects get them from `UVestigeGameData::Get()` instead of the C++ classes, so they now check the real assets: the cost and its clamp at 0, the refill and its clamp at the maximum, the drain's rate per game hour (the test reads the period from the active effect's spec instead of `PeriodSeconds`, and checks stamina hasn't changed right after the drain starts, so the asset still drains one period after midnight, not at once) and its end on the new day, exhaustion's tag, its one level, its removal and its save. New: `Vestige.GameData.NamesEveryEffect` (the loaded game data is the asset at `GameDataPath`, every effect is set and it passes `IsDataValid`) and `Vestige.GameData.ADrainThatIsntInfiniteAndPeriodicIsInvalid` (a transient game data whose drain is the instant `GE_StaminaCost`, or a transient effect with a duration and a period, fails `IsDataValid`, and one without effects reports each).

**Pitfalls:**

- Building more of the system than stamina needs. Start with the stamina set, the tool abilities, the effects above and one ability set; add Lyra's extras (custom ability system component subclasses, activation groups, ability cost classes) only when a second use needs them.
- Abilities that tick or poll. Use ability tasks, timers and attribute change delegates.
- Setting an attribute from outside its set. The set exposes only getters; its `Init` helpers are private and only its constructor uses them. Loading goes through the set's restore function.
- Instancing policy: use instanced-per-actor abilities (non-instanced abilities are deprecated in recent engine versions); avoid instanced-per-execution, which creates an object on every use.

## 10. Robots: AI, routines and crowds

**Status:** planned. Hob, the caretaker robot, is milestone 2; the town is milestone 3.

**References:** Epic's "StateTree", "Behavior Trees", "Smart Objects", "AI Perception", "Mass Entity" and "Significance Manager" pages. Lyra's bots use behavior trees with an `AAIController`, but StateTree is Epic's newer general-purpose state machine, built for Unreal Engine 5, and the one we choose for new AI.

**Rules:**

- **AI controller plus StateTree** for robot behavior. The robot pawn is a body with components; the controller and its StateTree decide what to do. Log the choice of StateTree over behavior trees when the first robot is built.
- **Daily routines are data.** A robot's schedule (where it is at what hour, by season and weather) lives in a data asset, not in the tree. The tree runs "go to the next activity" generically.
- **Smart Objects for places robots use** (a bench, a pump to repair, a shop counter): the place says what can be done there, and robots claim slots. This suits robots repeating pre-war routines and lets mods add places without touching AI code.
- **Robot definitions are content** (`UVestigeCharacterData`, a `UVestigeContentData` subclass) with the ID, name, schedule, dialogue and relationship data.
- **Scale by significance.** Robots far away or out of sight update less often (tick interval, perception off, animation update rate optimization). When the town has many simple background robots, consider Mass for them and keep full actors for the named characters.
- Perception (sight, hearing) only for robots that need it, and not every frame.

**Pitfalls:**

- Behavior logic in the robot's Blueprint event graph or `Tick`.
- Pathfinding on every frame: move with `MoveTo` requests and react to completion events.

## 11. Dialogue and relationships

**Status:** planned for Hob in milestone 2.

**References:** no built-in Epic dialogue system. The design is ours: data assets, gameplay tags for conditions, and `FText` for all lines.

**Rules:**

- **Dialogue is data**: data assets (or data tables imported from a spreadsheet) of lines with speaker, `FText`, conditions and outcomes. Conditions and outcomes are small, named, data-selected types (instanced `UObject` fragments or tags), not code per conversation.
- **Relationship state** (points, flags, seen lines) lives in one owner, a relationship component on the player state or a world subsystem, keyed by character `ContentId`. Robots don't store how the player feels about them.
- Dialogue UI is presentation: it shows lines it is given and reports choices.
- Saves store character IDs, points, flags and seen line IDs.
- Every line is localizable `FText` with a stable key ([section 18](#18-localization)).

**Pitfalls:**

- Writing lines in C++ or Blueprint string literals.
- Branching dialogue as a big Blueprint graph per character; it can't be reviewed, localized or modded.

## 12. Camera

**Status:** the template's spring arm and follow camera on `AVestigeCharacter`.

**References:** Epic's "Cameras" and "Camera Shakes" pages; Lyra's `ULyraCameraComponent` with a stack of `ULyraCameraMode` objects that blend (third person, aiming, death).

**Rules:**

- Keep the spring arm camera until a second camera behavior is needed (a dialogue close-up, an interior, a building mode). Then adopt Lyra's idea: camera modes as small classes on a stack, blended by the camera component, chosen by whoever needs them (dialogue pushes a dialogue mode).
- The camera only presents; it never changes game state.

**Pitfalls:**

- Several systems setting the camera's transform directly. Everything goes through one camera owner.

## 13. Animation

**Status:** the template's mannequin and animation Blueprint, with the held tool attached to `hand_r`.

**References:** Epic's "Animation Blueprints", "Linked Animation Layers", "Property Access" and "Animation Optimization" pages; Lyra's animation Blueprint, which uses linked animation layers per held item and thread-safe update functions.

**Rules:**

- **No game logic in animation Blueprints.** They read state (speed, the tool in hand, exhausted or not) and play animation.
- **Thread-safe animation updates.** Read gameplay state through property access in `BlueprintThreadSafeUpdateAnimation` so the update can run off the game thread; avoid the event graph's per-frame logic.
- **Linked animation layers per tool type**, as Lyra does per weapon, so a new tool animation set doesn't touch the main graph.
- Gameplay timing that matters (when a hoe hits the ground) comes from the game, not from an animation notify, so rules still work without animation and in tests. Notifies trigger cosmetics only (sounds, dust).
- Robots far away use animation update rate optimization; crowds use the Animation Budget Allocator plugin if needed.

**Pitfalls:**

- Casting to the character every frame in the animation Blueprint; cache it in `NativeInitializeAnimation`.

## 14. Audio

**Status:** none yet.

**References:** Epic's "MetaSounds", "Sound Classes", "Sound Mixes", "Audio Modulation" (control buses), "Sound Concurrency" and "Submixes" pages; Lyra's audio setup: control buses and bus mixes for the volume settings, and context effects for surface-dependent footsteps.

**Rules:**

- **Sounds react to events** ([Architecture.md, section 7](Architecture.md#7-presentation-ui-audio-and-effects)): game code broadcasts (tile watered, crop harvested); audio code listens and plays. With the Gameplay Ability System, gameplay cues play them.
- **MetaSounds** for anything with variation or logic (random pitch, layered water splash); plain sound waves for one-shots.
- **Sound classes and a control bus per volume slider** (master, music, effects, ambience, voice), set from the settings, as Lyra does.
- **Concurrency limits** on frequent sounds (footsteps, tool hits), and attenuation on every world sound.
- Music and ambience that continue across maps belong to a game-instance subsystem.
- Audio assets are soft references in content data (an `Audio` bundle).

**Pitfalls:**

- `PlaySound2D` scattered through gameplay code.
- Hundreds of looping ambient sounds placed in the level without concurrency or virtualization settings.

## 15. Visual effects and feedback

**Status:** none yet beyond the HUD messages.

**References:** Epic's "Niagara" pages, including scalability and effect types; Lyra's gameplay cues and `ULyraContextEffectComponent` (effects chosen by surface type and context tags).

**Rules:**

- **Effects react to events**, like audio. Game code never spawns a particle system.
- **Niagara systems use effect types** with scalability settings and culling distances, so effects shrink or disappear at lower scalability and distance.
- Short, frequent effects are pooled (Niagara's pooling method on the spawn call).
- Effects are soft references in content data and load with the content that uses them.
- Player feedback messages go through a presentation-neutral feedback event, `UVestigeFeedbackSubsystem` ([Architecture.md, section 13](Architecture.md#13-systems-and-ownership)).

**Pitfalls:**

- Large translucent particles filling the screen (smoke, haze): use fog and post-processing for the haze ([Architecture.md, section 10](Architecture.md#10-performance-by-design)).

## 16. Placing objects and building

**Status:** planned (sprinklers and the shipping bin in milestone 2, building and crafting later).

**References:** Epic's "Instanced Static Mesh Component" and "Hierarchical Instanced Static Mesh" pages for many placed copies; the farm grid in [Architecture.md, section 13](Architecture.md#13-systems-and-ownership).

**Rules:**

- Placeable objects are content: a placeable item fragment ([section 8](#8-inventory-and-items)) names the actor class or mesh to place, its footprint in tiles and its rules.
- **One owner of what is placed where**: the grid (or a placement subsystem if placement extends beyond the farm grid) records occupancy by grid coordinates, checks placement and saves placed objects by ID and coordinates.
- Placement preview is presentation: a ghost mesh that reads the placement check, never a real spawned object until confirmed.
- Objects with behavior (a sprinkler) are actors that listen to `OnNewDay`; purely decorative objects (fences, paths) are instances in an instanced static mesh component, not actors.

**Pitfalls:**

- Spawning and destroying actors every frame for the preview.

## 17. Settings

**Status:** none yet beyond the engine defaults.

**References:** Epic's `UGameUserSettings` (resolution, scalability, frame limit) and Enhanced Input's player-mappable key settings; Lyra's split between `ULyraSettingsLocal` (machine-specific, a `UGameUserSettings` subclass saved in config) and `ULyraSettingsShared` (player preferences, a `USaveGame` that would follow the player between machines).

**Rules:**

- Follow Lyra's split: `UVestigeSettingsLocal` (a `UGameUserSettings` subclass, set in `DefaultEngine.ini` as the game user settings class) for graphics, scalability, resolution and audio device; `UVestigeSettingsShared` (a `USaveGame`) for preferences like subtitles, survival needs on or off ([GameDesign.md](GameDesign.md)) and volume.
- **Key bindings are saved by Enhanced Input's own user settings** (`UEnhancedInputUserSettings`, a save game of its own), not in `UVestigeSettingsShared` (changed with [H6](CodeReview.md#h6--input-is-built-in-c-with-hard-coded-keys), 2026-11-07; decision log): it is the engine's tested path for key profiles and slots, and remaps keep working without our code. When this section is built:
   - `bEnableUserSettings=True` goes in `DefaultInput.ini` (under `[/Script/EnhancedInput.EnhancedInputDeveloperSettings]`); the pawn already registers its contexts with the user settings when it adds them ([section 5](#5-input)).
   - Its save slot is `EnhancedInputUserSettings` in 5.8 (`InputSettingsSaveSlotName` in the same settings). It must never clash with the farm's `Farm` and `Farm2` slots, and New game or deleting saves never removes it.
   - Our settings screen calls `SaveSettings()` after each remap.
   - The player-mappable names (the input tags, `Vestige.Input.UseTool`) are the keys remaps are saved under, so they are frozen once a player can save a remap.
- Settings apply through the owner of each system (audio volumes through control buses, scalability through `UGameUserSettings`), not by each system reading the settings file.
- Settings are saved separately from the farm save, so a new game keeps them.
- The first run picks scalability with `RunHardwareBenchmark`, and the player can change it.

**Pitfalls:**

- Storing gameplay difficulty options in config files that the player can't reach from the menu.

## 18. Localization

**Status:** not started; player text is `FString` today ([CodingGuidelinesAudit C-10](CodingGuidelinesAudit.md)).

**References:** Epic's "Localization" pages (`FText`, `LOCTEXT`/`NSLOCTEXT`, string tables, the Localization Dashboard, `FText::Format`, `FText::AsNumber`, culture-aware formatting).

**Rules:**

- **Every string the player reads is `FText`**: `LOCTEXT` in C++ with a `LOCTEXT_NAMESPACE` per file, `FText` properties in data assets, string tables for shared UI strings.
- **Build sentences with `FText::Format`** and named arguments (`{Day}`), never by concatenating strings, so translators can reorder words.
- Numbers, dates and the clock go through culture-aware formatting; the 24-hour clock stays the default.
- Set up the Localization Dashboard's gather once there is a menu, so missing text shows up early. It gathers from assets as well as code: the input actions' descriptions and mappable display names are asset text since H6 (2026-11-07), so the gather includes `Content/Vestige/Input`.

**Pitfalls:**

- `FText::FromString` on literal text, which makes it untranslatable.
- Text baked into textures.

## 19. Debugging and cheats

**Status:** built (2026-11-04, [Roadmap 4.0](Roadmap.md#40-structure-from-the-code-review)): `UVestigeCheatManager` (`Player/`) with the console commands `NewGame` (deletes both saves and reopens the map on day 1), `SkipDay` (the player sleeps where they stand), `SkipToTime <Time>` (the clock runs forward to a time today, typed as digits, 1930 for 19:30, broadcasting the hours it passes; added 2026-11-06) and `BenchmarkField <Size> <CropId>` (a Size by Size field of that crop, ripe, for the performance baseline in [Architecture.md, section 10](Architecture.md#measuring)). They replace the test keys F9 and N, which are no longer bound. No console variables or debug drawing yet. The target tile highlight is a shipping component, not debug drawing.

**References:** Epic's `UCheatManager` (console commands in development builds), console variables (`TAutoConsoleVariable`), the Gameplay Debugger and Visual Logger; Lyra's `ULyraCheatManager`.

**Rules:**

- Developer commands (add seeds, skip to a day, grow every crop) live in a `UVestigeCheatManager` (set as the player controller's cheat manager class) as `UFUNCTION(Exec)` functions. Cheat managers aren't created in shipping builds: `APlayerController::AddCheats` is compiled out there, and our controller sets `CheatClass` only under `UE_WITH_CHEAT_MANAGER`, the engine's own guard. Outside Shipping the engine creates one when the game mode's `AllowCheats` agrees (a standalone game, Play in Editor included).
- A command asks the owner of the action to do it (`SkipDay` calls the player's rest component's `Sleep`, as the bed does) and has no rules of its own, so a cheat does what the game does. A command that can't run (no player state, no save subsystem) logs a warning and does nothing.
- Commands are typed in the console while playing ([PlayTest.md, Before you test](PlayTest.md#1-before-you-test) says how to open it), named as verbs in PascalCase without a prefix (`NewGame`, `SkipDay`). Check the name isn't already an engine command (type it in the console first).
- Debug visuals sit behind console variables (`farm.Debug.Targeting 1`) and `#if ENABLE_DRAW_DEBUG`, never always on.
- No debug keys in the game's input: a developer action is a console command. Something players need (a new game) comes as a menu, behind a confirmation when it destroys data.
- Robots get Gameplay Debugger categories and Visual Logger output when their AI is built.

**Pitfalls:**

- Debug-only behavior that players come to rely on ([ArchitectureAudit A-10](ArchitectureAudit.md)).

## 20. Maps, loading screens and the main menu

**Status:** one map (`L_Farm`), no menu, no loading screen. Decided 2026-11-04: separate maps opened with `OpenLevel`, with the clock and the farm data in game-instance subsystems (built) ([Architecture.md, section 2](Architecture.md#2-unreals-gameplay-framework-where-things-go)).

**References:** Epic's "Level Streaming", "World Partition", "Level Instances" and "Data Layers" pages, and the MoviePlayer loading screen; Lyra's CommonLoadingScreen plugin (a loading screen shown during map loads and while systems report they're not ready) and its front-end map.

**Rules:**

- The farm, the town and the wasteland are separate maps opened with `OpenLevel` (decision log, 2026-11-04), not streamed sublevels or World Partition. What must go on while its map is closed lives in a game-instance subsystem; a level only shows it.
- A main menu is its own small map with its own game mode, set in its World Settings, so it loads nothing from the farm: `BP_GameMode`, the default for every map, hard-references the player's Blueprints.
- A loading screen covers map changes and stays up until the systems that load content at start (the tools and crops, the save) report ready, as Lyra's loading screen waits on its "loading processors".
- A main menu loads the chosen save with an explicit Continue (or starts a New game), instead of the save subsystem loading when the first map is initialised; the save is already read into the game instance, so travelling between maps never loads it again ([section 1](#1-saving-and-loading), [Architecture.md, section 6](Architecture.md#6-saving)).

**Pitfalls:**

- A map that hard-references every other map's content through a game mode or a Blueprint.

## 21. Logging

**Status:** built (2026-11-02, [Roadmap 4.1](Roadmap.md#41-foundations)) as designed below, approved by David on 2026-11-01: the categories in the table, `VESTIGE_LOG`, and every line in our code moved to them. Chosen on 2026-10-30: we build on Unreal's logging and write no logging tool of our own, which would rebuild what the engine has and lose the Output Log's filters and colours, the log files, crash reports and the `log` console command.

**References:** Epic's "Logging in Unreal Engine" page (log categories, verbosity levels, `UE_LOG`, the `log` console command and `[Core.Log]`), "Structured Logging" (`UE_LOGFMT`) and "Visual Logger". The engine source: `Logging/LogMacros.h` (`DECLARE_LOG_CATEGORY_EXTERN`, compile-time verbosity), `Logging/StructuredLog.h` (`UE_LOGFMT` checks the category's verbosity before it evaluates its fields) and `OutputLogSettings.h` (Category Colorization Mode). Lyra's `LyraLogChannels.h`: a few categories per area (`LogLyra`, `LogLyraExperience`, `LogLyraAbilitySystem`, `LogLyraTeams`) declared together, plus `LogLyraCheat` and `LogLyraGamePhase` declared next to their systems, and `GetClientServerContextString(this)` passed into lines that need to say where they came from.

**What we take from Lyra and what we skip:**

- **Take:** one category per system, named with the project prefix, each `Log` by default and `All` at compile time so its tracing can be turned on while the game runs; the context of the line passed in from the calling object.
- **Skip:** the client and server context string. We have no networking; the sender's name and function take its place.
- **Change:** Lyra's lines use `UE_LOG` with printf formats. We use `UE_LOGFMT` underneath our macro: its fields are typed, so the printf mistakes (an `FString` without `*`, `%d` for an `int64`) can't happen, and the fields stay separate for tools that read structured logs.

**Rules:**

- **One category per system,** not per class or per file, so a filter shows one system's whole story. Every line in our code (outside `Template/`) uses one of these:

  | Category | System | Declared in |
  | --- | --- | --- |
  | `LogVestige` | Level setup that belongs to no one system (`VestigeActorLookup`), and Epic's template code | `Core/VestigeLog.h` |
  | `LogVestigeFeedback` | Every message for the player, written by `UVestigeFeedbackSubsystem::Notify` | `Core/VestigeLog.h` |
  | `LogVestigeAbilitySystem` | Ability sets, stamina, abilities | `AbilitySystem/VestigeAbilitySystemLog.h` |
  | `LogVestigeContent` | Loading content, content IDs, the Asset Manager's checks | `Content/VestigeContentLog.h` |
  | `LogVestigeTime` | The clock and the day cycle | `Time/VestigeTimeLog.h` |
  | `LogVestigeInventory` | Items and the inventory | `Inventory/VestigeInventoryLog.h` |
  | `LogVestigeInput` | The input config and the player's bindings | `Player/VestigeInputLog.h` |
  | `LogFarm` | The farm: grid, tiles, crops, tools | `Farming/FarmLog.h` |
  | `LogVestigeSave` | Saving and loading | `Core/VestigeSaveLog.h` |

- **Names follow the type prefixes** (AGENTS rule 2): `LogVestige<System>`, and `LogFarm` for the farm, which is one system today (`LogFarm<System>` if it ever needs splitting). A new system gets its category with its first line (`LogVestigeInteraction`, `LogVestigeRest`, `LogVestigeHud`), not before, in the folder of the code that uses it, and this table gets a row.
- **Declared in the layer's own log header,** `<Layer>/<Prefix><System>Log.h` with `DECLARE_LOG_CATEGORY_EXTERN(LogVestigeSave, Log, All);`, and defined in the matching `.cpp` with `DEFINE_LOG_CATEGORY`. The layer's log header includes `Core/VestigeLog.h`, so a file includes only its own layer's log header to get both its category and the macro, and the include direction test keeps holding. `Core/Vestige.h` keeps including `Core/VestigeLog.h`, so the template code compiles unchanged.
- **Every line goes through `VESTIGE_LOG(Sender, Category, Verbosity, Format, ...)`** in `Core/VestigeLog.h`, a thin macro over `UE_LOGFMT`. The sender comes first, as in Epic's `UE_VLOG`. It puts the sender's name and the function in front of the message:

  ```cpp
  VESTIGE_LOG(this, LogFarm, Warning, "No Tile Class is set, so the field is empty.");
  VESTIGE_LOG(this, LogVestigeSave, Error, "Couldn't write the save to slot {Slot}. The previous save is kept.", SlotName);
  ```

  prints

  ```
  LogFarm: Warning: [FarmGrid_1] AFarmGrid::SpawnTiles: No Tile Class is set, so the field is empty.
  LogVestigeSave: Error: [VestigeSaveSubsystem_0] UVestigeSaveSubsystem::SaveFarm: Couldn't write the save to slot Farm. The previous save is kept.
  ```

   - The sender is any `UObject`: `this` in members; in a static or free function the object the line is about (the world for `VestigeActorLookup::FindSingle`), or `nullptr`. Its name comes from `VestigeLog::GetSenderName`: an actor's or subsystem's own name (`FarmGrid_1`), a component's owner and name (`BP_PlayerCharacter_C_0.FarmingComponent`, since a component's name alone isn't unique), `None` for `nullptr`.
   - The function is the compiler's `__FUNCTION__`, which on MSVC includes the class (`AFarmGrid::SpawnTiles`).
   - The format is a plain string literal (no `TEXT()`), with fields in braces named after what they hold (`{Slot}`, `{ItemId}`) and given in order after it. Named fields can't be mixed with the macro's own, so they aren't used.
   - Field values are numbers, `bool`, `FString`, `FName` and the engine's ID types. Pass an `FText` as `.ToString()`, an `FIntPoint` or vector as `.ToString()`, and an object as `GetNameSafe(Object)`.
   - `UE_LOG`, `UE_LOGFMT` and their conditional forms aren't called directly outside `Template/` and the macro. A source test checks it.
- **The message** is a full sentence that says what happened and what follows from it, as the lines do today: "The save has a crop {CropId} that no longer exists, so tile {Tile} is empty." It doesn't repeat the sender's name, which the macro already prints.
- **Verbosity:**

  | Verbosity | Use it for | Today's examples |
  | --- | --- | --- |
  | `Fatal` | Never. It crashes the game; use `check` for programmer errors ([CodingGuidelines.md, section 10](CodingGuidelines.md#10-errors-logging-and-assertions)) | |
  | `Error` | Broken state someone must fix: the game carries on, but something is lost or turned off. Fails an automation test that logs it, unless the test expects it | Duplicate content IDs, a save that couldn't be written or read, two grids in a level |
  | `Warning` | A setup or content problem the game works around: a missing setting, an unknown ID in a save, a fallback spawn | No Tile Class, an unknown item ID, no room for saved items |
  | `Display` | Rare: a line for someone reading the console window of a packaged game, where `Log` lines don't show | None yet. Later: which mods were loaded |
  | `Log` | Notable one-off events, a handful per day of play | A save loaded, a backup made, item definitions loaded, a player message |
  | `Verbose` | Tracing one action: a tool use, an interaction, a tile changing state. Off by default | None yet |
  | `VeryVerbose` | Tracing inside loops: each tile on a new day, each slot when loading. Off by default | None yet |

- **Volume:** nothing is logged every frame or on every tick of a timer that always runs (the interaction scan), at any level, not even `Verbose`: a suppressed line still costs a check, and turning tracing on must leave the game playable. At `Log` and above, a line per player action is already too much; that is `Verbose`. Arguments are evaluated only when the line is printed, so put the work (`ToString`, `GetNameSafe`) in the macro's arguments rather than in locals built before it.
- **Turning a category up or down:**
   - While playing, in the console (the backtick key, or the Output Log's command box): `log LogFarm Verbose`, `log LogVestigeSave Warning`, `log list` to see every category, `log reset` to go back.
   - From the start of a run: the command line `-LogCmds="LogFarm Verbose, LogVestigeSave Verbose"`.
   - For good: a line under `[Core.Log]` in an `Engine.ini`, such as `LogFarm=Verbose`. Personal settings go in `Saved/Config/WindowsEditor/Engine.ini`, which git ignores; `Config/DefaultEngine.ini` gets a `[Core.Log]` line only for a decision everyone shares, recorded in the decision log.
   - Shipping builds compile logging out (our `Vestige.Target.cs` doesn't set `bUseLoggingInShipping`), so the lines cost nothing there.
- **Reading our lines in the editor's Output Log** (Window > Output Log): the Filters menu has Categories, where we tick ours (it lists only the categories that have written a line since the editor started, so a system that only warns, like `LogFarm`, is missing until it does), and the verbosity filters (Messages, Warnings, Errors); the search box filters by text, so `LogVestige` finds every Vestige category. Editor Preferences > General > Output Log > Category Colorization Mode gives each category its own colour (David uses Colorize Whole Line, which leaves warnings and errors in their own yellow and red). Both are per-user editor settings, nothing committed; [PlayTest.md](PlayTest.md) says how to set them. The full log of each run is in `Saved/Logs/Vestige.log`.
- **Reading a line.** In the log file and the console window a line looks like this (the Output Log window leaves out the first two parts, unless its Output Log Window Timestamp Mode, on the same preferences page, adds a time):

  ```
  [2026.11.02-19.56.43:858][647]LogFarm: Warning: [BP_PlayerCharacter_C_0.Farming] UFarmingComponent::HandleItemInHandChanged: The item SomeMod.Scythe names the tool VestigeTool:SomeMod.Scythe, which doesn't exist, so the hand is used.
  ```

  | Part | Written by | What it is |
  | --- | --- | --- |
  | `[2026.11.02-19.56.43:858]` | The engine | When: date, then time to the millisecond, in UTC (the engine's default, `LogTimes=True` in `BaseEngine.ini`), so an hour behind Swedish winter time |
  | `[647]` | The engine | The frame the line was written in, counted modulo 1000, so it wraps from 999 to 0. Lines with the same number come from the same frame; the gap between two numbers is how many frames passed |
  | `LogFarm:` | The engine | The category: which system wrote it |
  | `Warning:` | The engine | The verbosity. Left out for `Log` lines |
  | `[BP_PlayerCharacter_C_0.Farming]` | `VESTIGE_LOG` | The sender: the object that wrote the line (here the `Farming` component on the player character), the level's world for code that has no object (`[L_Farm]`), or `None` |
  | `UFarmingComponent::HandleItemInHandChanged:` | `VESTIGE_LOG` | The C++ function the line comes from, with its class |
  | The rest | The line itself | The message, with its fields (`SomeMod.Scythe`) filled in |
- **The Visual Logger** (Tools > Debug > Visual Logger, `UE_VLOG` and its shape macros) records lines and shapes per actor over time and plays them back on a timeline. It is for gameplay that moves: the robots' AI gets Visual Logger output with its Gameplay Debugger categories when it is built ([section 19](#19-debugging-and-cheats)). The farm doesn't need it today; its rules are tested instead.
- **Logs aren't feedback.** A line in the log is for developers and modders. What the player must know goes through `UVestigeFeedbackSubsystem::Notify` ([CodingGuidelines.md, section 10](CodingGuidelines.md#10-errors-logging-and-assertions)), which also logs it to `LogVestigeFeedback`.
- **Tests** (`Vestige.Logging.*`, in `Tests/VestigeLogTests.cpp` on a test-only category): a line through the macro is exactly `[sender] function: message` with its fields filled in (captured with an output device on `GLog`); a component's sender name is its owner's name and its own, an actor's is its own, and `nullptr` gives `None`; nothing in the macro's arguments is evaluated while the category is turned down. `Vestige.Architecture.LogsUseTheSenderMacro` scans our source, as the include direction test does, and fails on a direct `UE_LOG`, `UE_CLOG`, `UE_LOGFMT` or `UE_CLOGFMT` outside `Template/`, `Tests/` and `Core/VestigeLog.h`.

**Pitfalls:**

- One category for everything (as `LogVestige` was until 2026-11-02): the Output Log's category filter can't show one system's story.
- A category per class: a filter for one system means ticking a dozen boxes, and the colours stop meaning anything.
- `Warning` or `Error` for something that is normal play (no seeds left, the inventory full). Those are player feedback, maybe a `Verbose` line.
- `Error` in code that tests run on purpose: the test fails unless it declares the error it expects.
- Logging inside `Tick`, a timer that always runs, or a Slate binding.
- An on-screen debug message instead of a log line (AGENTS rule 8).

## 22. Farm fields and tiles

**Status:** the data is built: `UFarmFieldSubsystem` keeps one `UFarmField` per field ID, each a row-by-row array of `FFarmTileState` with the farming rules (2026-11-04, [CodeReview.md, C1](CodeReview.md#c1--all-game-state-lives-in-the-level-but-the-game-needs-several-maps)). Drawing with instanced meshes is built as designed below (2026-11-05, [Roadmap.md, 4.0](Roadmap.md#40-structure-from-the-code-review), [CodeReview.md, C2](CodeReview.md#c2--one-actor-per-tile-each-with-two-meshes-and-two-dynamic-materials)), approved by David on 2026-11-05. Until then `AFarmGrid` spawned an `AFarmTile` actor per tile, each with two mesh components and two dynamic materials. Water and radiation per tile ([Roadmap.md, 4.2](Roadmap.md#42-farming-depth)) and placing objects on the grid ([section 16](#16-placing-objects-and-building)) build on it.

**References:**

- Epic's "Instanced Static Mesh Component" page and the `UInstancedStaticMeshComponent` class reference: many copies of one mesh drawn as one primitive, each instance with its own transform and, optionally, a few floats of per-instance custom data.
- Epic's "Nanite Virtualized Geometry" page: Nanite draws every instance of an instanced component through the GPU scene and culls each instance there. The hierarchical variant (`UHierarchicalInstancedStaticMeshComponent`) adds CPU-side culling and LODs for meshes without Nanite, so Nanite art doesn't need it.
- The material nodes **PerInstanceCustomData** and **PerInstanceCustomData3Vector**: a material reads an instance's custom data floats, which replaces a dynamic material per object. A material drawn on instances needs **Used with Instanced Static Meshes** ticked and saved; UE 5.8 doesn't set missing usage flags during play, and such a material draws the grey default material instead.
- The engine source, `Components/InstancedStaticMeshComponent.h` in 5.8: `AddInstanceById`, `RemoveInstanceById`, `UpdateInstanceTransformById` and `SetCustomDataById` name an instance by an `FPrimitiveInstanceId`, which stays the same when other instances are removed. An instance's index doesn't: removing one moves the others.
- Lyra has nothing like a tile grid. We take only its general rule that data and presentation are separate objects.

**How the parts fit:**

| Part | Kind | Owns |
| --- | --- | --- |
| `UFarmField` + `FFarmTileState` | Object in `UFarmFieldSubsystem`, struct per tile | The tiles as data, the rules, and the events `OnTileChanged` (one tile) and `OnTilesChanged` (many at once). Unchanged by this design |
| `AFarmGrid` | Actor placed in the level, as `BP_FarmGrid` | Where its field lies (the actor's corner, `TileSize`), finding its field by `FieldId`, converting between world space and coordinates, a tile's transform in the world (`GetTileTransform`), `OnFieldReady` once it has its field, resizing for developer tools. The interactable for its tiles: offering Harvest while the interactor faces a ripe tile, from `InteractionVolume` over the field and one tile around ([section 7](#7-interaction), harvesting with F). No tile actors and no drawing |
| `UFarmFieldMeshesComponent` | Scene component on the grid | Drawing the grid's field: one instanced static mesh component per instance set, one soil instance and at most one crop instance per tile, kept up to date from the field's events. Presentation only |
| `FarmTileLooks::GetLook` | Pure function (`FarmTileLooks.h`) | What one tile looks like: for its soil and its crop, the mesh, material override, transform in the grid and colour, from the tile's state and the meshes component's settings. Tested without a world |

**Rules:**

- **Tiles are data, not actors.** Nothing spawns an actor or creates a `UObject` per tile. Game code names a tile by its field and its coordinates (column X, row Y) and asks the field to change it. Something on the grid becomes an actor only when it needs to be one, such as a placed sprinkler with behaviour ([section 16](#16-placing-objects-and-building)).
- **One instanced static mesh component per instance set.** A set is one mesh with one material override (none means the mesh's own materials): grass, tilled soil, watered soil, each crop growth stage mesh, and the placeholders. The meshes component creates a set's component the first time a tile needs it and keeps it until the grid ends; a set's mesh never changes. A tile whose look changes moves its instance to another set.
- **Per-tile numbers are per-instance custom data, never a dynamic material.** Every set has the same custom data layout, so a material reads the same index on every mesh:

  | Index | Holds | Read by |
  | --- | --- | --- |
  | 0 to 2 | Colour (linear RGB) | `M_FarmPlaceholder`, the placeholder shapes' material. The Blender art ignores it |
  | 3 and up | Free, for [4.2](Roadmap.md#42-farming-depth): the water amount, then radiation | The soil material that 4.2 makes |

  The meshes component sets `NumCustomDataFloats` when it creates a set's component, before it adds an instance.
- **Watered soil is its own set for now.** It is the tilled soil mesh with `M_Soil_Watered`, as it is drawn today. The Blender soil's materials are instances of the Interchange importer's material and can't read custom data, and in this milestone water is only on or off. When 4.2 makes water an amount, the soil gets a project material that reads it from custom data, and the watered set goes away.
- **Instances are named by ID.** For each tile the meshes component keeps which set and which `FPrimitiveInstanceId` its soil and its crop instance have, in an array in the field's tile order. It never stores an instance index.
- **Updates follow the field's events,** and nothing ticks:
   - `OnTileChanged` (a tool use): only that tile. If a part keeps its set, its transform and custom data are updated in place; otherwise the instance is removed from the old set and added to the new one.
   - `OnTilesChanged` (a new day, a restore, a resize): every set is rebuilt in one pass. The sets are cleared, each set's instances are added in one batch (`AddInstancesById`) with their custom data. The component's instance data manager sends only the changed instances to the renderer, so nothing marks render state dirty. It runs under a `TRACE_CPUPROFILER_EVENT_SCOPE` and counts against the new day's 100 ms ([Architecture.md, section 10](Architecture.md#budget)).
- **The set components:** no collision, no overlap events, no effect on navigation, shadows on, ticking off, mobility Movable (the engine's default for instanced components; a static one may not change its instances during play, and an unmoving movable primitive keeps its cached virtual shadow map pages). They are attached to the meshes component with instances in the grid's space, so moving the grid in the editor moves the field.
- **The look settings live on the meshes component and are set in `BP_FarmGrid`:** the tilled soil mesh, the watered soil material, the height crops stand at, the placeholder material, and the placeholder shapes (the engine's cube and cylinder, found with `ConstructorHelpers` as [CodingGuidelines.md, section 12.4](CodingGuidelines.md#124-assets-and-loading) allows for engine placeholders). Anything left empty falls back to the placeholder. Crop meshes come from each crop's data, loaded with its `Visuals` bundle by whoever loaded the crop, so drawing never loads an asset. A level places `BP_FarmGrid`, never the C++ `AFarmGrid`, so every map's farm looks the same.
- **Gameplay never calls the meshes component.** The field's events reach it directly. Targeting, the target highlight and anything else that needs a tile's place ask the grid for coordinates or a transform.
- **Extension points name tiles by coordinates.** `On Crop Harvested` and `On Tool Used` carry the tile's coordinates (an `FIntPoint`) where they carried the tile actor ([Architecture.md, section 12](Architecture.md#extension-points-today)).
- **One set of components per field.** Splitting a large field into chunks (for example 32 by 32 tiles each, so culling and rebuilds work on smaller pieces) waits for a profile of a farm-sized field that shows the need.
- **Tiles have no collision.** The farmer walks on the level's floor. If placing objects needs to know what is on a tile, the grid answers by coordinates, not with a shape per tile.

**Tests** (`Source/Vestige/Tests/`):

- `Vestige.FarmTileLooks.*`: the look of each state with art and with placeholders (grass, tilled, watered, each growth stage, ripe), without a world.
- `Vestige.FarmFieldMeshes.*`: a grid with a field of N tiles draws N soil instances and spawns no actor for them; tilling, watering, planting and harvesting move or add or remove exactly one instance; a new day that grows a crop moves it to its next stage's set; a resize redraws every tile; no `UMaterialInstanceDynamic` is created; `M_FarmPlaceholder` has the instanced usage flag.
- The rule tests (`FFarmTileState`, `UFarmField`) don't change. The farming, targeting and cheat tests name tiles by coordinates instead of actors.

**Pitfalls:**

- Storing instance indices. Removing an instance moves others (or swaps the last into its place), so a stored index soon points at another tile's instance. Use the IDs.
- Calling `SetStaticMesh` on a set's component to change a tile's look. That changes every tile in the set; move the instance to another set.
- A material without **Used with Instanced Static Meshes**: grey tiles in play, and a "missing usage flag" line in the log.
- Custom data written before `NumCustomDataFloats` is set is dropped without a warning.
- Marking render state dirty or updating navigation per instance inside a loop over the field.
- The hierarchical instanced component for Nanite art: it costs CPU time for culling the GPU already does.
- A dynamic material per tile "just for the placeholder": the placeholder is what the field looks like until there is art, so it has to scale too.
