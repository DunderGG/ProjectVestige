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

---

## 1. Saving and loading

**Status:** built (`UVestigeSaveSubsystem`, `UVestigeSaveGame`, and each owner's save struct next to the owner), with the safety rules in [Architecture.md, section 6](Architecture.md#6-saving): save version 2 with upgrades from versions 0 and 1, owners saving their own parts, tiles by coordinates with crop IDs, two alternating slots, and a save that can't be read left untouched with saving off. The decision of what to load (`UVestigeSaveSubsystem::DecideLoad`) is a static function, so its rules are tested without touching the player's save files.

**References:** Epic's "Saving and Loading Your Game" (`USaveGame`, `UGameplayStatics::SaveGameToSlot` and `AsyncSaveGameToSlot`); `UPROPERTY(SaveGame)` with `FObjectAndNameAsStringProxyArchive` (`ArIsSaveGame = true`) for serializing marked properties; `FCustomVersion` for versioned binary data. Lyra only saves settings (`ULyraSettingsShared`, a `USaveGame` written with `AsyncSaveGameToSlot`), not the world, so this guide relies on Epic's documentation more than on Lyra.

**Rules:**

- **Each owner saves itself.** An owner returns a plain `USTRUCT` of its saved state (`FFarmTileSaveData`) and restores from one. The save subsystem only collects, versions and writes ([Architecture.md, section 6](Architecture.md#6-saving)).
- **Save data is values and IDs:** numbers, enums, `FName` content IDs, grid coordinates. Never object pointers, asset paths, array positions or display text.
- **A save version** (`int32 SaveVersion` on `UVestigeSaveGame`, raised with every layout change) and one migration function per old version, each tested.
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

**Status:** in use for seasons (`Vestige.Season.*` in `Config/DefaultGameplayTags.ini`).

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

**Status:** day and clock built (`AVestigeDayCycle`). Seasons and weather are milestone 2 and 3.

**References:** no single Epic system; the rules here follow [Architecture.md, sections 15 and 18](Architecture.md#15-the-day-flow). Lighting follows Epic's "Sky Atmosphere", "Exponential Height Fog" and "Lumen" pages.

**Rules:**

- One owner of game time (`AVestigeDayCycle` today). Everything else reads it or listens to its events (`OnNewDay`, `OnNewDayFinished`, `OnDayEndReached`). Nothing keeps its own clock.
- Game time is separate from real time and from `UWorld::GetTimeSeconds`. Pausing and menus stop game time.
- Daily changes happen in `OnNewDay` listeners that finish synchronously ([Architecture.md, section 15](Architecture.md#15-the-day-flow)).
- Seasons are gameplay tags and change on a day event; weather is chosen once per day from data (per-season weights), so it's saved as a value, not re-rolled on load.
- Lighting, fog and sky react to time and weather through the day cycle; nothing else moves the sun.

**Pitfalls:**

- Timers (`FTimerManager`) for game-time events: they run on real time and don't save. Schedule game-time events against the clock.
- Comparing hours as floats for equality; compare against ranges or minute counts.

## 5. Input

**Status:** Enhanced Input, with farming actions and their mapping context created once in code (`AVestigePlayerCharacter`). The HUD's key hints are read from that mapping context. Moving to assets is a milestone 2 decision.

**References:** Epic's "Enhanced Input" pages (input actions, mapping contexts, triggers, modifiers, player-mappable key settings); Lyra's `ULyraInputConfig` (a data asset mapping input actions to input tags), `ULyraInputComponent` and `ULyraHeroComponent`, which bind actions by tag to abilities.

**Rules:**

- Input actions (`IA_`) and mapping contexts (`IMC_`) become assets once there are more than a handful of keys or once players can rebind keys. Mapping contexts are added and removed by context (on foot, in a menu, driving a machine) with explicit priorities.
- The character binds input to calls on the components that own the action (`UFarmingComponent::UseTool`); input code holds no game rules. Movement input also aims the farming component at a neighbouring tile (`AimAt`), and the walk eases in from the press, read from the move action's elapsed time (`FInputActionInstance::GetElapsedTime`), so the template's `IA_Move` needs no extra trigger or modifier. Turning belongs to the farmer's movement component, `UVestigeCharacterMovementComponent`, which turns toward the aimed tile when there is no input.
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

**Status:** planned (`IVestigeInteractable`, milestone 2 foundations).

**References:** Epic's "Interfaces" page; Lyra's interaction: `IInteractableTarget` returns `FInteractionOption`s, an ability task (`UAbilityTask_GrantNearbyInteraction`) scans nearby targets, and `ULyraGameplayAbility_Interact` runs the chosen option.

**Rules:**

- Anything the player can use (bed, shipping bin, water source, chest, robot) implements one interface, `IVestigeInteractable`, with functions such as `GetInteractionOptions` (what can be done, with display text) and `Interact`.
- The farmer finds candidates from a **cheap query on a timer or on movement**, not every frame, and only among nearby actors (an overlap sphere on a dedicated collision channel). The best candidate is shown by a presentation component, not by debug drawing ([ArchitectureAudit A-10](ArchitectureAudit.md)).
- Farm tiles stay targeted through the grid lookup; they don't need to be interactable actors.
- The interacted object owns what happens; the farmer only asks.
- When the Gameplay Ability System arrives, interaction becomes an ability as in Lyra, so it can be blocked by tags (stunned, in a menu).

**Pitfalls:**

- One `if` per interactable class in the farmer's code: the reason the interface exists.

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
- **Save version 2** adds the inventory to `UVestigeSaveGame`. The upgrade from version 1 turns `Farming.Seeds` into `Base.TurnipSeeds` in slot 0 and `Farming.HarvestedCrops` into `Base.Turnip` in slot 1 (the turnip was the only crop and seed until now), as the version 0 upgrade names `Base.Turnip`. `FFarmFarmingSaveData` keeps the two counts, marked as read only by the upgrade, so version 1 saves still read into them. A version 0 save goes through both steps.
- **The HUD** shows the non-empty slots as "Bag: Turnip Seeds 9   Turnip 2", by display name, rebuilt on `OnInventoryChanged`, until the UMG inventory screen replaces the line.
- **Performance:** no tick. Adding, removing and counting walk the 24 slots, only on a player action. Loading the definitions has a `TRACE_CPUPROFILER_EVENT_SCOPE`.
- **Tests** (`Vestige.Inventory.*`): stacking into existing stacks before empty slots, the stack size limit, all-or-nothing adds and removes, counts across stacks, unknown IDs, the change event, the save round trip with unknown items, lowered stack sizes, bad slot indexes and no room left, and `ThePlayerStartsWithTurnipSeeds`, which checks that the game mode's player state class has an inventory with the starting seeds, so a missing editor step fails a test. The version 1 upgrade is in `Vestige.Save`; the farming tests (`Vestige.Farming`) plant from a seed item, plant nothing without seeds or a player state, harvest the crop's item, and leave the crop when the bag is full.

**Pitfalls:**

- A field on `UVestigeItemData` that only some items use (a crop ID, a stamina value). It belongs in a fragment.
- Two fragments of the same class on one item: `FindFragment` would only ever see the first. Validation rejects it.
- One `UObject` per item in the inventory when a stack (ID and count) would do; instances only for items with their own state.
- Skip Lyra's replicated `FFastArraySerializer` list until multiplayer is real.
- Caching the inventory or the player state on the pawn at `BeginPlay`: a pawn spawned later begins play before it is possessed and has no player state yet. Ask the pawn for its player state when needed (an action), or follow possession.
- Reading item definitions by loading them at the moment an item is added. Every definition is loaded at `BeginPlay`.

## 9. Abilities, attributes and effects (Gameplay Ability System)

**Status:** adopted 2026-10-29, starting with stamina (milestone 2, [Roadmap 4.3](Roadmap.md#43-stamina-and-late-nights)); not built yet. `UVestigeRestComponent` exists and owns sleep and passing out, without ability system parts so far. Health and combat (milestone 3) build on the same setup. David approved the stamina design below on 2026-10-29.

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

**Setup:** enable the Gameplay Abilities plugin in `Vestige.uproject`, and add `GameplayAbilities` and `GameplayTasks` to `Vestige.Build.cs` (`GameplayTags` is already there). Lyra calls `UAbilitySystemGlobals::Get().InitGlobalData()` once at startup, from its asset manager's `StartInitialLoading`; check during setup whether Unreal Engine 5.8 still needs that call, and record the answer here.

**First use: stamina.** The rules in [GameDesign.md, section 5](GameDesign.md#5-time-days-and-seasons) mapped onto the system:

| Rule | Built as |
|---|---|
| The player has stamina, shown on the HUD | `UVestigeStaminaSet` with `Stamina` and `MaxStamina`, on the ability system component of a new `AVestigePlayerState`. The HUD listens to the attribute's change delegate |
| Tool actions cost stamina | Using a tool becomes a gameplay ability (`UVestigeGameplayAbility_UseTool`). Its cost is a gameplay effect whose magnitude is the tool's `StaminaCost`, passed as a set-by-caller value tagged `Vestige.Data.StaminaCost` |
| Stamina never goes below zero; at zero, actions still work but take twice as long | The set clamps `Stamina` to 0..`MaxStamina`. While it is 0, an infinite effect grants `Vestige.State.OutOfStamina`, and tool abilities read that tag to double their duration. Costs are still applied, and clamp at 0 |
| Stamina drains faster after midnight | An infinite periodic effect (`GE_LateNightDrain`) applied at midnight and removed on the new day, by the owner of the late-night rules |
| Going to bed late makes the player exhausted: next day at 75%, one level, cleared by a night in bed before midnight | An `Exhausted` effect granting `Vestige.State.Exhausted`, which never stacks. The new-day refill applies 75% instead of 100% while it is active, and an early night removes it |
| Sleep refills stamina | An instant effect applied in the new-day flow, so it is saved state changed synchronously ([Architecture.md, section 15](Architecture.md#15-the-day-flow)) |

Who owns what: `AVestigePlayerState` owns the ability system component; `UVestigeStaminaSet` owns the numbers and their limits; the farming component keeps choosing the tile and the tool and activates the tool ability; a small `UVestigeRestComponent` on the farmer owns sleep, the late-night rules and exhaustion (moving those out of `UFarmingComponent`, [ArchitectureAudit A-13](ArchitectureAudit.md)). Abilities and the stamina set are granted from a `UFarmAbilitySet` data asset. Effects are Blueprint assets (`GE_`) under `Content/Vestige/Abilities/`, with every magnitude in data. Saving stores `Stamina` and whether `Exhausted` is active, and restores them without replaying effects. Tests cover the clamp, the cost, the zero-stamina tag and the exhausted refill.

**Pitfalls:**

- Building more of the system than stamina needs. Start with the stamina set, the use-tool ability, the effects above and one ability set; add Lyra's extras (custom ability system component subclasses, activation groups, ability cost classes) only when a second use needs them.
- Abilities that tick or poll. Use ability tasks and attribute change delegates.
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

**Status:** F9 starts a new game; no cheat system. The target tile highlight is a shipping component, not debug drawing.

**References:** Epic's `UCheatManager` (console commands in development builds), console variables (`TAutoConsoleVariable`), the Gameplay Debugger and Visual Logger; Lyra's `ULyraCheatManager`.

**Rules:**

- Developer commands (add seeds, skip to a day, grow every crop) live in a `UVestigeCheatManager` (set as the player controller's cheat manager class) as `UFUNCTION(Exec)` functions. Cheat managers aren't created in shipping builds.
- Debug visuals sit behind console variables (`farm.Debug.Targeting 1`) and `#if ENABLE_DRAW_DEBUG`, never always on.
- Debug keys like F9 move into cheats or menus before a public build.
- Robots get Gameplay Debugger categories and Visual Logger output when their AI is built.

**Pitfalls:**

- Debug-only behavior that players come to rely on ([ArchitectureAudit A-10](ArchitectureAudit.md)).

## 20. Maps, loading screens and the main menu

**Status:** one map (`L_Farm`), no menu, no loading screen ([Architecture.md, section 2](Architecture.md#2-unreals-gameplay-framework-where-things-go)).

**References:** Epic's "Level Streaming", "World Partition", "Level Instances" and "Data Layers" pages, and the MoviePlayer loading screen; Lyra's CommonLoadingScreen plugin (a loading screen shown during map loads and while systems report they're not ready) and its front-end map.

**Rules:**

- The farm, the town and the wasteland are separate maps or streamed sublevels; decide with a profile and log it before the town is built. World Partition is for large open worlds; small hand-made areas usually don't need it.
- A main menu is its own small map with its own game mode, so it loads nothing from the farm.
- A loading screen covers map changes and stays up until the systems that load content at start (the toolbar, crops, the save) report ready, as Lyra's loading screen waits on its "loading processors".
- Before any of this, the save subsystem split and the game-instance rules in [Architecture.md, sections 2 and 6](Architecture.md#6-saving) apply.

**Pitfalls:**

- A map that hard-references every other map's content through a game mode or a Blueprint.
