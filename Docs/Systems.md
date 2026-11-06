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

**Status:** built (`UVestigeSaveSubsystem`, `UVestigeSaveGame`, and each owner's save struct next to the owner), with the safety rules in [Architecture.md, section 6](Architecture.md#6-saving): a save version (2, with no upgrades until the first public build), owners saving their own parts, each map's field by its ID with its tiles by coordinates and crop IDs, two alternating slots, and a save that can't be read left untouched with saving off. The decision of what to load (`UVestigeSaveSubsystem::DecideLoad`) is a static function, so its rules are tested without touching the player's save files.

**References:** Epic's "Saving and Loading Your Game" (`USaveGame`, `UGameplayStatics::SaveGameToSlot` and `AsyncSaveGameToSlot`); `UPROPERTY(SaveGame)` with `FObjectAndNameAsStringProxyArchive` (`ArIsSaveGame = true`) for serializing marked properties; `FCustomVersion` for versioned binary data. Lyra only saves settings (`ULyraSettingsShared`, a `USaveGame` written with `AsyncSaveGameToSlot`), not the world, so this guide relies on Epic's documentation more than on Lyra.

**Rules:**

- **Each owner saves itself.** An owner returns a plain `USTRUCT` of its saved state (`FFarmTileSaveData`) and restores from one. The save subsystem only collects, versions and writes ([Architecture.md, section 6](Architecture.md#6-saving)).
- **Save data is values and IDs:** numbers, enums, `FName` content IDs, grid coordinates. Never object pointers, asset paths, array positions or display text.
- **A save version** (`int32 SaveVersion` on `UVestigeSaveGame`). Until the first public build there are no migrations: a change that makes older saves load wrong raises the version, and older saves are refused like a corrupt file (decision log, 2026-11-04). From the first public build, one migration function per old version, each tested ([Roadmap.md, section 6](Roadmap.md#6-before-a-public-release)).
- **Write safely.** Write the new save to a second slot, check the write succeeded, then make it the current slot (two alternating slots, each with a sequence number, and load the newest one that reads cleanly). A failed write never touches the last good save.
- **Save at quiet moments** (the new day) and synchronously while it fits the new-day budget ([Architecture.md, section 10](Architecture.md#10-performance-by-design)); switch to `AsyncSaveGameToSlot` when it doesn't, and never start a map change while a write is in flight.
- **Restore without side effects:** restoring sets state; it never replays actions or broadcasts gameplay events like `OnNewDay`.
- **Unknown content IDs are skipped with a warning** and the player is told; the rest of the save loads.
- Saving the player's settings is separate ([section 17](#17-settings)).

**Pitfalls:**

- `UPROPERTY(SaveGame)` with archive serialization is tempting for "save every actor automatically", but it couples the save layout to class layouts and breaks on renames. Use it at most inside an owner's own save struct, not across the world.
- Saving a world subsystem's state that should survive a map change. Slot choice moves to a game-instance subsystem first ([Architecture.md, section 6](Architecture.md#6-saving)).
- **Changing the default value of a saved property.** Unreal leaves a property out of the save file when its value equals the default, so an old save would read the new default instead of the value it had. Keep defaults of saved properties as they are; add a new property (and a version step) instead.
- Testing only the round trip. Also test an old version, a corrupt file and a newer version.

## 2. Content and the Asset Manager

**Status:** built for crops, tools and items (`UVestigeContentData`, `UFarmCropData`, `UVestigeToolData`, `UVestigeItemData`). `UVestigeContentData::IsDataValid` checks the ID format, and `UVestigeAssetManager` reports duplicate IDs after the initial asset scan. The ID redirect list is deferred until an ID has to change.

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

**Status:** in use for seasons (`Vestige.Season.*` in `Config/DefaultGameplayTags.ini`) and, as native tags in `AbilitySystem/VestigeGameplayTags.h`, for the stamina system ([section 9](#9-abilities-attributes-and-effects-gameplay-ability-system)).

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

- One owner of game time (`UVestigeClockSubsystem`), in the game instance so it keeps time across maps; its speed and hours, sunrise and sunset included, are project settings (`UVestigeClockSettings`), never a level's. Everything else reads it or listens to its events (`OnNewDay`, `OnNewDayFinished`, `OnHourStarted`, `OnDayEndReached`). Nothing keeps its own clock: something that happens at a time of day listens to `OnHourStarted`, which carries the hour since the start of the day's midnight (22 for 22:00, 24 for the next midnight), rather than watching the clock or running a timer of its own.
- Blueprints, and so mods, hear of a new day through `On New Day` (`OnNewDayBlueprint`), the extension point broadcast after `OnNewDay` and before the save ([Architecture.md, section 12](Architecture.md#extension-points-today)). A Blueprint finds the clock with **Get Vestige Clock**.
- Game time is separate from real time and from `UWorld::GetTimeSeconds`. Pausing and menus stop game time: the clock ticks with the open world, so pause stops it and time dilation (`slomo`) speeds it.
- Daily changes happen in `OnNewDay` listeners that finish synchronously ([Architecture.md, section 15](Architecture.md#15-the-day-flow)).
- Seasons are gameplay tags and change on a day event; weather is chosen once per day from data (per-season weights), so it's saved as a value, not re-rolled on load.
- Lighting, fog and sky react to time and weather through the sky lighting actor (`AVestigeSkyLighting`, one per outdoor level), which reads the clock and its sunrise and sunset hours and never changes them; nothing else moves the sun. The sun turns in small steps, not every frame, so virtual shadow maps keep its cached shadows between them; brightness and colour may change every frame.

**Pitfalls:**

- Timers (`FTimerManager`) for game-time events: they run on real time and don't save. Schedule game-time events against the clock.
- Comparing hours as floats for equality; compare against ranges or minute counts.

## 5. Input

**Status:** Enhanced Input, with farming actions and their mapping context created once in code (`AVestigePlayerCharacter`). The HUD's key hints are read from that mapping context. Moving to assets is a milestone 2 decision.

**References:** Epic's "Enhanced Input" pages (input actions, mapping contexts, triggers, modifiers, player-mappable key settings); Lyra's `ULyraInputConfig` (a data asset mapping input actions to input tags), `ULyraInputComponent` and `ULyraHeroComponent`, which bind actions by tag to abilities.

**Rules:**

- Input actions (`IA_`) and mapping contexts (`IMC_`) become assets once there are more than a handful of keys or once players can rebind keys. Mapping contexts are added and removed by context (on foot, in a menu, driving a machine) with explicit priorities.
- The character binds input to calls on the components that own the action (`UFarmingComponent::StartUsingTool` on the use key's `Started`, `StopUsingTool` on its `Completed`); input code holds no game rules. Holding a key to repeat an action is the owner's rule, driven by events rather than by `Triggered`, which fires every frame: the farming component starts the next use when the last one ends (the ability system's `OnAbilityEnded`) or the target tile changes, and only if the use would change the tile, so a held key never posts a message every frame. While it is held the farmer walks and jumps at half speed, a setting on the movement component (`ToolUseSpeedScale`), so a row is worked evenly. No tool works in the air (the use-tool ability's `CanActivateAbility`), and a held key resumes on landing, so jumping can't get round the slower walk. Movement input also aims the farming component at a neighbouring tile (`AimAt`), and the walk eases in from the press, read from the move action's elapsed time (`FInputActionInstance::GetElapsedTime`), so the template's `IA_Move` needs no extra trigger or modifier. Turning belongs to the farmer's movement component, `UVestigeCharacterMovementComponent`, which turns toward the aimed tile when there is no input.
- When the Gameplay Ability System arrives, follow Lyra: a `UVestigeInputConfig` data asset maps each action to an input tag, and abilities activate by input tag, so new abilities don't need new binding code.
- Key hints on screen come from the mappings, never hard-coded text ([ArchitectureAudit A-14](ArchitectureAudit.md)).
- Player rebinding uses Enhanced Input's player-mappable key settings, saved with the settings ([section 17](#17-settings)).
- Gamepad support is designed in from the first asset-based mapping: every action has a gamepad key.

**Pitfalls:**

- Adding a mapping context every time a pawn is possessed without removing the old one.
- Using the legacy input system (`BindAxis`, `BindAction` on `UInputComponent`) in new code.

## 6. User interface

**Status:** a C++ Slate HUD (`AVestigeHUD`, `SVestigeHudWidget`). Menus and an inventory in UMG are milestone 2.

**References:** Epic's "UMG UI Designer", "Common UI" and "UMG Viewmodel" (MVVM plugin) pages; Lyra's UI: `UGameUIManagerSubsystem` and `UGameUIPolicy` (CommonGame plugin), a `UPrimaryGameLayout` with layers (game, game menu, menu, modal) that widgets are pushed onto, and the UIExtension plugin (`UUIExtensionSubsystem`), which lets features add widgets to named HUD slots without the HUD knowing them.

**Rules:**

- **Presentation only.** Widgets read state and listen to events; they never change game state except by calling the owner's public functions in response to player input ([Architecture.md, section 7](Architecture.md#7-presentation-ui-audio-and-effects)).
- **UMG Widget Blueprints with C++ base classes** for menus and the inventory: the C++ base holds the logic and binds to events, and the Blueprint holds only layout and style (`meta = (BindWidget)` for the parts C++ needs).
- **Event-driven updates.** Widgets update when an event fires, not through UMG property bindings, which run every frame ([CodingGuidelines.md, section 12.6](CodingGuidelines.md#126-user-interface)). The MVVM plugin's viewmodels are a good fit once screens have many fields.
- **Layers.** Once there is more than one menu, adopt Lyra's layer idea: one root layout with named layers, and screens pushed and popped on a layer stack. Common UI provides this plus gamepad navigation and input routing; adopt it when gamepad menus are needed, and log the decision.
- **HUD extension points.** For mods and for features that add HUD elements (stamina, weather), use named slots the way Lyra's UIExtension does, rather than editing the HUD for each feature.
- All text is `FText` ([section 18](#18-localization)); 24-hour clock everywhere.

**Pitfalls:**

- Widgets that cast to the character or search the world to find their data. Give them their data when they're created, or let them find it once.
- Creating and destroying widgets often (inventory slots on every refresh). Create once, update contents.

## 7. Interaction

**Status:** built (2026-11-01, milestone 2 foundations, [Roadmap 4.1](Roadmap.md#41-foundations)) as designed below, approved by David on 2026-11-01 with the option list and N kept as a test key: `IVestigeInteractable`, `UVestigeInteractionComponent` on the player's pawn, the interact ability, the F key and the HUD prompt. The first interactable is the bed ([Roadmap 4.3](Roadmap.md#43-stamina-and-late-nights)), one placed in `L_Farm`.

**References:** Epic's "Interfaces" page (`UINTERFACE`, `BlueprintNativeEvent` functions called through `Execute_`, `Implements<U...>()`), "Collision Overview" and "Collision Response Reference" (custom object channels and collision profiles in Project Settings > Collision), and `UWorld::OverlapMultiByObjectType`. Lyra's interaction (`Source/LyraGame/Interaction`): `IInteractableTarget` fills an `FInteractionOption` list (text, sub-text, the ability to run and the target's own ability system), `UAbilityTask_GrantNearbyInteraction` runs an overlap query around the player on a timer (every 0.1 s) on Lyra's interaction channel, `UAbilityTask_WaitForInteractableTargets_SingleLineTrace` picks the one the camera looks at, and `ULyraGameplayAbility_Interact` shows the prompt and activates the option's ability, which receives the target in its event data.

**What we take from Lyra and what we skip:**

- **Take:** one interface every usable thing implements; the option list, so an object can offer several actions without the interface (a mod API) changing when the first one does; a cheap overlap query around the player on a timer, on a dedicated collision channel; using it as a gameplay ability, so tags can block it later (in a menu, in dialogue).
- **Skip:** granting each target's ability to the player and running the option's ability (Lyra's `InteractionAbilityToGrant`): the interactable does the action itself. That machinery exists for networked play, where the server must run the ability.
- **Until there is a way to choose** (a menu, or a key per option), the interact key uses the first option (decided with David, 2026-11-01). An interactable lists its options with the one the player most likely wants first.
- **Skip:** the camera line trace. Our camera is a third-person follow camera over a farm; the nearest interactable is what the player means. A facing rule comes if two interactables stand close enough to confuse it in play.

**Rules:**

- **One interface, `IVestigeInteractable`** (`UINTERFACE(Blueprintable)`), so C++ actors and Blueprint actors (and so mods) can both be interactable. Two `BlueprintNativeEvent` functions, called through `IVestigeInteractable::Execute_...`:
   - `GetInteractionOptions(Interactor, OutOptions)`: fills `OutOptions` with what can be done with it now, each an `FVestigeInteractionOption` (`BlueprintType` struct): `Text`, the prompt's verb (`Sleep`), and `Action`, an `FName` the interactable chooses to tell its own options apart (`Sleep`). No options means there is nothing to do right now, and the actor isn't offered. It must be cheap and change nothing: it is asked on every scan. Later fields (a second line, an icon) are added to the struct without changing the function.
   - `Interact(Interactor, Option)`: does the chosen option. The interactable owns what happens and asks the interactor's own components for anything about the player (the bed asks the interactor's `UVestigeRestComponent` to `Sleep`). The interactor never knows the interactable's class: no `if` per class.
   - Both are extension points ([Architecture.md, section 12](Architecture.md#extension-points-today)) and listed in `Vestige.ExtensionPoints.*`.
- **Interactables are found by collision, not by class.** A new object channel, `Interactable` (default response Ignore, so nothing else notices it), and a collision profile `VestigeInteractable` (object type `Interactable`, queries only, ignoring every channel: a query by object type finds it without a response) for the shape that marks where an actor can be used from: a box component a little bigger than the bed. The actor's own blocking collision stays as it is.
- **`UVestigeInteractionComponent` on the player's pawn owns finding and using interactables.** Every `ScanSeconds` (0.2 s) a timer runs one `OverlapMultiByObjectType` for `Interactable` in a sphere of `InteractionRadius` (100 cm, one tile) around the pawn, keeps the overlapping actors that implement the interface and offer at least one option, and focuses the nearest. It gathers the options into one array it reuses, so a scan allocates nothing once warmed up. It keeps the focus as a weak pointer with its first option, and broadcasts `OnFocusChanged` only when the focused actor or that option changes. No tick; the timer stops at `EndPlay`.
- **Interacting is a gameplay ability**, `UVestigeGameplayAbility_Interact`, granted by `DA_AbilitySet_Player` and activated by its asset tag `Vestige.Ability.Interact` (a native tag), as using a tool is. The interaction component's `Interact` activates it; the ability asks the component to rescan once (so the focus is current, not up to 0.2 s old) and calls `Interact` on the focus with its first option, then ends. It is instanced per actor, has no cost and takes no time today; an interaction that takes time later (filling the watering can) holds it active as the use-tool ability does.
- **Input: F interacts** (`Interact`, an input action the character maps and binds to the interaction component, shown in the key hints like the others). E stays the tool, so a tile next to the bed can still be hoed. **The bed is how the player sleeps.** Sleeping is instant: it starts the next day at 06:00, so there is no sleeping state and nothing to wake from. **N stays as a test key** that skips to the next day anywhere (decided with David, 2026-11-01), with the key hint "Skip day (test)", and moves into the cheat manager with F9 before a public build ([section 19](#19-debugging-and-cheats)). Done 2026-11-04: N is now the `SkipDay` console command. The 02:00 pass-out stays until its own roadmap item removes it.
- **The prompt on the HUD** is presentation: `AVestigeHUD` listens to `OnFocusChanged` on the possessed pawn's interaction component and shows "F Sleep" (the interact key, read from the mapping context like the other key hints, and the first option's text) while something is focused. Nothing is drawn in the world yet; an outline or a marker over the focused actor is a later presentation component.
- **The bed, `AVestigeBed`** (in `Farming/`, next to the rest component it asks): a static mesh with blocking collision (an engine cube set in C++ as a placeholder until the bed mesh in [Roadmap 4.6](Roadmap.md#46-interface-and-input)) and the interaction box. It offers one option, `Sleep`, whenever the interactor has a rest component, and interacting with it calls that component's `Sleep`, so the rules of sleeping (the refill, later bedtime and exhaustion) stay with the rest component. `BP_Bed` in `Content/Vestige/Blueprints` is the bed levels place and will hold the real mesh; one lies along the near edge of the field in `L_Farm`, so a tile can be worked from beside it, until there is a farmhouse.
- **Layers:** the interface, the interaction component and the ability need nothing above `AbilitySystem/`, and everything usable (the bed in `Farming/`, the shipping bin, robots) sits above them, so they get a layer folder of their own, `Interaction/`, right above `AbilitySystem/`. The tag joins `AbilitySystem/VestigeGameplayTags`.
- **Saving:** nothing. The focus is recomputed, and an interactable that has state (a chest) saves it as its own owner ([section 1](#1-saving-and-loading)).
- **Performance:** one overlap query on a channel only interactables use, five times a second, and a loop over the few actors it returns; no world search, no allocation once the options array has grown, no strings rebuilt unless the focused option changed, no tick. The query's cost goes in the first performance baseline.
- **Tests** (`Vestige.Interaction.*`): the nearest interactable with options is focused, one without options is skipped, one out of range isn't found, the focus event fires only on a change, interacting calls the focused actor with its first option and does nothing without one, and the ability is activated by its tag. The bed (`Vestige.Interaction.TheBed...`) sleeps the interactor (the day advances; the refill is the rest component's, tested in `Vestige.Rest`) and offers nothing to an actor without a rest component. The ability test uses `BP_PlayerState`, so it also fails if `DA_AbilitySet_Player` doesn't grant the interact ability, and a test checks the channel and profile in `DefaultEngine.ini`. The interface functions are listed in `Vestige.ExtensionPoints`.

**Pitfalls:**

- One `if` per interactable class in the farmer's code: the reason the interface exists.
- Calling an interface function directly (`Cast<IVestigeInteractable>(Actor)->Interact(...)`): a Blueprint that implements the interface has no C++ interface behind it, so the cast returns null and the Blueprint is never asked. Always call `IVestigeInteractable::Execute_Interact(Actor, ...)` and test with `Actor->Implements<UVestigeInteractable>()`.
- Interactables on the `Visibility` or `Pawn` channels, or a scan with `OverlapMultiByChannel` on a shared channel: every query would also return the farm tiles and other actors. Keep the dedicated object channel.
- An options function with side effects or a load: it runs five times a second while the player is near.

## 8. Inventory and items

**Status:** milestone 2, in two steps. **Item definitions** (`UVestigeItemData` and its fragments) are built as designed below, approved by David on 2026-10-30, with two items so far: `Base.TurnipSeeds` and `Base.Turnip`. **The inventory** (`UVestigeInventoryComponent` on `AVestigePlayerState`) is built as designed below, approved by David on 2026-10-31: seeds and harvests are items in it, and the player starts with 10 `Base.TurnipSeeds`. Money waits for the shipping bin.

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
- **Fragments come with the system that reads them:** a tool fragment (naming a `VestigeTool` ID) when the toolbar becomes part of the inventory, a food fragment with stamina, a placeable fragment with placement ([section 16](#16-placing-objects-and-building)), and an icon (a soft texture in the `Visuals` bundle) and description with the inventory screen. Item category tags wait for the shipping bin or gifts.
- **Data validation** (`IsDataValid`) on top of the ID check: `MaxStackSize` at least 1, no empty fragment entries, at most one fragment of each class, and each fragment checks its own fields (a seed's `CropId` is set and names a crop the Asset Manager knows). `HarvestItemId` on a crop is checked the same way.
- **Loading:** items have no meshes or textures yet, so a definition is a few hundred bytes. Which definitions are loaded, and when, is the inventory's decision; the rule from [CodingGuidelines.md, section 12](CodingGuidelines.md#12-performance) stands: an item that can enter the inventory during play (a harvest) has its definition loaded beforehand, never synchronously at that moment.

**Rules for the inventory:**

- **The inventory is a component on the player state,** so it outlives the pawn ([Architecture.md, section 2](Architecture.md#2-unreals-gameplay-framework-where-things-go)). `AVestigePlayerState`, in `Player/`, for now holds only `UVestigeInventoryComponent` (created in its constructor); the ability system component joins it with stamina ([section 9](#9-abilities-attributes-and-effects-gameplay-ability-system)). `AVestigeGameMode` sets it as the player state class, and `BP_PlayerState`, a Blueprint subclass in `Content/Vestige/Blueprints`, holds the values (the starting items), as the farmer Blueprint does for the farmer. `BP_ThirdPersonGameMode` names it.
- **The rules sit in a plain struct, `FVestigeInventory`,** as the tile's rules sit in `FFarmTileState` and the toolbar's in `FFarmToolbar`, so they are tested without a world. The component wraps it with loading, events and saving. Both live in the layer folder `Inventory/`, between `Time/` and `Farming/`: the farming component uses the inventory, and the inventory knows nothing about farming. Nothing below `Player/` knows `AVestigePlayerState`; the farming component and the save subsystem find the inventory with `FindComponentByClass` on the player's `APlayerState`, the same way the save subsystem finds the farming component on the pawn.
- **A fixed number of slots** (`SlotCount`, 24, on the component's defaults). A slot is empty or holds one item definition and a count from 1 to the item's `MaxStackSize`. A bigger bag later raises the count.
- **Adding and removing are all or nothing.** `TryAddItem(ItemId, Count)` first tops up stacks of the same item in slot order, then fills empty slots in order, and changes nothing if the whole count doesn't fit. `TryRemoveItem(ItemId, Count)` takes from the last stacks of that item first and changes nothing if there aren't enough. `GetItemCount(ItemId)` sums the stacks, and `CanAddItem` asks without changing anything. Callers that turn one thing into another (a ripe crop into items) ask first, so a full bag leaves the crop in the ground instead of destroying it. Partial adds wait for a caller that needs them (picking up a pile).
- **The API takes item IDs** (`FPrimaryAssetId` of type `VestigeItem`), the form content uses for references (`HarvestItemId`). An ID that names no item is refused with a warning.
- **Loading:** at `BeginPlay` the component loads every `VestigeItem` definition the Asset Manager knows, without bundles, and keeps them in a map by ID, so adding an item during play never waits on a load ([CodingGuidelines.md, section 12](CodingGuidelines.md#12-performance)). Definitions have no meshes or textures (an icon will be in the `Visuals` bundle, loaded by the inventory screen), so all of them together stay small. Revisit, with a measurement, if items ever get heavy or number in the thousands.
- **Starting items** (`StartingItems`, a list of `FVestigeItemStack` item IDs and counts on the component, 10 `Base.TurnipSeeds` on `BP_PlayerState`) are added at `BeginPlay`. Loading a save replaces them.
- **One event, `OnInventoryChanged`** (`FOnVestigeInventoryChanged`, a C++ multicast delegate with no parameters), broadcast after every change, including a restore. The HUD listens to it on the player state, not on the pawn.
- **Seeds and the harvest are items.** `UFarmingComponent` keeps the tools, the targeting and using a tool; it has no counts of its own. Its `SeedItemId` (limited to `VestigeItem`, `Base.TurnipSeeds` on `BP_PlayerCharacter`) names the seed the Seeds tool plants: at `BeginPlay` it loads the crop that item's `UFarmSeedFragment` names, with its visuals. Planting removes one of that item; harvesting adds the crop's `Yield` of its `HarvestItemId`, and if the bag can't take them, the crop stays and the player is told ("No room in your bag."). The farming component asks its pawn for the player state on each action, never at `BeginPlay`. Choosing between several kinds of seed is the later hotbar item ([Roadmap.md, section 4](Roadmap.md#4-milestone-2-vertical-slice)), which decides how their crops are loaded.
- **The toolbar stays on the farming component** (decision log, 2026-10-29). Tools become items, with a tool fragment, when the hotbar joins the inventory.
- **Money is not built yet.** Nothing earns or spends it until the shipping bin and shops. It arrives with the first of them, as its own decision (a count on the inventory component or a small wallet component on the player state), rather than as an unused field now.
- **Saving:** the component returns an `FVestigeInventorySaveData` (in `Inventory/`) holding one entry per non-empty slot: the slot index, the item's `ContentId` and the count. Restoring clears the slots and puts each entry back in its slot. An entry naming an unknown item is skipped with a warning and counted as missing content. A count above the item's stack size (its `MaxStackSize` was lowered, or a mod changed it) fills its slot and the rest goes in as a normal add; what doesn't fit is dropped with a warning and counted as missing. A bad slot index goes in as a normal add.
- **The HUD** shows the non-empty slots as "Bag: Turnip Seeds 9   Turnip 2", by display name, rebuilt on `OnInventoryChanged`, until the UMG inventory screen replaces the line.
- **Performance:** no tick. Adding, removing and counting walk the 24 slots, only on a player action. Loading the definitions has a `TRACE_CPUPROFILER_EVENT_SCOPE`.
- **Tests** (`Vestige.Inventory.*`): stacking into existing stacks before empty slots, the stack size limit, all-or-nothing adds and removes, counts across stacks, unknown IDs, the change event, the save round trip with unknown items, lowered stack sizes, bad slot indexes and no room left, and `ThePlayerStartsWithTurnipSeeds`, which checks that the game mode's player state class has an inventory with the starting seeds, so a missing editor step fails a test. The farming tests (`Vestige.Farming`) plant from a seed item, plant nothing without seeds or a player state, harvest the crop's item, and leave the crop when the bag is full.

**Pitfalls:**

- A field on `UVestigeItemData` that only some items use (a crop ID, a stamina value). It belongs in a fragment.
- Two fragments of the same class on one item: `FindFragment` would only ever see the first. Validation rejects it.
- One `UObject` per item in the inventory when a stack (ID and count) would do; instances only for items with their own state.
- Skip Lyra's replicated `FFastArraySerializer` list until multiplayer is real.
- Caching the inventory or the player state on the pawn at `BeginPlay`: a pawn spawned later begins play before it is possessed and has no player state yet. Ask the pawn for its player state when needed (an action), or follow possession.
- Reading item definitions by loading them at the moment an item is added. Every definition is loaded at `BeginPlay`.

## 9. Abilities, attributes and effects (Gameplay Ability System)

**Status:** adopted 2026-10-29, starting with stamina (milestone 2, [Roadmap 4.3](Roadmap.md#43-stamina-and-late-nights)). David approved the stamina design below on 2026-10-29. Built: the setup, the stamina set, tool use as an ability with its cost and the slower uses at zero, the refill on sleep, the HUD line and saving (2026-11-01), the red HUD line at zero, and the late-night rules: the drain after midnight, exhaustion and the end of the pass-out at 02:00 (2026-11-02, agreed with David). Health and combat (milestone 3) build on the same setup.

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

**Native tags** are declared in `AbilitySystem/VestigeGameplayTags.h` (`VestigeGameplayTags::Ability_UseTool`, `Ability_Interact`, `Data_StaminaCost`, `Data_StaminaDrain`, `Data_StaminaRefill`, `State_OutOfStamina`, `State_Exhausted`), so code never builds a tag from a string. Tags only content uses stay in `DefaultGameplayTags.ini` ([section 3](#3-gameplay-tags)).

**First use: stamina.** The rules in [GameDesign.md, section 5](GameDesign.md#5-time-days-and-seasons) mapped onto the system:

| Rule | Built as |
|---|---|
| The player has stamina, shown on the HUD | Built. `UVestigeStaminaSet` with `Stamina` and `MaxStamina` (100 at the start, a named constant until characters get starting stats from data), on the ability system component of `AVestigePlayerState`. The HUD listens to both attributes' change delegates and shows "Stamina 85 / 100", in red while the ability system holds `Vestige.State.OutOfStamina`: the HUD follows the tag (`RegisterGameplayTagEvent`) rather than the number, so the set alone decides when stamina has run out, and it recolours the line only when the tag changes |
| Tool actions cost stamina | Built. Using a tool is a gameplay ability (`UVestigeGameplayAbility_UseTool`). Its cost is `UVestigeStaminaCostEffect`, whose magnitude is the tool's `StaminaCost`, passed as a set-by-caller value tagged `Vestige.Data.StaminaCost`. A use that changes nothing on the farm (no target, nothing to water) costs nothing |
| Stamina never goes below zero; at zero, actions still work but take twice as long | Built. The set clamps `Stamina` to 0..`MaxStamina`. While it is 0, the set holds `Vestige.State.OutOfStamina` on the ability system as a loose tag, and tells the player once when it runs out. Each use takes the tool's `UseSeconds`, `OutOfStaminaTimeScale` (2) times as long while the tag is held; the ability stays active that long, so the next use can't start. Costs are still applied, and clamp at 0. The ability's `CheckCost` always passes, so a cost never blocks an action |
| Stamina drains faster after midnight | Built. There is no daytime drain, so this is a drain that starts at midnight: `UVestigeStaminaDrainEffect`, an infinite periodic effect (one drain per real second, `PeriodSeconds`) whose amount per period is a set-by-caller value tagged `Vestige.Data.StaminaDrain`. The rest component applies it when the day cycle's `OnHourStarted` reaches its `LateNightHour` (24) and removes it just before the next day starts. It turns its `LateNightDrainPerGameHour` (10) into an amount per period with the clock's `RealSecondsPerGameHour`, so a faster clock doesn't change the drain per game hour |
| Going to bed late makes the player exhausted: next day at 75%, one level, cleared by a night in bed before midnight | Built. A night that ends at `LateNightHour` or later (sleeping then, or staying up until the day ends at 06:00) applies `UVestigeExhaustedEffect`, an infinite effect with no modifiers that grants `Vestige.State.Exhausted` through a target tags component, unless the player is already exhausted. The refill is then `ExhaustedRefillFraction` (0.75) of `MaxStamina`. A night that ends earlier refills fully and removes the effect. The HUD shows "Exhausted" after the stamina while the tag is held |
| Sleep refills stamina | Built. `UVestigeStaminaRefillEffect`, an instant effect that overrides `Stamina` with a set-by-caller amount tagged `Vestige.Data.StaminaRefill`: the rest component sets it to `MaxStamina` times the night's refill share and applies it just before it starts the next day, so it is saved state changed synchronously ([Architecture.md, section 15](Architecture.md#15-the-day-flow)). The set clamps the result |

Who owns what: `AVestigePlayerState` owns the ability system component and grants its ability sets in `PostInitializeComponents`, so the stamina set exists before any actor's `BeginPlay`; it makes the pawn the avatar when `OnPawnSet` fires, and cancels running abilities when the pawn changes. The pawn (`AVestigePlayerCharacter`) implements `IAbilitySystemInterface` and returns the player state's component, so lower layers find it with `UAbilitySystemGlobals::GetAbilitySystemComponentFromActor` without knowing the player state class. `UVestigeStaminaSet` owns the numbers, their limits and the out-of-stamina tag. The farming component keeps choosing the tile and the tool and applying the tool's rules (`UseToolOnTarget`, which returns whether the farm changed); its `UseTool` only activates the abilities tagged `Vestige.Ability.UseTool`. The use-tool ability owns the cost and how long a use takes, and calls `UseToolOnTarget`. `UVestigeRestComponent` on the pawn owns sleep, the late-night rules (the warnings at 22:00 and midnight, the drain), exhaustion and the refill, and its settings hold their numbers; the effects it applies live on the player state's ability system, so it removes the drain in its `EndPlay`. Abilities and the stamina set are granted from a `UVestigeAbilitySet` data asset (`Content/Vestige/Abilities/DA_AbilitySet_Player`, named on `BP_PlayerState`): a plain list of attribute set classes and ability classes, without Lyra's grant handles until something needs to take a set away. Saving stores the base value of `Stamina` (`FVestigeStaminaSaveData`) and restores it through the set's restore function, which sets the base value without replaying effects. Whether the player is exhausted is saved by the rest component (`FVestigeRestSaveData`), which applies the effect again on load. The drain needs no saving: saves are made at the start of a day, before midnight. Tests cover the clamp, the cost, a use that costs nothing, one use at a time, the zero-stamina tag and slowdown, the refill on sleep and the save, and the late-night rules: the drain starting at midnight with its rate per game hour and stopping on the new day, the warnings, exhaustion after a late night or a night awake, its one level, an early night clearing it, and exhaustion in the save.

Built differently from the approved design, decided on 2026-11-01 (decision log):

- **Effects are C++ classes, not `GE_` Blueprint assets.** Their shape is fixed (which attribute, which operation) and every magnitude comes from data: the cost from the tool's `StaminaCost`, the refill from `MaxStamina`. In C++ they show in code review and tests, and the agents that build this project can't author gameplay effect modifiers with editor scripting. An effect with a number of its own to tune (the late-night drain rate, the 75% exhausted refill) gets that number from data too: a set-by-caller value from a settings or data asset, or a Blueprint subclass of a C++ effect.
- **The out-of-stamina tag is a loose tag the set keeps in step with `Stamina`, not an infinite effect.** It is derived from the number, so it can't drift from it, needs nothing in the save and no effect asset.
- **The refill's magnitude is an amount set by the caller** (2026-11-02), not `MaxStamina` read by the effect: an attribute-based magnitude's coefficient can't come from the caller, so a refill to 75% would need a second effect or a custom calculation class. The rest component multiplies `MaxStamina` by the night's share.
- **A tool use takes time.** "Twice as long" needs a duration, and there are no animations yet, so the tool's `UseSeconds` is how long the ability stays active. The ability is instanced per actor, so a use creates no objects, and its wait is a timer on that instance rather than an ability task, which would be a new object per use. With animations, the duration becomes the montage's length and the play rate halves at zero stamina.

**Pitfalls:**

- Building more of the system than stamina needs. Start with the stamina set, the use-tool ability, the effects above and one ability set; add Lyra's extras (custom ability system component subclasses, activation groups, ability cost classes) only when a second use needs them.
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

- Follow Lyra's split: `UVestigeSettingsLocal` (a `UGameUserSettings` subclass, set in `DefaultEngine.ini` as the game user settings class) for graphics, scalability, resolution and audio device; `UVestigeSettingsShared` (a `USaveGame`) for preferences like subtitles, survival needs on or off ([GameDesign.md](GameDesign.md)), key bindings and volume.
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
- Set up the Localization Dashboard's gather once there is a menu, so missing text shows up early.

**Pitfalls:**

- `FText::FromString` on literal text, which makes it untranslatable.
- Text baked into textures.

## 19. Debugging and cheats

**Status:** built (2026-11-04, [Roadmap 4.0](Roadmap.md#40-structure-from-the-code-review)): `UVestigeCheatManager` (`Player/`) with the console commands `NewGame` (deletes both saves and reopens the map on day 1), `SkipDay` (the player sleeps where they stand), `SkipToTime <Time>` (the clock runs forward to 19:30 or 19.5 today, broadcasting the hours it passes; added 2026-11-06) and `BenchmarkField <Size> <CropId>` (a Size by Size field of that crop, ripe, for the performance baseline in [Architecture.md, section 10](Architecture.md#measuring)). They replace the test keys F9 and N, which are no longer bound. No console variables or debug drawing yet. The target tile highlight is a shipping component, not debug drawing.

**References:** Epic's `UCheatManager` (console commands in development builds), console variables (`TAutoConsoleVariable`), the Gameplay Debugger and Visual Logger; Lyra's `ULyraCheatManager`.

**Rules:**

- Developer commands (add seeds, skip to a day, grow every crop) live in a `UVestigeCheatManager` (set as the player controller's cheat manager class) as `UFUNCTION(Exec)` functions. Cheat managers aren't created in shipping builds: `APlayerController::AddCheats` is compiled out there, and our controller sets `CheatClass` only under `UE_WITH_CHEAT_MANAGER`, the engine's own guard. Outside Shipping the engine creates one when the game mode's `AllowCheats` agrees (a standalone game, Play in Editor included).
- A command asks the owner of the action to do it (`SkipDay` calls the pawn's rest component's `Sleep`, as the bed does) and has no rules of its own, so a cheat does what the game does. A command that can't run (no pawn, no save subsystem) logs a warning and does nothing.
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
- A main menu is its own small map with its own game mode, so it loads nothing from the farm.
- A loading screen covers map changes and stays up until the systems that load content at start (the toolbar, crops, the save) report ready, as Lyra's loading screen waits on its "loading processors".
- Before any of this, the save subsystem split and the game-instance rules in [Architecture.md, sections 2 and 6](Architecture.md#6-saving) apply.

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
  | `LogFarm` | The farm: grid, tiles, crops, tools | `Farming/FarmLog.h` |
  | `LogVestigeSave` | Saving and loading | `Save/VestigeSaveLog.h` |

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
  [2026.11.02-19.56.43:858][647]LogFarm: Warning: [BP_PlayerCharacter_C_0.Farming] UFarmingComponent::RestoreCurrentTool: The saved tool SomeMod.Scythe isn't on the toolbar, so the first slot is in hand.
  ```

  | Part | Written by | What it is |
  | --- | --- | --- |
  | `[2026.11.02-19.56.43:858]` | The engine | When: date, then time to the millisecond, in UTC (the engine's default, `LogTimes=True` in `BaseEngine.ini`), so an hour behind Swedish winter time |
  | `[647]` | The engine | The frame the line was written in, counted modulo 1000, so it wraps from 999 to 0. Lines with the same number come from the same frame; the gap between two numbers is how many frames passed |
  | `LogFarm:` | The engine | The category: which system wrote it |
  | `Warning:` | The engine | The verbosity. Left out for `Log` lines |
  | `[BP_PlayerCharacter_C_0.Farming]` | `VESTIGE_LOG` | The sender: the object that wrote the line (here the `Farming` component on the player character), the level's world for code that has no object (`[L_Farm]`), or `None` |
  | `UFarmingComponent::RestoreCurrentTool:` | `VESTIGE_LOG` | The C++ function the line comes from, with its class |
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
| `AFarmGrid` | Actor placed in the level, as `BP_FarmGrid` | Where its field lies (the actor's corner, `TileSize`), finding its field by `FieldId`, converting between world space and coordinates, a tile's transform in the world (`GetTileTransform`), `OnFieldReady` once it has its field, resizing for developer tools. No tile actors and no drawing |
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
