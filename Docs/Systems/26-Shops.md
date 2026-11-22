# 26. Buying and shops

One of the [system guides](../Systems.md); read [How to use this document](../Systems.md#how-to-use-this-document) first.

**Status:** designed and built 2026-11-22 (the content by `EditorScripts/SetUpSeedShop.py`, with the stand-in cube) for [Roadmap.md, 4.5](../Roadmap.md#45-automation-and-selling) ("A simple way to buy seeds, with one cheap basic seed always for sale"), from [GameDesign.md, section 9](../GameDesign.md#economy) ("Buying seeds": the texts decided by the game designer within its limits, and three dispensers, all three seeds always and without limit, and one seed per press decided by @DunderGG on 2026-11-22 for the slice, with a store building to visit replacing the dispensers later), built so each choice is a data or text change. Money is the wallet's ([section 25](25-Selling.md)); this guide adds spending to it.

**References:**

- Epic's "Data Assets" and "Asset Management" pages: a `UPrimaryDataAsset` type listed in `PrimaryAssetTypesToScan` is found by its primary asset ID and cooked by its cook rule; ours are found through `UVestigeContentCatalog` ([section 2](02-Content.md)).
- Epic's "Gameplay Framework" pages (`APlayerState`): the purse and the bag are on the player state ([section 8](08-Inventory.md), [section 25](25-Selling.md)).
- Lyra (`Source/LyraGame`) has no shop or currency. What we take is its separation of definition and use, as the inventory took it: a definition asset with no runtime state (the shop's stock), and an actor that is only the place the player uses it, as the bin and the well are ([section 7](07-Interaction.md), [section 24](24-Water.md)).
- It uses only engine API the project already uses (`UPrimaryDataAsset`, `FPrimaryAssetId` with `AllowedTypes`, `UBoxComponent`, `FText::Format`, `/Engine/BasicShapes/Cube` as a stand-in mesh, as the bed had).

**What we take and what we skip:**

- **Take:** the stock as data (a shop asset listing offers, each an item ID and a price), so the sprinkler or a mod's seed joins a shop without code; the buying rule as one function every shop uses; the dispenser as an interactable of the bin's family.
- **Skip:** a shop subsystem or any saved shop state (the stock is unlimited and the prices fixed, [GameDesign.md, section 9](../GameDesign.md#economy)); a shop screen and amounts ([roadmap 4.6](../Roadmap.md#46-interface-and-input)); stock limits, restocking, seasonal stock, sales and haggling; Blueprint exposure until a Blueprint caller exists; replication.

**How the parts fit:**

| Part | Kind and layer | Owns |
| --- | --- | --- |
| `UVestigeShopData` + `FVestigeShopOffer` | **New.** Primary data asset `VestigeShop` (`DA_Shop_<Name>` in `Content/Vestige/Shops`), `Content/` | A shop's stock: its offers, each an item ID and the price of one, and finding the offer for an item |
| `VestigeShop::Buy` + `EVestigeBuyResult` | **New.** Free function and enum, `Inventory/VestigeShop.h` | The buying rule: the bag first, then the purse, all or nothing, spend and add |
| `UVestigeWalletComponent` | Existing, `Inventory/` | Now also `SpendMoney` and `FindForPawn` |
| `AFarmShopDispenser` | **New.** Actor (`BP_ShopDispenser` and one child per offer, placed in `L_Farm`'s service yard), `Farming/` | Being a dispenser: resolving its one offer, offering "Buy Turnip Seeds (10)", buying through `Buy` and telling the player how it went |
| `UVestigeInventoryComponent` | Existing, `Inventory/` | Unchanged: `CanAddItem` and `AddItem` are what the rule calls |

**Rules for the stock:**

- **One asset per shop:** `UVestigeShopData`, a `UVestigeContentData` subclass of primary asset type `VestigeShop`, scanned in `Config/DefaultGame.ini` from `/Game/Vestige/Shops` with `CookRule=AlwaysCook` and `bHasBlueprintClasses=False`, so the content catalogue loads it with the other content and `Find<UVestigeShopData>(Id)` never loads. The slice has one, `DA_Shop_Farm`, ContentId `Base.FarmShop`.
- **An offer** is `FVestigeShopOffer`: `ItemId` (`FPrimaryAssetId`, `meta = (AllowedTypes = "VestigeItem")`) and `Price` (`int32`, coins for one, `ClampMin = 1`). `Offers` is a private `TArray<FVestigeShopOffer>` (`EditDefaultsOnly`) with `GetOffers()` and `FindOffer(const FPrimaryAssetId& ItemId)`, which returns a pointer to the offer or nullptr. The order of the list is the order a later shop screen shows.
- **`DA_Shop_Farm`'s offers:** `VestigeItem:Base.TurnipSeeds` 10, `VestigeItem:Base.PotatoSeeds` 20, `VestigeItem:Base.CabbageSeeds` 40 ([GameDesign.md, section 6](../GameDesign.md#crops)). The prices are the shop's, not the items' ([section 8](08-Inventory.md)): two shops may price one item differently, and an item's `SellPrice` is unrelated. The sprinkler becomes one more offer when it exists as an item.
- **Data validation** (`IsDataValid`, `WITH_EDITOR`): every offer's `ItemId` passes `UVestigeContentData::FindContentReferenceError(ItemId, UVestigeItemData::AssetType)` (set, an item, known to the Asset Manager; the fragments' check, not a second one), its `Price` is at least 1, and no item is offered twice.
- **The game design's open answers are data:** stock (b), only the turnip, removes two offers (and their dispensers); a pack of 5 is a later `Count` on the offer (the rule already takes a count); none needs a new class.

**Rules for buying** (`Inventory/VestigeShop.h`, `namespace VestigeShop`):

- `EVestigeBuyResult Buy(UVestigeInventoryComponent& Inventory, UVestigeWalletComponent& Wallet, const FVestigeShopOffer& Offer, int32 Count)`, with `enum class EVestigeBuyResult : uint8 { Bought, InvalidOffer, NoRoom, NotEnoughMoney }`. It is the one place a purchase happens, for a dispenser now and a shop screen or a town shop later.
- **The checks, the first that applies:** `Count < 1` or `Price < 1` is `InvalidOffer`; `!Inventory.CanAddItem(ItemId, Count)` is `NoRoom` (also for an ID that names no item, which the inventory logs; a dispenser never gets there, since it resolved its item); a cost (`int64` `Price * Count`, never clamped) above `Wallet.GetMoney()` is `NotEnoughMoney`; past that check the cost is at most the purse, so it fits `int32`. Clamping the cost would sell an order above `MaxMoney` for `MaxMoney`. The bag comes before the purse ([GameDesign.md, section 9](../GameDesign.md#economy)). A refusal changes nothing.
- **Then spend, then add:** `Wallet.SpendMoney(static_cast<int32>(Cost))`, then `Inventory.AddItem(ItemId, Count)`. Both were checked, so neither fails in a single-threaded game; if the add still fails, the cost is given back with `AddMoney`, an `Error` is logged and the result is `NoRoom`, so the player never pays for nothing. There is no credit or debt.
- **Instant and free:** no stamina, no clock time, no world needed (the function takes the components), so tests call it directly.
- **Logging:** `LogVestigeInventory`, the context object the inventory: a purchase at `Log` (item, count, cost, purse after), a refusal at `Verbose`.

**Rules for spending** (added to `UVestigeWalletComponent`, [section 25](25-Selling.md)):

- `bool SpendMoney(int32 Amount)`: returns false and changes nothing for 0 or less or more than `GetMoney()`; otherwise subtracts, logs at `Verbose` and broadcasts `OnMoneyChanged`.
- `static UVestigeWalletComponent* FindForPawn(const AActor* Pawn)`, as `UVestigeInventoryComponent::FindForPawn`: the wallet on the pawn's player state, or nullptr.
- Neither is `BlueprintCallable` yet: the dispenser is C++. A Blueprint mod's shop gets them with the first such caller ([Architecture.md, section 12](../Architecture.md#12-designing-for-mods)).

**Rules for the dispenser:**

- **`AFarmShopDispenser`** (`Farming/FarmShopDispenser.h`, `UCLASS(Blueprintable)`, as `AFarmShippingBin`): a `Root` scene component, `DispenserMesh` (a static mesh component with its default blocking collision and no mesh in C++), and `InteractionVolume`, a box with the `VestigeInteractable` profile, overlap events off and `SetCanEverAffectNavigation(false)`. No tick.
- **One dispenser, one offer.** Two private `EditDefaultsOnly` properties name it: `ShopId` (`FPrimaryAssetId`, `AllowedTypes = "VestigeShop"`) and `ItemId` (`AllowedTypes = "VestigeItem"`). `BP_ShopDispenser` (`Content/Vestige/Blueprints`, parent `AFarmShopDispenser`) sets the mesh and `ShopId` `VestigeShop:Base.FarmShop`; its children `BP_ShopDispenser_TurnipSeeds`, `_PotatoSeeds` and `_CabbageSeeds` set `ItemId` (and later the window's material that shows the seed). The map holds only their placement; no content ID is in C++. Public const getters `GetShopId()` and `GetItemId()` (for tests and a later shop screen), and a `WITH_DEV_AUTOMATION_TESTS` setter for both, used before `BeginPlay` on a deferred spawn.
- **Resolving, once, at `BeginPlay`:** the content catalogue's shop, its `FindOffer(ItemId)`, and the item. Kept: the offer (a copy), the item (`UPROPERTY(Transient) TObjectPtr<const UVestigeItemData>`; the catalogue keeps it loaded) and the prompt, `LOCTEXT "Buy {Item} ({Price})"` with the item's display name and `FText::AsNumber(Price)`: "Buy Turnip Seeds (10)". If anything is missing (no catalogue, an unknown shop or item, or the shop doesn't offer the item), it logs a `Warning` (`LogFarm`, naming the actor, the shop and the item) and offers nothing all game. The data doesn't change while the game runs, so nothing is resolved again; the scan, five times a second, builds no text.
- **The option.** `GetInteractionOptions(Interactor)` adds one option, `{Text BuyText, Action "Buy"}` (`BuyAction`, a static `FName`), with no location, when the offer resolved and the interactor has both an inventory and a wallet (`FindForPawn` on each). It is offered whatever the purse and the bag hold, so a press tells the player why it can't buy.
- **Buying** (`Interact` with the `Buy` action; any other action does nothing): find the inventory and the wallet, then `VestigeShop::Buy(*Inventory, *Wallet, Offer, 1)`, one per press ([GameDesign.md, section 9](../GameDesign.md#economy)), and report: `Bought` → `Vestige.Feedback.Shop.Bought` with `Item` (the display name), `Count` and `Price` (the cost); `NoRoom` → `Vestige.Feedback.Inventory.Full` (the harvest's "No room in your bag."); `NotEnoughMoney` → `Vestige.Feedback.Shop.NotEnoughMoney` with `Price` (the cost); `InvalidOffer` reports nothing (the warning was logged at `BeginPlay`; the option isn't offered then). The interact ability ends at once and doesn't repeat while F is held.
- **Sized for its mesh:** a 0.9 by 0.9 m cabinet, 1.5 m high, pivot on the floor at its centre, its front (the window and the slot) towards the mesh's +X, as the bin's gear plate. The box's half extents are 55 (X, front to back), 45 (Y, side to side) and 75 cm, at 75 cm up. Against the front the farmer's capsule (42 cm) keeps his centre 87 cm from the cabinet's centre and the box reaches 55, so the facing ray never starts inside it ([section 7](07-Interaction.md)); 45 to the sides is the cabinet's own width, so on a 1 m pitch the boxes of neighbours don't overlap: facing a dispenser picks it, never its neighbour, and a farmer centred on the 10 cm gap between two faces neither and gets no prompt until he steps in front of one (better than a wrong pick). Not usable at a corner, as the bin. A change to the hull (the artist's `UCX_SM_ShopDispenser_00`) re-checks these numbers.
- **Placement** in `L_Farm`'s service yard, continuing the line of the well (Y 360) and the bin (Y 600), all at X 1880, yaw 180 so their fronts face the field: the turnip dispenser at Y 900, the potato at Y 1000, the cabbage at Y 1100, Z 0. That leaves 2.1 m between the bin and the first cabinet and 10 cm between cabinets. The blockout may move the row, keeping it together and beside the bin.

**Feedback** ([section 15](15-EffectsAndFeedback.md), two new native tags, each a row in `DA_FeedbackResponses`; texts from [GameDesign.md, section 9](../GameDesign.md#economy)):

| Tag | Reported by | Values | Text |
| --- | --- | --- | --- |
| `Vestige.Feedback.Shop.Bought` | the dispenser | `Item`, `Count`, `Price` | Bought: {Item} ×{Count} for {Price}. |
| `Vestige.Feedback.Shop.NotEnoughMoney` | the dispenser | `Price` | Not enough coins. These seeds cost {Price}. |
| `Vestige.Feedback.Inventory.Full` (existing) | the harvest, and now the dispenser | | No room in your bag. |

The native tag comment of `Inventory.Full` changes from "so the harvest stays" to "so nothing is added". "These seeds" and "coins" are the slice's words: a non-seed offer (the sprinkler) or the writer's currency name changes the row's text, not the rule.

**The mesh until the art comes.** `SM_ShopDispenser` (the artist's, with hull `UCX_SM_ShopDispenser_00` and a window per seed) comes later ([Roadmap.md, 4.5](../Roadmap.md#45-automation-and-selling)). Until it is imported into `Content/Vestige/Meshes`, `BP_ShopDispenser`'s `DispenserMesh` is the stand-in `/Engine/BasicShapes/Cube` (engine content, no licensing record; a plain placeholder as the [home farm plan](../Levels/HomeFarm/README.md) asks) at relative scale (0.9, 0.9, 1.5) and location Z 75, because the cube is 100 cm with its pivot at its centre: so it fills the floor-pivot cabinet the box is sized for. When the real mesh is set, the scale goes back to 1 and the location to 0 (the bed's lesson). The C++ stays meshless; no `ConstructorHelpers`.

**Editor steps** (after the C++ is built and the editor reopened). `EditorScripts/SetUpSeedShop.py`, written like `SetUpShippingBin.py` (idempotent, logs each fact, headless with the editor closed, [section 23](23-EditorScripting.md)), does all of them, and is run again when the real mesh is imported:

1. **`DA_Shop_Farm`** in `Content/Vestige/Shops`: ContentId `Base.FarmShop`, display name "Farm Shop" (set from the script, so not yet gathered for localization; set it as localizable text when a shop screen first shows it), the three offers above.
2. **Feedback rows** in `DA_FeedbackResponses`: the two new tags with their texts and no sound.
3. **`BP_ShopDispenser`** (`ShopId`, and the mesh: `SM_ShopDispenser` at scale 1 and location 0 when it exists, otherwise the stand-in cube at the scale and offset above, logged as a stand-in) and its three children (`ItemId`).
4. **`L_Farm`:** places the three children at their places if the map has none of them, and saves the map.

The `PrimaryAssetTypesToScan` line in `Config/DefaultGame.ini` is the implementer's, with the C++.

**Saving.** Nothing new: a purchase changes the purse (`Base.Wallet`) and the bag (`Base.Inventory`), both saved already. The shop and the dispensers have no state. A save from before the shop loads unchanged.

**Layers.** `UVestigeShopData` is in `Content/` (a definition). `VestigeShop::Buy` is in `Inventory/`, beside the two components it joins, and may include `Content/`. The dispenser is in `Farming/` with the bin and the well, and includes `Inventory/`, `Content/`, `Interaction/` and `Core/`. No new folder, so `Vestige.Architecture.IncludeDirection`'s table doesn't change.

**Performance.** No tick or timer. Resolving is three catalogue finds and one `FText::Format` per dispenser at `BeginPlay`. The options cost two component lookups five times a second while the farmer is near a dispenser, and no text. A purchase is two checks, an integer and an add, on a press; the HUD rebuilds the money and bag lines on their events.

**Mods.** A mod's shop is a `DA_Shop_` asset of its own; its dispenser a Blueprint child of `AFarmShopDispenser` naming that shop and an item. A mod can't yet add an offer to `Base.FarmShop` (the list is one asset), nor place a dispenser on our farm without editing the map; both wait for placing buildings ([section 16](16-PlacingAndBuilding.md)) and the mod loader ([Architecture.md, section 12](../Architecture.md#12-designing-for-mods)).

**Rejected:**

- A `BuyPrice` on `UVestigeItemData`: one price for every shop, and [section 8](08-Inventory.md) gives prices to the shop.
- The price and item on each placed dispenser only: prices scattered in a map, no list a shop screen can read, not content the Asset Manager finds.
- A shop subsystem: it would own no state (unlimited stock, nothing saved); one comes with stock limits or restocking.
- The buying rule in the wallet (it knows no items) or in the dispenser (a shop screen would copy it).
- Add, then spend: equivalent, but undoing it removes items, which can't say which stack they came from; giving coins back is exact.
- One dispenser with a key to step its offers, or a shop screen: rejected for the slice by @DunderGG (2026-11-22, [GameDesign.md, section 9](../GameDesign.md#economy)).
- An offer index on the dispenser: reordering the list would swap what it sells.

**Tests** (`Source/Vestige/Tests/`):

- `Vestige.Wallet.*`: `SpendingTakesTheAmountAndBroadcasts`, `SpendingMoreThanThePurseChangesNothing`, `ASpendOfZeroOrLessChangesNothing`, `SpendingTheWholePurseLeavesZero`, `FindForPawnFindsThePlayerStatesWallet`.
- `Vestige.Shop.*` (`Buy` on a test inventory and wallet with test items in the catalogue): `BuyingAddsTheItemAndTakesThePrice`, `ABuyCostsPriceTimesCount`, `ACostAboveThePurseLimitIsNotEnoughMoney` (price `MaxMoney`, count 2, a full purse), `AFullBagBuysNothingAndCostsNothing`, `AShortPurseBuysNothing`, `TheBagIsCheckedBeforeThePurse` (full bag and short purse is `NoRoom`), `BuyingWithExactlyThePriceLeavesZero`, `ACountOrPriceBelowOneIsInvalid`, `BuyingIntoAPartStackMergesIt`.
- `Vestige.Shop.*` with the dispenser (spawned deferred with its offer set for tests, the Blueprint farmer as the bin's tests): `TheDispenserOffersBuyWithTheItemAndPrice` ("Buy Turnip Seeds (10)"), `TheDispenserOffersNothingWithoutABagOrAWallet`, `ADispenserWithoutItsOfferOffersNothingAndWarns`, `BuyingReportsTheItemCountAndPrice`, `AShortPurseReportsThePrice`, `AFullBagReportsNoRoom`, `AnotherActionAtTheDispenserDoesNothing`, `AFarmerFacingADispenserFocusesItNotItsNeighbour` (three in a row on a 1 m pitch; a farmer centred on a gap focuses neither).
- `Vestige.Content.*`: `TheFarmShopSellsTheThreeSeedsAtTheirPrices` (`Base.FarmShop`: 10, 20, 40, and each ID is an item), `AShopOfferingAnItemTwiceIsInvalid` (`IsDataValid`), `TheDispenserBlueprintsOfferTheThreeSeeds` (each child's `ShopId` and `ItemId`, and that it has a mesh, whichever).
- `Vestige.UI.EveryFeedbackTagHasAResponse` lists the two new tags with their values.
- `Vestige.Architecture.IncludeDirection` for the layers.

**Manual checks** (for `Docs/PlayTest.md`, by the test author with the commit):

- The row's place: the three stand-ins face the field and read as one row beside the bin, and the farmer walks between the bin and the row without catching. (Telling the three seeds apart at a glance is the art item's check.)
- Facing each dispenser shows its own "F Buy ... (price)" and never its neighbour's; between two there is no prompt until he steps in front of one, and that doesn't feel wrong.
- Buying on day 1 and after the first sale: the purse and the bag lines change at once, and the messages read well with the morning's.
- Whether one seed per press feels tedious for a usual replant of 2 to 10 seeds (hold-to-repeat is the fallback if it does; [GameDesign.md, section 9](../GameDesign.md#economy)).

**Pitfalls:**

- Spending before checking the bag: a full bag would take the coins.
- Clamping the cost before comparing it with the purse: a big order would sell for `MaxMoney`.
- Leaving the stand-in's scale and offset when the real mesh is set: the cabinet would be 0.9 by 1.5 times too big and float 75 cm up.
- Building the prompt in `GetInteractionOptions`: the scan asks five times a second.
- A content ID in C++ for the shop or the item: it goes on the Blueprint.
- Treating a missing offer as price 0 (free): a missing offer sells nothing.
- Renaming `Base.FarmShop` or the `VestigeShop` type: Blueprints name them by ID (nothing is saved with them, so no save breaks).
