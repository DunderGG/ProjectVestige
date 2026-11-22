# 25. Selling and money

One of the [system guides](../Systems.md); read [How to use this document](../Systems.md#how-to-use-this-document) first.

**Status:** designed 2026-11-22 for [RoadmapArchive.md, 4.5](../RoadmapArchive.md#45-automation-and-selling) ("A shipping bin: crops put in it are sold overnight", "The currency ... on the HUD", the purse of "Starting seeds and a small purse for a new game", and the prices of the season budget), from [GameDesign.md, section 9](../GameDesign.md#economy) ("Selling at the shipping bin" and the season budget) and the recommended options of section 15 (the bin's use, the starting purse, no taking back, seeds don't sell), assumed until @DunderGG answers them. Built 2026-11-22; `SetUpShippingBin.py` has run, and `WBP_Hud`'s `MoneyText` (step 5 below) is a hand step. The seed shop, which spends money, comes after it and adds spending to the wallet.

**References:**

- Epic's "Programming Subsystems" page and `FSubsystemCollectionBase::InitializeDependency`: a game-instance subsystem outlives every map, so what is in the bin is there whichever map the night passes in, as the farm's fields and water are ([section 22](22-FarmFields.md), [section 24](24-Water.md)).
- Epic's "Gameplay Framework" pages (`APlayerState`): the player's data that outlives the pawn sits on the player state, as the inventory does ([Architecture.md, section 2](../Architecture.md#2-unreals-gameplay-framework-where-things-go)).
- Epic's "Text Localization" and "Text Formatting" pages: `FText::AsNumber` formats a number with the culture's grouping ("1,500" in English), and an integer argument in `FText::Format` is formatted the same way.
- Lyra (`Source/LyraGame`) has no currency, shop or sale. What we take from it is the pattern its inventory and quick bar already gave us ([section 8](08-Inventory.md)): a component on the player for what the player has, events without values that listeners read from, and a manager apart from the actor the player uses. The interaction guide ([section 7](07-Interaction.md)) and the well ([section 24](24-Water.md)) are the pattern for the bin as an interactable.
- The engine source and the Lyra project weren't on this machine when this guide was written (the architect's check, 2026-11-22), so the engine behaviour above rests on Epic's documentation and on the engine code read for the guides it cites; the test `TheMoneyLineShowsCoinsAndTheAmount` checks the grouping.

**What we take and what we skip:**

- **Take:** the money as a small component on the player state next to the inventory (Lyra keeps each aspect of the player in its own component); the shipment as farm data in a game-instance subsystem, apart from the bin actor (the bin is a place to put things, as the well is a place to draw from); events with no parameters, read on the call.
- **Skip:** money as an item in a slot or as a gameplay attribute (below, rejected); a shop or stock data (the seed shop's design); a screen of the bin's contents ([roadmap 4.6](../Roadmap.md#46-interface-and-input)); replication.

**How the parts fit:**

| Part | Kind and layer | Owns |
| --- | --- | --- |
| `UVestigeWalletComponent` | **New.** Component on the player state, `Inventory/` | The player's money: the amount, the starting purse, adding (and later spending), its event and its save part `Base.Wallet` |
| `UFarmShippingSubsystem` | **New.** Game-instance subsystem, `Farming/` | What is in the bin until the night (the shipment): putting the stack in hand into it, selling it at each item's sell price on the night, paying the wallet and returning the total, and its save part `Base.Shipping` |
| `AFarmShippingBin` | **New.** Actor (`BP_ShippingBin`, placed in `L_Farm`'s service yard), `Farming/` | Being the bin: offering "Ship" and shipping the stack in hand through the subsystem, telling the player how it went |
| `UVestigeItemData` | Existing, `Content/` | `SellPrice`, now read, and `CanBeSold()` (a sell price above 0) |
| `AVestigePlayerState` | Existing, `Player/` | Holds the wallet, as it holds the inventory |
| `AVestigeGameMode` | Existing, `Game/` | The order of the night's steps: the sale, then the day message with what the bin paid in it, then the next day |
| `AVestigeHUD` + `UVestigeHudWidget` | Existing, `UI/` | The money line, "Coins 250", under the stamina |
| `FVestigeFeedbackReport` + `UVestigeFeedbackSubsystem` | Existing, `Core/` | Clauses: a report can carry a clause (a tag and its values) for one of its text arguments, so "The bin paid 250." rides in the day message ([section 15](15-EffectsAndFeedback.md)) |
| `UVestigeFeedbackResponses` | Existing, `UI/` | Turning each clause into its argument's text when it formats a message |

**Rules for money:**

- **The money is `UVestigeWalletComponent`'s,** a component on `AVestigePlayerState` (`Inventory/VestigeWalletComponent.h`), created in the player state's constructor beside the inventory, with a getter `GetWallet()` on the player state. Code below `Player/` finds it with `FindComponentByClass<UVestigeWalletComponent>()` on the player's `APlayerState`, as it finds the inventory. It is not the inventory's: the inventory is slots and their rules, and a chest (an inventory too) has no purse.
- **One currency, a whole number.** `int32 Money`, never below 0 and never above `VestigeWallet::MaxMoney` (999,999,999, a named constant: far beyond any season, and below `int32`'s limit, so a sum of two amounts is checked in `int64` without overflow).
- **The interface:** `GetMoney()`; `AddMoney(int32 Amount)`, which refuses 0 or less (returns false, changes nothing), adds up to `MaxMoney` (the rest is lost with a warning; nothing in the game comes near it) and broadcasts; `OnMoneyChanged` (`FOnVestigeMoneyChanged`, a C++ multicast delegate without parameters, as `OnInventoryChanged`), broadcast after every change, including a new game and a restore, so listeners read `GetMoney()`. Spending (`TrySpendMoney(Amount)`, all or nothing) is added with the seed shop, its first caller, rather than unused now. Not exposed to Blueprints yet; a Blueprint mod's shop gets `BlueprintCallable` reads and spends with the first one ([Architecture.md, section 12](../Architecture.md#12-designing-for-mods)).
- **The starting purse** is `StartingMoney` on the component (`UPROPERTY(EditDefaultsOnly, Category = "Wallet", meta = (ClampMin = 0))`, 100 in C++). `BP_PlayerState` keeps the C++ default (a new property has no saved Blueprint value, so it inherits 100), and the purse is tuned there without a build, as the starting items are. A value above `MaxMoney` is clamped when used. The game mode's player state class is checked by a test, so a changed Blueprint value is seen.
- **Saving.** `IVestigeSaveable` with key `Base.Wallet`, version 1, `FVestigeWalletSaveData` (`Inventory/VestigeWalletSaveData.h`): `int32 Money`. It registers in `BeginPlay` and unregisters at the start of `EndPlay`, as the inventory does ([section 1](01-Saving.md)). No record (a new game, or a save from before coins) is `StartNewGame`: `Money = StartingMoney`, so an older save gets the starting purse ([GameDesign.md, section 9](../GameDesign.md#economy)) without a new version of `Base.Inventory`. Restoring clamps the saved amount to 0 to `MaxMoney` (a hand-edited or modded save), with a warning when it had to.
- **Logging:** `LogVestigeInventory`: an add at `Verbose`, a clamped add or restore at `Warning`.
- **Performance:** no tick; an add is an integer and a broadcast, once a night.

**Rules for what sells:**

- **An item sells when its `SellPrice` is above 0.** `UVestigeItemData::CanBeSold()` says so; there is no item category yet ([section 8](08-Inventory.md): category tags wait for a system that needs more than one rule). `SellPrice` is what one pays, read on the night of the sale, so a price changed in the data applies to what is already in the bin.
- **The slice's prices** ([GameDesign.md, section 6](../GameDesign.md#crops)) are data: `DA_Item_Turnip` 25, `DA_Item_Potato` 18, `DA_Item_Cabbage` 110. Seeds, the hoe and the watering can have `SellPrice` 0 and can't be shipped (section 15, seeds don't sell). The seed prices (10, 20, 40) are not item data: they go into the seed shop's stock data with the shop ([section 8](08-Inventory.md), prices belong to items; what a shop charges is the shop's).
- The tooltip of `SellPrice` changes from "Not used yet" to what it is: what the shipping bin pays for one, 0 for an item the bin won't take.

**Rules for the shipment:**

- **What is in the bin belongs to `UFarmShippingSubsystem`,** a game-instance subsystem in `Farming/`, not to the bin actor: the night can pass in a map without the bin (a house interior, later), the shipment must outlive the map, and a second bin or a robot that ships for the player later adds to the same shipment. It is a list of `FVestigeItemStack` (item ID and count), one entry per item, merged when the same item is shipped again.
- **Its interface:** `static Get(const UWorld*)`; `ShipFromSlot(UVestigeInventoryComponent& Inventory, int32 Slot)`, which returns an `FFarmShipResult` (`const UVestigeItemData* Item`, `int32 Count`); `GetShippedStacks()` (for tests and a later bin screen); `SellShipment(UVestigeWalletComponent& Wallet)`, which returns the total paid and reports nothing (the game mode tells the player, in the day message). `Initialize` takes `UVestigeSaveRegistry` and `UVestigeContentCatalog` with `InitializeDependency` and registers as a game owner; `Deinitialize` unregisters. It subscribes to nothing.
- **Shipping takes the whole stack in the slot, and only that slot.** `ShipFromSlot` reads the slot's item and count; an empty slot or an item that can't be sold returns `{nullptr, 0}` and changes nothing. Otherwise it removes the whole count with `TryRemoveItemFromSlot(Slot, Count)` and only then adds the stack to the shipment, so a refused removal ships nothing and nothing is counted twice. Another stack of the same item elsewhere in the bag stays (one stack per press, [GameDesign.md, section 9](../GameDesign.md#economy)). An item that doesn't stack loses its slot stats with the slot ([section 8](08-Inventory.md)); no sellable item has slot stats.
- **Nothing comes back out.** There is no take-back (section 15, option (a)); a bin screen with its contents may add one with the first menu.
- **The sale.** `SellShipment(Wallet)`, under `TRACE_CPUPROFILER_EVENT_SCOPE(UFarmShippingSubsystem::SellShipment)`: for each stack, the item from the content catalogue and `Count * SellPrice`, summed in `int64`; a stack whose item no longer exists or no longer sells (the data changed since it was shipped) pays nothing and is dropped with a warning. The sum is clamped to `VestigeWallet::MaxMoney` before it becomes the `int32` total, with a warning when it had to be. Then `Wallet.AddMoney(Total)` when the total is above 0, the shipment is emptied, and the total is returned. An empty bin returns 0 and changes nothing. It logs the sale at `Log` (`LogFarm`: the stacks and the total). It reports nothing, so it needs no world.
- **Saving.** Key `Base.Shipping`, version 1, `FFarmShippingSaveData` (`Farming/FarmShippingSaveData.h`): `Stacks`, one `FFarmShippedStackSaveData` (`ItemId`, the item's `ContentId` as an `FName`, and `Count`) per stack. Restoring skips an unknown item or a count below 1 with a warning and counts each as left out; two entries of the same item are merged. No record is a new game: an empty bin. The game saves only on the new day, after the sale, so the bin is usually empty in a save; it is saved anyway so that nothing in it is lost if a sale is skipped (below) or the game later saves at other moments (a menu, quitting). **Loading sells nothing** (restore sets state only, [section 1](01-Saving.md)); what was in the bin sells the next night.

**Rules for the night:**

- **The game mode runs the sale, as a step of the end of the day,** because the morning's messages follow the day's steps, never the order of subscription ([section 4](04-Time.md), the morning's messages). In `AVestigeGameMode::StartNextDay`, first: find the player's wallet (the first player controller's player state's `UVestigeWalletComponent`; single player, and a co-op rule is decided with co-op), and call `UFarmShippingSubsystem::SellShipment(*Wallet)`. Then the day report, with the sale's total in its `Paid` clause (the morning message, below), then `UVestigeClockSubsystem::StartNextDay`. So the morning reads: the day message ending "... 23 days left. The bin paid 250.", the withering line (in `OnSeasonStarted`, inside the clock's `StartNextDay`), waking exhausted or rested, "Game Saved!", and the coins are in the morning's save, made on `OnNewDayFinished`.
- **The morning message.** "The bin paid 250." is a clause at the end of the day message ([GameDesign.md, section 9](../GameDesign.md#economy)), so a morning with a sale has no extra message. Gameplay never writes the player's text ([section 15](15-EffectsAndFeedback.md)), so the game mode passes the total, not a sentence: every day report it makes (the six day tags: `Day.Slept`, `Day.StayedUp` and their `.NewSeason` and `.LastDay` forms) carries a feedback clause ([section 15](15-EffectsAndFeedback.md), clauses) named `Paid`: tag `Vestige.Feedback.Shipping.Paid` with `Amount` (the total) when the sale paid, and no tag when it didn't (nothing shipped, no wallet). Presentation turns the clause into the `{Paid}` text argument: the `Shipping.Paid` row's text, "The bin paid {Amount}.", or empty. The six day rows end with " {Paid}", and the formatted message is trimmed at the end, so a morning without a sale ends at "left." as before. "Welcome back." (`Save.Loaded`) has no clause, since a load sells nothing.
- **It runs every night, however the day ended:** `OnDayEnded` comes from the bed, `SkipDay`, and staying up until 06:00 alike. `JumpToDay` (a developer command) doesn't end a day and sells nothing. A new game and a load sell nothing.
- **Without a wallet the shipment waits.** If the game mode finds no wallet (no player state, a test world), it logs a warning (`LogFarm`, only when the bin holds something) and doesn't call the sale, so the stacks stay in the bin for the next night rather than being sold to nobody, and the day report's `Paid` clause is empty.
- **The game mode owns the order and the day message, not the rules:** prices and paying are the subsystem's. Its row in [Architecture.md, section 13](../Architecture.md#13-systems-and-ownership) says so.

**Rules for the bin:**

- **`AFarmShippingBin`** (`Farming/FarmShippingBin.h`, `UCLASS(Blueprintable)`, as `AFarmWell`): a `Root` scene component with `BinMesh` attached to it, a static mesh component with its default blocking collision and no mesh in C++ (`BP_ShippingBin` sets `SM_ShippingBin`), as `AFarmWell` has; and `InteractionVolume`, a box with the `VestigeInteractable` profile, overlap events off and `SetCanEverAffectNavigation(false)`, as the well's and the bed's.
- **Sized for `SM_ShippingBin`'s hull,** `UCX_SM_ShippingBin_00`, one solid box of 90 by 90 by 90 cm with its pivot on the floor at its centre (`Art/Blender/make_shipping_bin.py`). The box's half extents are 55, 55, 50 cm, at 50 cm up. The farmer's capsule (42 cm) keeps his centre at least 87 cm from the bin's centre against a face and 105.6 cm along a diagonal against a corner (63.6 + 42); the box reaches 55 cm on the axes (32 cm short of him) and 77.8 cm at its corners (27.8 cm short), so the facing ray never starts inside it ([section 7](07-Interaction.md), facing). The bin can be used facing any side, anywhere along it, but not at a corner: the aim has four directions, so the ray passes beside the hopper, and a box wide enough to catch it would contain the farmer's centre, which section 7 forbids (kept as it is; the play test asks whether it feels wrong). A change to the hull re-checks these numbers.
- **The option.** `GetInteractionOptions(Interactor)` adds one option, `{Text "Ship", Action "Ship"}` (`ShipAction`, a static `FName`), with no location (the facing ray picks the bin, as it picks the bed and the well), whenever the interactor has an inventory (`UVestigeInventoryComponent::FindForPawn`). It is offered whatever is in hand, so F with a tool, seeds or an empty hand tells the player why ([GameDesign.md, section 9](../GameDesign.md#economy)). The text is a `LOCTEXT` built once, in the constructor; the bin shows no state, so the prompt never changes.
- **Shipping** (`Interact` with the `Ship` action; any other action does nothing): find the inventory and its selected hotbar slot, then `UFarmShippingSubsystem::ShipFromSlot`. A result with a count reports `Vestige.Feedback.Shipping.Shipped` with `Item` (the item's display name) and `Count`; no count reports `Vestige.Feedback.Shipping.NothingToShip`. Instant and free: no stamina, no clock time. The interact ability ends at once, as for the well, and it doesn't repeat while F is held, so one press ships one stack.
- **Placement.** `BP_ShippingBin` (`Content/Vestige/Blueprints`, parent `AFarmShippingBin`, `BinMesh` set to `SM_ShippingBin` at scale 1 and location 0) stands in `L_Farm`'s service yard beside the well ([home farm plan](../Levels/HomeFarm/README.md): water and selling together, outside the field's apron): X 1880, Y 600, Z 0, yaw 180, so its gear plate (the mesh's +X) faces the field. That is 2.4 m from the well's centre: 1.5 m between the well's ring and the bin, room for the farmer (84 cm) to pass, and the two interaction boxes 1.3 m apart, so each is chosen by facing. The blockout may move both, keeping them together.

**Editor steps** (for whoever does them, after the C++ is built and the editor reopened; `SM_ShippingBin` must be imported first, by the artist, in `Content/Vestige/Meshes` as `Art/README.md` says). An editor script, `EditorScripts/SetUpShippingBin.py`, written like `SetUpWell.py` (idempotent, logs each fact, run headless with the editor closed, [section 23](23-EditorScripting.md)), does 1 to 4; 5 is by hand:

1. **Prices:** sets `SellPrice` on `DA_Item_Turnip` (25), `DA_Item_Potato` (18), `DA_Item_Cabbage` (110), and 0 on the three seed items, `DA_Item_Hoe` and `DA_Item_WateringCan`, and saves each it changed.
2. **Feedback rows** in `DA_FeedbackResponses`: the three tags below, with their texts and no sound, and " {Paid}" added at the end of the six day rows' texts (once: the script skips a row whose text already has `{Paid}`).
3. **`BP_ShippingBin`:** makes it if missing (parent `AFarmShippingBin`) and sets `BinMesh` to `SM_ShippingBin`; fails if the mesh isn't there.
4. **`L_Farm`:** places one `BP_ShippingBin` at X 1880, Y 600, Z 0, yaw 180 if the map has none, and saves the map.
5. **`WBP_Hud`, by hand** (Content Browser > `Content/Vestige/UI`, open `WBP_Hud`): in the status panel's vertical box, add a **Text** under `StaminaText`, name it `MoneyText`, tick **Is Variable**, font Roboto Regular 16 and padding 4 on top as `StaminaText` and `BagText` have, and Compile and Save. `MoneyText` is a `BindWidgetOptional` part, so the widget compiles before this is done; until then the HUD shows no money line and `Vestige.UI.TheHudWidgetHasTheMoneyLine` fails. @DunderGG does it right after the implementer's build and before the test author's run, so the suite passes before the commit.

**Feedback** ([section 15](15-EffectsAndFeedback.md), three new native tags, each a row in `DA_FeedbackResponses`):

| Tag | Reported by | Values | Text |
| --- | --- | --- | --- |
| `Vestige.Feedback.Shipping.Shipped` | the bin | `Item` (the display name), `Count` | Shipped: {Item} ×{Count}. The bin pays in the morning. |
| `Vestige.Feedback.Shipping.NothingToShip` | the bin | | Hold a crop to ship it. |
| `Vestige.Feedback.Shipping.Paid` | never alone: the game mode's `Paid` clause of the day report, when the night's sale paid | `Amount` | The bin paid {Amount}. |

The six day rows (`Day.Slept`, `Day.StayedUp`, and their `.NewSeason` and `.LastDay` forms) take `Paid` as one more value and end with " {Paid}": "You slept. Spring, day 5 begins. 23 days left. {Paid}".

**The HUD:**

- **The money line,** "Coins 250", on its own line under the stamina ([GameDesign.md, section 9](../GameDesign.md#economy)). `UVestigeHudWidget` gets the part `UTextBlock MoneyText` and `SetMoneyText(const FText&)`, which does nothing while the part is missing. `MoneyText` is the one `BindWidgetOptional` part (the others are `BindWidget`), with a comment saying why: it was added to the C++ after `WBP_Hud`'s layout, so the widget compiles before a person adds it, and `TheHudWidgetHasTheMoneyLine` fails until they do. `AVestigeHUD` follows the wallet on the player state once at `BeginPlay`, as it follows the inventory (`FollowWallet`, a weak pointer, unbound in `EndPlay`), binds `OnMoneyChanged` to `RefreshMoneyText`, and refreshes once when it starts following, so the line shows the purse from the first frame whichever of the HUD and the player state begins play first. The text is built only on the event, by the public static `FormatMoney(int32 Money)`: `LOCTEXT "Coins {Money}"` with `FText::AsNumber`, so 1500 reads "Coins 1,500". "Coins" is the working name until the writer names the currency (a text change in the HUD, no rule changes); the layout pass ([roadmap 4.6](../Roadmap.md#46-interface-and-input)) may move the line.
- Without a wallet (a test world) the line is empty.

**Layers.** The wallet is in `Inventory/` (what the player has; it knows no farm). The subsystem and the bin are in `Farming/`, which may include `Inventory/` and `Content/`. The game mode (`Game/`) includes the subsystem and the wallet; the HUD (`UI/`) the wallet. No new folder, so the layer table of `Vestige.Architecture.IncludeDirection` doesn't change.

**Performance.** No tick and no timer. The bin's options cost one component lookup five times a second while the farmer is near it, and no text is built. Shipping is a slot read, a removal and a short list on a press. The sale walks the shipment (a few stacks) once a night, with its trace scope. The HUD rebuilds the money line once a night and on a new game or load.

**Mods.** A mod's item sells by having a `SellPrice` above 0. A Blueprint mod's bin is a Blueprint subclass of `AFarmShippingBin` (another mesh, another box) with the same rules. Shipping or selling from Blueprints (a robot that ships, a mod's shop) needs the subsystem's and the wallet's functions exposed, which happens with the first such caller ([Architecture.md, section 12](../Architecture.md#12-designing-for-mods)).

**Rejected:**

- Money as a count on `UVestigeInventoryComponent`: the inventory is slots, and a chest reuses it; old saves would need `Base.Inventory` version 3 to get the purse, where a new key gets it from "no record".
- Money as an item in a slot (coins): it takes a bag slot, has a stack limit, and could be shipped.
- Money as a gameplay attribute: a float changed only through effects, for a number nothing modifies or predicts.
- Money in a game-instance subsystem: it is the player's, as the inventory is; the player state already outlives the pawn and the map's teardown saves it.
- The shipment on the bin actor: it would die with its map and need saving per placed bin.
- The sale in an `OnDayEnded` or `OnNewDay` handler of the subsystem: a multicast calls listeners in no guaranteed order, so the total might not be known when the game mode reports the day message, and `OnNewDay` comes after it.
- "The bin paid N." as a message of its own after the day message: as cheap to build (the subsystem would report it), but a sixth message on a morning that can already show five, and the design asks for a clause.
- The game mode passing the clause as a finished `FText`: gameplay code would write the player's text; the clause passes the total and presentation picks the words.
- Twin day tags with the clause: twelve rows to keep in step, where one argument does it.
- A category tag for sellable items: one rule (a price) covers the slice; tags come with gifts or donations.
- Shipping every sellable stack in the bag at once (section 15, option (b)): the design keeps the choice of what to sell.

**Tests** (`Source/Vestige/Tests/`):

- `Vestige.Wallet.*`: `ANewGameStartsWithTheStartingPurse`, `ASaveFromBeforeCoinsStartsWithThePurse` (no `Base.Wallet` record), `MoneyIsSavedAndRestored`, `ARestoredAmountIsClampedToTheLimits` (negative and above `MaxMoney`), `AddingMoneyBroadcasts`, `AnAddOfZeroOrLessChangesNothing`, `MoneyStopsAtTheMaximum`, `ThePlayerStartsWithAPurseOf100` (the game mode's player state class, so `BP_PlayerState`'s value is checked).
- `Vestige.Shipping.*` (the subsystem on a test game instance, the bin with the Blueprint farmer as the well's tests do): `ShippingTakesTheWholeStackInHand`, `ShippingLeavesOtherStacksOfTheSameItem`, `ShippingTheSameItemTwiceMergesIt`, `SeedsAToolAndAnEmptyHandShipNothing`, `TheBinOffersShipToAnInteractorWithABag`, `TheBinOffersNothingWithoutABag`, `ShippingReportsTheItemAndTheCount`, `NothingSellableReportsHoldACrop`, `AnotherActionAtTheBinDoesNothing`, `TheSalePaysEachItemsPrice` (10 turnips and 3 cabbages pay 580), `TheSalePaysThePriceOnTheNight` (a test item's price changed after shipping), `AStackThatNoLongerSellsIsDroppedAndPaysNothing`, `TheSaleEmptiesTheBinAndReturnsTheTotal`, `AnEmptyBinPaysNothing`, `ATotalAboveTheLimitIsClamped`, `TheShipmentIsSavedAndRestored`, `AnUnknownShippedItemIsLeftOutAndCounted`, `ANewGameEmptiesTheBin`, `LoadingSellsNothing`, `AFarmerAtTheBinFacingItFocusesIt` (against a face, and not at a corner), `TheBinBlueprintHasItsMesh`.
- `Vestige.Shipping` with the game mode: `SleepingSellsTheShipment`, `StayingUpAllNightSellsTheShipment`, `TheDayReportCarriesWhatTheBinPaid` (the day report's `Paid` clause has `Shipping.Paid` and the total, and the wallet was paid before the report), `AMorningWithoutASaleHasAnEmptyPaidClause`, `TheMorningMessageEndsWithTheBinsPay` (the day report formatted with `DA_FeedbackResponses`: "... left. The bin paid 250."; without a sale it ends at "left."), `TheWitheringLineStillFollowsTheDayMessage` (a season's first morning, reports in order), `WithoutAWalletTheShipmentWaits` (and the clause is empty), `TheMorningsSaveHasTheCoinsAndAnEmptyBin` (the registry's snapshot after `OnNewDayFinished`).
- `Vestige.Content.TheHarvestsSellAtTheirPrices` (25, 18, 110) and `SeedsAndToolsDontSell`.
- `Vestige.UI.TheMoneyLineShowsCoinsAndTheAmount` (`FormatMoney`: "Coins 100", "Coins 1,500"); `TheHudWidgetHasTheMoneyLine` (creates `BP_GameMode`'s HUD widget class and checks `MoneyText` is bound; fails until the hand step is done); `SetMoneyTextWithoutThePartDoesNothing`; `EveryFeedbackTagHasAResponse` lists the three tags with `Item` and `Count`, none, and `Amount`, and `Paid` among the six day tags' values.
- `Vestige.Feedback.*` for clauses ([section 15](15-EffectsAndFeedback.md)): `AClauseFillsItsArgumentWithItsRowsText`, `AClauseWithoutATagIsEmptyAndLeavesNoTrailingSpace`, `AClauseWithoutARowIsEmptyAndWarns`, `TheLogShowsAReportsClauses`.
- `Vestige.Architecture.IncludeDirection` for the layers.

**Manual checks** (for `Docs/PlayTest.md`, by the test author with the commit):

- The bin's look and place: it reads as the bin beside the well from the field, its gear plate faces the field, and the farmer walks between it and the well without catching.
- From every side of the bin, facing it shows "F Ship" and facing away doesn't; next to the well, facing each picks the right one.
- A night with a sale: the day message with "The bin paid N." at its end reads as one sentence and fits its box, and the morning's lines (day, withering on a season's first day, waking, saved) are readable in the time they show.
- The coins line under the stamina reads well at 1920 by 1080 and doesn't crowd the status panel.
- A play of the first days against the season budget: by day 14 about 1,000 to 1,500 coins ([GameDesign.md, section 9](../GameDesign.md#economy)); this is a game design check, not a test.

**Pitfalls:**

- Selling from a listener of a clock event: the order against the day message and the withering line is then the order of subscription.
- Keeping the shipment on the bin: a map change or a second bin loses it.
- Removing the stack after adding it to the shipment: a refused removal would ship items the player still has.
- Building the prompt or the money text on the scan or every frame.
- Treating `SellPrice` 0 as "unpriced, sell at some default": 0 means the bin won't take it.
- Renaming `Base.Wallet` or `Base.Shipping`, or the item IDs a shipment saves: their names are in save files ([section 1](01-Saving.md)).
- A placed bin without `BP_ShippingBin`'s mesh: the C++ class has none, so it is invisible.
