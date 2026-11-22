// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/VestigeGameplayTags.h"
#include "Components/ActorTestSpawner.h"
#include "Components/StaticMeshComponent.h"
#include "Content/FarmCropData.h"
#include "Content/VestigeCalendar.h"
#include "Content/VestigeContentCatalog.h"
#include "Content/VestigeItemData.h"
#include "Core/VestigeFeedbackSubsystem.h"
#include "Core/VestigeSaveRecord.h"
#include "Core/VestigeSaveRegistry.h"
#include "CQTest.h"
#include "Engine/StaticMesh.h"
#include "Farming/FarmField.h"
#include "Farming/FarmFieldSubsystem.h"
#include "Farming/FarmGrid.h"
#include "Farming/FarmShippingBin.h"
#include "Farming/FarmShippingSaveData.h"
#include "Farming/FarmShippingSubsystem.h"
#include "Farming/FarmTargetingComponent.h"
#include "Game/VestigeGameMode.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/VestigeInteractionComponent.h"
#include "Inventory/VestigeInventoryComponent.h"
#include "Inventory/VestigeInventorySaveData.h"
#include "Inventory/VestigeWalletComponent.h"
#include "Inventory/VestigeWalletSaveData.h"
#include "Player/VestigePlayerCharacter.h"
#include "Player/VestigePlayerState.h"
#include "Time/VestigeClockSaveData.h"
#include "Time/VestigeClockSubsystem.h"
#include "UI/VestigeFeedbackResponses.h"
#include "UI/VestigeHUD.h"
#include "UObject/UnrealType.h"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace FarmShippingTests
{
	const TCHAR* const PlayerStatePath = TEXT("/Game/Vestige/Blueprints/BP_PlayerState.BP_PlayerState_C");
	const TCHAR* const FarmerPath = TEXT("/Game/Vestige/Blueprints/BP_PlayerCharacter.BP_PlayerCharacter_C");
	const TCHAR* const BinPath = TEXT("/Game/Vestige/Blueprints/BP_ShippingBin.BP_ShippingBin_C");
	const TCHAR* const GameModePath = TEXT("/Game/Vestige/Blueprints/BP_GameMode.BP_GameMode_C");

	/** The shipment, the shipping bin and the night's sale, with the game mode that runs it. */
	TEST_CLASS(Shipping, "Vestige")
	{
		FActorTestSpawner Spawner;
		UVestigeContentCatalog* Catalog = nullptr;
		UFarmShippingSubsystem* Shipment = nullptr;
		UVestigeClockSubsystem* Clock = nullptr;
		AVestigePlayerState* PlayerState = nullptr;
		AVestigePlayerCharacter* Farmer = nullptr;
		UVestigeWalletComponent* Wallet = nullptr;
		TArray<FVestigeFeedbackReport> Reports;

		/** What the wallet held when the first report with clauses (the day report) was made, or INDEX_NONE. */
		int32 MoneyAtTheDayReport = INDEX_NONE;

		BEFORE_EACH()
		{
			Spawner.InitializeGameSubsystems();
			Catalog = UVestigeContentCatalog::Get(&Spawner.GetWorld());
			Shipment = UFarmShippingSubsystem::Get(&Spawner.GetWorld());
			Clock = UVestigeClockSubsystem::Get(&Spawner.GetWorld());
			ASSERT_THAT(IsNotNull(Catalog));
			ASSERT_THAT(IsNotNull(Shipment));
			ASSERT_THAT(IsNotNull(Clock));

			AVestigeGameMode& GameMode = Spawner.SpawnActor<AVestigeGameMode>();
			GameMode.DispatchBeginPlay();

			UVestigeFeedbackSubsystem* Feedback = Spawner.GetWorld().GetSubsystem<UVestigeFeedbackSubsystem>();
			ASSERT_THAT(IsNotNull(Feedback));
			Feedback->OnFeedback.AddLambda(
				[this](const FVestigeFeedbackReport& Report)
				{
					if (MoneyAtTheDayReport == INDEX_NONE && !Report.Clauses.IsEmpty())
					{
						MoneyAtTheDayReport = Wallet ? Wallet->GetMoney() : INDEX_NONE;
					}
					Reports.Add(Report);
				});
		}

		// ---- Setting up ----

		/** A BP_PlayerState in play with a controller that has it, so the game mode finds the wallet; and the farmer, if asked. */
		void SetUpPlayer(bool bWithFarmer)
		{
			UClass* PlayerStateClass = LoadClass<AVestigePlayerState>(nullptr, PlayerStatePath);
			check(PlayerStateClass);
			PlayerState = &Spawner.SpawnActor<AVestigePlayerState>(FActorSpawnParameters(), PlayerStateClass);
			PlayerState->DispatchBeginPlay();
			Wallet = PlayerState->GetWallet();
			check(Wallet);

			APlayerController& Controller = Spawner.SpawnActor<APlayerController>();
			Controller.PlayerState = PlayerState;
			if (bWithFarmer)
			{
				// The farmer's facing comes from the farm grid, so the grid is in play before he is.
				Spawner.SpawnActor<AFarmGrid>().DispatchBeginPlay();
				UClass* FarmerClass = LoadClass<AVestigePlayerCharacter>(nullptr, FarmerPath);
				check(FarmerClass);
				Farmer = &Spawner.SpawnActor<AVestigePlayerCharacter>(FActorSpawnParameters(), FarmerClass);
				Farmer->DispatchBeginPlay();
				Controller.Possess(Farmer);
				Farmer->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
			}
		}

		/** Sets the item's sell price, which has no setter: the property is private and edited in the data asset. */
		static void SetSellPrice(const UVestigeItemData& Item, int32 Price)
		{
			FIntProperty* Property = FindFProperty<FIntProperty>(UVestigeItemData::StaticClass(), TEXT("SellPrice"));
			check(Property);
			Property->SetPropertyValue_InContainer(const_cast<UVestigeItemData*>(&Item), Price);
		}

		/** A test item in the catalogue with this price. */
		const UVestigeItemData& MakeItem(FName ContentId, int32 Price, int32 StackSize = 99)
		{
			UVestigeItemData& Item = Spawner.SpawnObject<UVestigeItemData>();
			Item.SetContentIdForTests(ContentId);
			Item.SetMaxStackSizeForTests(StackSize);
			SetSellPrice(Item, Price);
			Catalog->AddForTests(Item);
			return Item;
		}

		/** A real item of the game, from the catalogue. */
		const UVestigeItemData* FindItem(const TCHAR* ContentId) const
		{
			return Catalog->Find<UVestigeItemData>(FPrimaryAssetId(UVestigeItemData::AssetType, ContentId));
		}

		/** A bag of its own with Slots slots, not the player's. */
		UVestigeInventoryComponent& MakeBag(int32 Slots = 4)
		{
			AActor& Owner = Spawner.SpawnActor<AActor>();
			UVestigeInventoryComponent* Bag = NewObject<UVestigeInventoryComponent>(&Owner);
			Bag->ResetForTests(Slots);
			return *Bag;
		}

		/** A wallet of its own, empty, not the player's. */
		UVestigeWalletComponent& MakeWallet()
		{
			AActor& Owner = Spawner.SpawnActor<AActor>();
			return *NewObject<UVestigeWalletComponent>(&Owner);
		}

		/** Puts Count of Item into the bin through a bag of its own. Returns what was shipped. */
		FFarmShipResult Ship(const UVestigeItemData& Item, int32 Count)
		{
			UVestigeInventoryComponent& Bag = MakeBag();
			verify(Bag.TryAddItem(Item.GetPrimaryAssetId(), Count));
			return Shipment->ShipFromSlot(Bag, 0);
		}

		/** Restores the player's bag to exactly these slots, with slot 0 selected, as loading a save does. */
		void SetBag(TArray<FVestigeInventorySlotSaveData> Slots)
		{
			FVestigeInventorySaveData Bag;
			Bag.Slots = MoveTemp(Slots);
			UVestigeInventoryComponent* Inventory = PlayerState->GetInventory();
			check(Inventory);
			verify(Inventory->RestoreFromSaveData(Bag) == 0);
		}

		/** How many of the stacks in the bin are of this item, and its count there; 0 for none. */
		int32 GetShippedCount(const UVestigeItemData& Item) const
		{
			int32 Total = 0;
			for (const FVestigeItemStack& Stack : Shipment->GetShippedStacks())
			{
				if (Stack.ItemId == Item.GetPrimaryAssetId())
				{
					Total += Stack.Count;
				}
			}
			return Total;
		}

		// ---- Shipping ----

		TEST_METHOD(ShippingTakesTheWholeStackInHand)
		{
			const UVestigeItemData& Beet = MakeItem(TEXT("Test.Beet"), 25);
			UVestigeInventoryComponent& Bag = MakeBag();
			ASSERT_THAT(IsTrue(Bag.TryAddItem(Beet.GetPrimaryAssetId(), 10)));

			const FFarmShipResult Result = Shipment->ShipFromSlot(Bag, 0);

			ASSERT_THAT(IsTrue(Result.Item == &Beet));
			ASSERT_THAT(AreEqual(Result.Count, 10));
			ASSERT_THAT(AreEqual(Bag.GetItemCount(Beet.GetPrimaryAssetId()), 0));
			ASSERT_THAT(AreEqual(Shipment->GetShippedStacks().Num(), 1));
			ASSERT_THAT(AreEqual(GetShippedCount(Beet), 10));
		}

		TEST_METHOD(ShippingLeavesOtherStacksOfTheSameItem)
		{
			// Stacks of 5: eight beets are a stack of 5 and a stack of 3, and one press ships one stack.
			const UVestigeItemData& Beet = MakeItem(TEXT("Test.Beet"), 25, 5);
			UVestigeInventoryComponent& Bag = MakeBag();
			ASSERT_THAT(IsTrue(Bag.TryAddItem(Beet.GetPrimaryAssetId(), 8)));

			const FFarmShipResult Result = Shipment->ShipFromSlot(Bag, 0);

			ASSERT_THAT(AreEqual(Result.Count, 5));
			ASSERT_THAT(AreEqual(Bag.GetItemCount(Beet.GetPrimaryAssetId()), 3));
			ASSERT_THAT(AreEqual(GetShippedCount(Beet), 5));
		}

		TEST_METHOD(ShippingTheSameItemTwiceMergesIt)
		{
			const UVestigeItemData& Beet = MakeItem(TEXT("Test.Beet"), 25);
			ASSERT_THAT(AreEqual(Ship(Beet, 4).Count, 4));
			ASSERT_THAT(AreEqual(Ship(Beet, 6).Count, 6));

			ASSERT_THAT(AreEqual(Shipment->GetShippedStacks().Num(), 1));
			ASSERT_THAT(AreEqual(GetShippedCount(Beet), 10));
		}

		TEST_METHOD(DifferentItemsAreSeparateEntries)
		{
			const UVestigeItemData& Beet = MakeItem(TEXT("Test.Beet"), 25);
			const UVestigeItemData& Leek = MakeItem(TEXT("Test.Leek"), 18);
			Ship(Beet, 2);
			Ship(Leek, 3);

			ASSERT_THAT(AreEqual(Shipment->GetShippedStacks().Num(), 2));
			ASSERT_THAT(AreEqual(GetShippedCount(Beet), 2));
			ASSERT_THAT(AreEqual(GetShippedCount(Leek), 3));
		}

		TEST_METHOD(SeedsAToolAndAnEmptyHandShipNothing)
		{
			const UVestigeItemData& NotForSale = MakeItem(TEXT("Test.Keepsake"), 0);
			UVestigeInventoryComponent& Bag = MakeBag();
			ASSERT_THAT(IsTrue(Bag.TryAddItem(NotForSale.GetPrimaryAssetId(), 3)));

			// The price 0 means "not for sale", never "sell at a default".
			ASSERT_THAT(IsNull(Shipment->ShipFromSlot(Bag, 0).Item));
			ASSERT_THAT(AreEqual(Bag.GetItemCount(NotForSale.GetPrimaryAssetId()), 3));
			// An empty slot, and slots that don't exist.
			ASSERT_THAT(AreEqual(Shipment->ShipFromSlot(Bag, 1).Count, 0));
			ASSERT_THAT(AreEqual(Shipment->ShipFromSlot(Bag, -1).Count, 0));
			ASSERT_THAT(AreEqual(Shipment->ShipFromSlot(Bag, 99).Count, 0));
			ASSERT_THAT(IsTrue(Shipment->GetShippedStacks().IsEmpty()));
		}

		TEST_METHOD(TheGamesSeedsAndToolsAreNotShipped)
		{
			UVestigeInventoryComponent& Bag = MakeBag(8);
			const TCHAR* const Names[] = {TEXT("Base.TurnipSeeds"), TEXT("Base.PotatoSeeds"), TEXT("Base.CabbageSeeds"), TEXT("Base.Hoe"),
				TEXT("Base.WateringCan")};
			for (const TCHAR* Name : Names)
			{
				const UVestigeItemData* Item = FindItem(Name);
				ASSERT_THAT(IsNotNull(Item, FString(Name)));
				ASSERT_THAT(IsTrue(Bag.TryAddItem(Item->GetPrimaryAssetId(), 1)));
			}

			for (int32 Slot = 0; Slot < 5; ++Slot)
			{
				ASSERT_THAT(IsNull(Shipment->ShipFromSlot(Bag, Slot).Item));
			}
			ASSERT_THAT(IsTrue(Shipment->GetShippedStacks().IsEmpty()));
		}

		// ---- The sale ----

		TEST_METHOD(TheSalePaysEachItemsPrice)
		{
			// 10 turnips at 25 and 3 cabbages at 110.
			const UVestigeItemData& Turnip = MakeItem(TEXT("Test.Turnip"), 25);
			const UVestigeItemData& Cabbage = MakeItem(TEXT("Test.Cabbage"), 110);
			Ship(Turnip, 10);
			Ship(Cabbage, 3);
			UVestigeWalletComponent& Purse = MakeWallet();

			const int32 Paid = Shipment->SellShipment(Purse);

			ASSERT_THAT(AreEqual(Paid, 580));
			ASSERT_THAT(AreEqual(Purse.GetMoney(), 580));
		}

		TEST_METHOD(TheSaleEmptiesTheBinAndReturnsTheTotal)
		{
			const UVestigeItemData& Turnip = MakeItem(TEXT("Test.Turnip"), 25);
			Ship(Turnip, 4);
			UVestigeWalletComponent& Purse = MakeWallet();

			ASSERT_THAT(AreEqual(Shipment->SellShipment(Purse), 100));

			ASSERT_THAT(IsTrue(Shipment->GetShippedStacks().IsEmpty()));
			// A second sale the same night has nothing to pay.
			ASSERT_THAT(AreEqual(Shipment->SellShipment(Purse), 0));
			ASSERT_THAT(AreEqual(Purse.GetMoney(), 100));
		}

		TEST_METHOD(TheSalePaysThePriceOnTheNight)
		{
			const UVestigeItemData& Beet = MakeItem(TEXT("Test.Beet"), 10);
			Ship(Beet, 4);
			SetSellPrice(Beet, 20);
			UVestigeWalletComponent& Purse = MakeWallet();

			ASSERT_THAT(AreEqual(Shipment->SellShipment(Purse), 80));
		}

		TEST_METHOD(AStackThatNoLongerSellsIsDroppedAndPaysNothing)
		{
			const UVestigeItemData& Beet = MakeItem(TEXT("Test.Beet"), 10);
			const UVestigeItemData& Leek = MakeItem(TEXT("Test.Leek"), 5);
			Ship(Beet, 4);
			Ship(Leek, 2);
			SetSellPrice(Beet, 0);
			UVestigeWalletComponent& Purse = MakeWallet();
			TestRunner->AddExpectedMessagePlain(TEXT("no longer sell"), ELogVerbosity::Warning);

			ASSERT_THAT(AreEqual(Shipment->SellShipment(Purse), 10));

			ASSERT_THAT(AreEqual(Purse.GetMoney(), 10));
			ASSERT_THAT(IsTrue(Shipment->GetShippedStacks().IsEmpty()));
		}

		TEST_METHOD(AnEmptyBinPaysNothingAndChangesNothing)
		{
			UVestigeWalletComponent& Purse = MakeWallet();
			int32 Broadcasts = 0;
			Purse.OnMoneyChanged.AddLambda(
				[&Broadcasts]()
				{
					++Broadcasts;
				});

			ASSERT_THAT(AreEqual(Shipment->SellShipment(Purse), 0));

			ASSERT_THAT(AreEqual(Purse.GetMoney(), 0));
			ASSERT_THAT(AreEqual(Broadcasts, 0));
		}

		TEST_METHOD(ATotalAboveTheLimitIsClamped)
		{
			// 5 at 500 million is 2.5 billion, more than an int32 holds.
			const UVestigeItemData& Gem = MakeItem(TEXT("Test.Gem"), 500000000);
			Ship(Gem, 5);
			UVestigeWalletComponent& Purse = MakeWallet();
			TestRunner->AddExpectedMessagePlain(TEXT("above the limit"), ELogVerbosity::Warning);

			ASSERT_THAT(AreEqual(Shipment->SellShipment(Purse), VestigeWallet::MaxMoney));

			ASSERT_THAT(AreEqual(Purse.GetMoney(), VestigeWallet::MaxMoney));
		}

		// ---- Saving ----

		TEST_METHOD(TheShipmentIsSavedAndRestored)
		{
			ASSERT_THAT(IsTrue(Shipment->GetSaveKey() == FName(TEXT("Base.Shipping"))));
			ASSERT_THAT(AreEqual(Shipment->GetSaveVersion(), 1));
			const UVestigeItemData& Turnip = *FindItem(TEXT("Base.Turnip"));
			const UVestigeItemData& Cabbage = *FindItem(TEXT("Base.Cabbage"));
			Ship(Turnip, 7);
			Ship(Cabbage, 2);
			const FVestigeSaveRecord Record = Shipment->WriteSaveRecord();

			Shipment->StartNewGame();
			ASSERT_THAT(IsTrue(Shipment->GetShippedStacks().IsEmpty()));
			int32 LeftOut = 0;
			ASSERT_THAT(IsTrue(Shipment->ReadSaveRecord(Record, LeftOut)));

			ASSERT_THAT(AreEqual(LeftOut, 0));
			ASSERT_THAT(AreEqual(Shipment->GetShippedStacks().Num(), 2));
			ASSERT_THAT(AreEqual(GetShippedCount(Turnip), 7));
			ASSERT_THAT(AreEqual(GetShippedCount(Cabbage), 2));
		}

		TEST_METHOD(AnUnknownShippedItemIsLeftOutAndCounted)
		{
			FFarmShippingSaveData Data;
			Data.Stacks.Add({TEXT("Base.Turnip"), 3});
			Data.Stacks.Add({TEXT("Gone.Item"), 2});
			Data.Stacks.Add({TEXT("Base.Cabbage"), 0});
			Data.Stacks.Add({TEXT("Base.Turnip"), 4});
			TestRunner->AddExpectedMessagePlain(TEXT("can't be restored"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 2);
			int32 LeftOut = 0;

			ASSERT_THAT(IsTrue(Shipment->ReadSaveRecord(FVestigeSaveRecord::Write(Data), LeftOut)));

			// The unknown item and the empty count are left out and counted; the two turnip entries are one.
			ASSERT_THAT(AreEqual(LeftOut, 2));
			ASSERT_THAT(AreEqual(Shipment->GetShippedStacks().Num(), 1));
			ASSERT_THAT(AreEqual(GetShippedCount(*FindItem(TEXT("Base.Turnip"))), 7));
		}

		TEST_METHOD(ANewGameEmptiesTheBin)
		{
			Ship(MakeItem(TEXT("Test.Beet"), 25), 3);

			Shipment->StartNewGame();

			ASSERT_THAT(IsTrue(Shipment->GetShippedStacks().IsEmpty()));
		}

		TEST_METHOD(ASavedShipmentComesBackThroughTheRegistry)
		{
			FFarmShippingSaveData Data;
			Data.Stacks.Add({TEXT("Base.Turnip"), 5});

			UVestigeSaveRegistry::Get(&Spawner.GetWorld())->Load({{TEXT("Base.Shipping"), FVestigeSaveRecord::Write(Data)}});

			ASSERT_THAT(AreEqual(GetShippedCount(*FindItem(TEXT("Base.Turnip"))), 5));
		}

		TEST_METHOD(LoadingSellsNothing)
		{
			SetUpPlayer(false);
			const int32 Before = Wallet->GetMoney();
			FFarmShippingSaveData Data;
			Data.Stacks.Add({TEXT("Base.Turnip"), 5});

			UVestigeSaveRegistry::Get(&Spawner.GetWorld())->Load({{TEXT("Base.Shipping"), FVestigeSaveRecord::Write(Data)}});

			// What was in the bin sells the next night, never on a load.
			ASSERT_THAT(AreEqual(Wallet->GetMoney(), Before));
			ASSERT_THAT(AreEqual(GetShippedCount(*FindItem(TEXT("Base.Turnip"))), 5));
		}

		// ---- The bin ----

		static inline const FVector BinLocation = FVector(1880.f, 600.f, 0.f);

		AFarmShippingBin& PlaceBin()
		{
			UClass* BinClass = LoadClass<AFarmShippingBin>(nullptr, BinPath);
			check(BinClass);
			AFarmShippingBin& Bin = Spawner.SpawnActor<AFarmShippingBin>(FActorSpawnParameters(), BinClass);
			Bin.SetActorLocation(BinLocation);
			return Bin;
		}

		TArray<FVestigeInteractionOption> GetOptions(AFarmShippingBin& Bin, AActor* Interactor) const
		{
			TArray<FVestigeInteractionOption> Options;
			IVestigeInteractable::Execute_GetInteractionOptions(&Bin, Interactor, Options);
			return Options;
		}

		/** Presses F's Ship at the bin: the option the bin offers, used through its interface. */
		void PressShip(AFarmShippingBin& Bin)
		{
			FVestigeInteractionOption Option;
			Option.Action = AFarmShippingBin::ShipAction;
			IVestigeInteractable::Execute_Interact(&Bin, Farmer, Option);
		}

		TEST_METHOD(TheBinOffersShipToAnInteractorWithABag)
		{
			SetUpPlayer(true);
			AFarmShippingBin& Bin = PlaceBin();
			// Whatever is in hand: a tool, so that F can say why nothing goes in.
			SetBag({{0, TEXT("Base.Hoe"), 1}});

			const TArray<FVestigeInteractionOption> Options = GetOptions(Bin, Farmer);

			ASSERT_THAT(AreEqual(Options.Num(), 1));
			ASSERT_THAT(IsTrue(Options[0].Action == AFarmShippingBin::ShipAction));
			ASSERT_THAT(IsFalse(Options[0].bHasLocation));
			ASSERT_THAT(AreEqual(Options[0].Text.ToString(), FString(TEXT("Ship"))));
		}

		TEST_METHOD(TheBinOffersNothingWithoutABag)
		{
			AFarmShippingBin& Bin = PlaceBin();

			ASSERT_THAT(IsTrue(GetOptions(Bin, &Spawner.SpawnActor<AActor>()).IsEmpty()));
			ASSERT_THAT(IsTrue(GetOptions(Bin, nullptr).IsEmpty()));
		}

		TEST_METHOD(ShippingReportsTheItemAndTheCount)
		{
			SetUpPlayer(true);
			AFarmShippingBin& Bin = PlaceBin();
			const UVestigeItemData* Turnip = FindItem(TEXT("Base.Turnip"));
			ASSERT_THAT(IsNotNull(Turnip));
			SetBag({{0, TEXT("Base.Turnip"), 10}});

			PressShip(Bin);

			ASSERT_THAT(AreEqual(Reports.Num(), 1));
			ASSERT_THAT(IsTrue(Reports[0].Tag == VestigeGameplayTags::Feedback_Shipping_Shipped));
			const FFormatArgumentValue* Item = Reports[0].Arguments.Find(TEXT("Item"));
			const FFormatArgumentValue* Count = Reports[0].Arguments.Find(TEXT("Count"));
			ASSERT_THAT(IsNotNull(Item));
			ASSERT_THAT(IsNotNull(Count));
			ASSERT_THAT(AreEqual(Item->GetTextValue().ToString(), Turnip->GetDisplayName().ToString()));
			ASSERT_THAT(AreEqual(static_cast<int32>(Count->GetIntValue()), 10));
			ASSERT_THAT(AreEqual(GetShippedCount(*Turnip), 10));
			ASSERT_THAT(AreEqual(PlayerState->GetInventory()->GetItemCount(Turnip->GetPrimaryAssetId()), 0));
		}

		TEST_METHOD(NothingSellableReportsHoldACrop)
		{
			SetUpPlayer(true);
			AFarmShippingBin& Bin = PlaceBin();
			SetBag({{0, TEXT("Base.Hoe"), 1}, {1, TEXT("Base.Turnip"), 4}});

			// A tool, then an empty hand, then seeds.
			PressShip(Bin);
			PlayerState->GetInventory()->SelectHotbarSlot(2);
			PressShip(Bin);

			ASSERT_THAT(AreEqual(Reports.Num(), 2));
			for (const FVestigeFeedbackReport& Report : Reports)
			{
				ASSERT_THAT(IsTrue(Report.Tag == VestigeGameplayTags::Feedback_Shipping_NothingToShip));
			}
			ASSERT_THAT(IsTrue(Shipment->GetShippedStacks().IsEmpty()));
			ASSERT_THAT(AreEqual(PlayerState->GetInventory()->GetItemCount(FPrimaryAssetId(UVestigeItemData::AssetType, TEXT("Base.Hoe"))), 1));
		}

		TEST_METHOD(TheBinShipsOnlyTheSelectedSlot)
		{
			SetUpPlayer(true);
			AFarmShippingBin& Bin = PlaceBin();
			const UVestigeItemData& Turnip = *FindItem(TEXT("Base.Turnip"));
			const UVestigeItemData& Cabbage = *FindItem(TEXT("Base.Cabbage"));
			SetBag({{0, TEXT("Base.Turnip"), 4}, {1, TEXT("Base.Cabbage"), 3}});

			PressShip(Bin);

			ASSERT_THAT(AreEqual(GetShippedCount(Turnip), 4));
			ASSERT_THAT(AreEqual(GetShippedCount(Cabbage), 0));
			ASSERT_THAT(AreEqual(PlayerState->GetInventory()->GetItemCount(Cabbage.GetPrimaryAssetId()), 3));
		}

		TEST_METHOD(AnotherActionAtTheBinDoesNothing)
		{
			SetUpPlayer(true);
			AFarmShippingBin& Bin = PlaceBin();
			SetBag({{0, TEXT("Base.Turnip"), 4}});
			FVestigeInteractionOption Option;
			Option.Action = TEXT("Harvest");

			IVestigeInteractable::Execute_Interact(&Bin, Farmer, Option);

			ASSERT_THAT(IsTrue(Reports.IsEmpty()));
			ASSERT_THAT(IsTrue(Shipment->GetShippedStacks().IsEmpty()));
			ASSERT_THAT(AreEqual(PlayerState->GetInventory()->GetItemCount(FPrimaryAssetId(UVestigeItemData::AssetType, TEXT("Base.Turnip"))), 4));
		}

		/** Stands the farmer at the offset from the bin, aiming at it, and lets the interaction scan. */
		void StandAtTheBin(const FVector& Offset)
		{
			Farmer->SetActorLocation(BinLocation + Offset);
			Farmer->FindComponentByClass<UFarmTargetingComponent>()->AimAt(-Offset.GetSafeNormal());
			Farmer->FindComponentByClass<UVestigeInteractionComponent>()->Scan();
		}

		TEST_METHOD(AFarmerAtTheBinFacingItFocusesIt)
		{
			SetUpPlayer(true);
			AFarmShippingBin& Bin = PlaceBin();
			SetBag({{0, TEXT("Base.Turnip"), 4}});
			const UVestigeInteractionComponent& Interaction = *Farmer->FindComponentByClass<UVestigeInteractionComponent>();
			// Against each face of the 90 cm hopper (45 cm out, the capsule 42 cm), in the middle and 50 cm off to one side. The
			// aim snaps to the four directions, so against a corner the bin is beside the ray and can not be focused: the farmer
			// steps along the face (checked by hand).
			const FVector Offsets[] = {FVector(-87.f, 0.f, 0.f), FVector(87.f, 0.f, 0.f), FVector(0.f, 87.f, 0.f), FVector(0.f, -87.f, 0.f),
				FVector(-87.f, 50.f, 0.f), FVector(87.f, -50.f, 0.f), FVector(50.f, 87.f, 0.f), FVector(-50.f, -87.f, 0.f)};
			for (const FVector& Offset : Offsets)
			{
				StandAtTheBin(Offset);

				ASSERT_THAT(IsTrue(Interaction.GetFocusedActor() == &Bin, Offset.ToString()));
				ASSERT_THAT(IsTrue(Interaction.GetFocusedOption().Action == AFarmShippingBin::ShipAction, Offset.ToString()));
			}
		}

		TEST_METHOD(TheBinBlueprintHasItsMesh)
		{
			AFarmShippingBin& Bin = PlaceBin();

			const UStaticMeshComponent* Mesh = Bin.FindComponentByClass<UStaticMeshComponent>();
			ASSERT_THAT(IsNotNull(Mesh));
			ASSERT_THAT(IsNotNull(Mesh->GetStaticMesh().Get()));
			ASSERT_THAT(AreEqual(Mesh->GetStaticMesh()->GetName(), FString(TEXT("SM_ShippingBin"))));
		}

		// ---- The night ----

		/** Sets the clock to Hour on day 1, the way loading a save does, without broadcasting anything. */
		void SetClock(float Hour, int32 Day = 1)
		{
			FVestigeClockSaveData Data;
			Data.Day = Day;
			Data.TimeOfDay = Hour;
			Clock->RestoreFromSaveData(Data);
		}

		/** The first report with clauses: the day report. */
		const FVestigeFeedbackReport* FindDayReport() const
		{
			return Reports.FindByPredicate(
				[](const FVestigeFeedbackReport& Report)
				{
					return !Report.Clauses.IsEmpty();
				});
		}

		/** The Amount of the day report's Paid clause, 0 for an empty clause, INDEX_NONE for no such clause. */
		int32 GetPaid(const FVestigeFeedbackReport& Report) const
		{
			for (const FVestigeFeedbackClause& Clause : Report.Clauses)
			{
				if (Clause.Argument != TEXT("Paid"))
				{
					continue;
				}
				if (!Clause.Tag.IsValid())
				{
					return 0;
				}
				const FFormatArgumentValue* Amount = Clause.Arguments.Find(TEXT("Amount"));
				return Amount ? static_cast<int32>(Amount->GetIntValue()) : INDEX_NONE;
			}
			return INDEX_NONE;
		}

		TEST_METHOD(SleepingSellsTheShipment)
		{
			SetUpPlayer(false);
			Ship(MakeItem(TEXT("Test.Turnip"), 25), 10);
			const int32 Before = Wallet->GetMoney();
			SetClock(22.f);

			ASSERT_THAT(IsTrue(Clock->EndDay()));

			ASSERT_THAT(AreEqual(Wallet->GetMoney(), Before + 250));
			ASSERT_THAT(IsTrue(Shipment->GetShippedStacks().IsEmpty()));
			ASSERT_THAT(AreEqual(Clock->GetCurrentDay(), 2));
		}

		TEST_METHOD(StayingUpAllNightSellsTheShipment)
		{
			SetUpPlayer(false);
			Ship(MakeItem(TEXT("Test.Turnip"), 25), 10);
			const int32 Before = Wallet->GetMoney();
			SetClock(22.f);

			Clock->Tick(Clock->GetDayEndHour() * Clock->GetRealSecondsPerGameHour());

			ASSERT_THAT(AreEqual(Clock->GetCurrentDay(), 2));
			ASSERT_THAT(AreEqual(Wallet->GetMoney(), Before + 250));
			ASSERT_THAT(IsTrue(Shipment->GetShippedStacks().IsEmpty()));
		}

		TEST_METHOD(JumpingToADaySellsNothing)
		{
			SetUpPlayer(false);
			Ship(MakeItem(TEXT("Test.Turnip"), 25), 10);
			const int32 Before = Wallet->GetMoney();

			ASSERT_THAT(IsTrue(Clock->JumpToDay(Clock->GetCurrentDay() + 1)));

			ASSERT_THAT(AreEqual(Wallet->GetMoney(), Before));
			ASSERT_THAT(AreEqual(Shipment->GetShippedStacks().Num(), 1));
		}

		TEST_METHOD(TheDayReportCarriesWhatTheBinPaid)
		{
			SetUpPlayer(false);
			Ship(MakeItem(TEXT("Test.Turnip"), 25), 10);
			const int32 Before = Wallet->GetMoney();
			SetClock(22.f);

			Clock->EndDay();

			const FVestigeFeedbackReport* Report = FindDayReport();
			ASSERT_THAT(IsNotNull(Report));
			ASSERT_THAT(IsTrue(Report->Tag == VestigeGameplayTags::Feedback_Day_Slept));
			ASSERT_THAT(AreEqual(Report->Clauses.Num(), 1));
			ASSERT_THAT(IsTrue(Report->Clauses[0].Tag == VestigeGameplayTags::Feedback_Shipping_Paid));
			ASSERT_THAT(AreEqual(GetPaid(*Report), 250));
			// The sale ran before the report, so the player is told of money already in the wallet.
			ASSERT_THAT(AreEqual(MoneyAtTheDayReport, Before + 250));
		}

		TEST_METHOD(AMorningWithoutASaleHasAnEmptyPaidClause)
		{
			SetUpPlayer(false);
			SetClock(22.f);

			Clock->EndDay();

			const FVestigeFeedbackReport* Report = FindDayReport();
			ASSERT_THAT(IsNotNull(Report));
			ASSERT_THAT(AreEqual(Report->Clauses.Num(), 1));
			ASSERT_THAT(IsTrue(Report->Clauses[0].Argument == FName(TEXT("Paid"))));
			ASSERT_THAT(IsFalse(Report->Clauses[0].Tag.IsValid()));
		}

		TEST_METHOD(EveryKindOfDayReportHasThePaidClause)
		{
			SetUpPlayer(false);
			const int32 Days = Clock->GetCalendar().GetDaysPerSeason();
			ASSERT_THAT(IsTrue(Clock->GetCalendar().GetSeasons().Num() > 1));

			// A mid-season night slept, one stayed up, a season's last day and a season's first.
			SetClock(22.f, 3);
			Clock->EndDay();
			SetClock(22.f, 3);
			Clock->Tick(Clock->GetDayEndHour() * Clock->GetRealSecondsPerGameHour());
			SetClock(22.f, Days - 1);
			Clock->EndDay();
			SetClock(22.f, Days);
			Clock->EndDay();
			SetClock(22.f, Days);
			Clock->Tick(Clock->GetDayEndHour() * Clock->GetRealSecondsPerGameHour());

			int32 DayReports = 0;
			for (const FVestigeFeedbackReport& Report : Reports)
			{
				if (!Report.Clauses.IsEmpty())
				{
					ASSERT_THAT(AreEqual(Report.Clauses.Num(), 1));
					ASSERT_THAT(AreEqual(GetPaid(Report), 0));
					++DayReports;
				}
			}
			ASSERT_THAT(AreEqual(DayReports, 5));
		}

		/** The day messages of a night's report formatted with the game's own responses, with and without the sale. */
		TEST_METHOD(TheMorningMessageEndsWithTheBinsPay)
		{
			SetUpPlayer(false);
			Ship(MakeItem(TEXT("Test.Turnip"), 25), 10);
			SetClock(22.f);
			Clock->EndDay();
			const FVestigeFeedbackReport* Sale = FindDayReport();
			ASSERT_THAT(IsNotNull(Sale));
			FVestigeFeedbackReport WithSale = *Sale;
			FVestigeFeedbackReport WithoutSale = *Sale;
			WithoutSale.Clauses[0].Tag = FGameplayTag();
			WithoutSale.Clauses[0].Arguments.Reset();

			const UClass* GameModeClass = LoadClass<AGameModeBase>(nullptr, GameModePath);
			ASSERT_THAT(IsNotNull(GameModeClass));
			const UClass* HudClass = GameModeClass->GetDefaultObject<AGameModeBase>()->HUDClass;
			ASSERT_THAT(IsTrue(HudClass && HudClass->IsChildOf<AVestigeHUD>()));
			const UVestigeFeedbackResponses* Responses = HudClass->GetDefaultObject<AVestigeHUD>()->GetFeedbackResponses();
			ASSERT_THAT(IsNotNull(Responses));
			const FVestigeFeedbackResponse* Response = Responses->FindResponse(WithSale.Tag);
			ASSERT_THAT(IsNotNull(Response));

			const FString Paid = Responses->FormatMessage(*Response, WithSale).ToString();
			const FString Usual = Responses->FormatMessage(*Response, WithoutSale).ToString();

			ASSERT_THAT(IsTrue(Paid.EndsWith(TEXT("The bin paid 250.")), Paid));
			ASSERT_THAT(AreEqual(Paid, Usual + TEXT(" The bin paid 250.")));
			// A morning without a sale reads as it did before the bin: no mention of it and no trailing space.
			ASSERT_THAT(IsFalse(Usual.Contains(TEXT("bin"))));
			ASSERT_THAT(AreEqual(Usual, Usual.TrimEnd()));
			ASSERT_THAT(IsFalse(Usual.IsEmpty()));
		}

		TEST_METHOD(TheWitheringLineStillFollowsTheDayMessage)
		{
			SetUpPlayer(false);
			ASSERT_THAT(IsTrue(Clock->GetCalendar().GetSeasons().Num() > 1));
			UFarmFieldSubsystem* Fields = UFarmFieldSubsystem::Get(&Spawner.GetWorld());
			const UFarmCropData* Turnip =
				Catalog->Find<UFarmCropData>(FPrimaryAssetId(UFarmCropData::AssetType, TEXT("Base.Turnip")));
			ASSERT_THAT(IsNotNull(Fields));
			ASSERT_THAT(IsNotNull(Turnip));
			UFarmField& Field = Fields->FindOrAddField(TEXT("Base.Farm"), FIntPoint(4, 4));
			ASSERT_THAT(IsTrue(Field.Till(FIntPoint(1, 1))));
			ASSERT_THAT(IsTrue(Field.Plant(FIntPoint(1, 1), Turnip)));
			Ship(MakeItem(TEXT("Test.Turnip"), 25), 10);
			SetClock(22.f, Clock->GetCalendar().GetDaysPerSeason());

			Clock->EndDay();

			// The day message with what the bin paid, then the withered line.
			ASSERT_THAT(IsTrue(Reports.Num() >= 2));
			ASSERT_THAT(IsTrue(Reports[0].Tag == VestigeGameplayTags::Feedback_Day_Slept_NewSeason));
			ASSERT_THAT(AreEqual(GetPaid(Reports[0]), 250));
			ASSERT_THAT(IsTrue(Reports[1].Tag == VestigeGameplayTags::Feedback_Farm_Withered));
		}

		TEST_METHOD(WithoutAWalletTheShipmentWaits)
		{
			// No player state, no controller: nobody to pay.
			Ship(MakeItem(TEXT("Test.Turnip"), 25), 10);
			SetClock(22.f);
			TestRunner->AddExpectedMessagePlain(TEXT("no wallet to pay"), ELogVerbosity::Warning);

			Clock->EndDay();

			ASSERT_THAT(AreEqual(Clock->GetCurrentDay(), 2));
			ASSERT_THAT(AreEqual(Shipment->GetShippedStacks().Num(), 1));
			const FVestigeFeedbackReport* Report = FindDayReport();
			ASSERT_THAT(IsNotNull(Report));
			ASSERT_THAT(AreEqual(GetPaid(*Report), 0));
		}

		TEST_METHOD(TheMorningsRecordsHaveTheCoinsAndAnEmptyBin)
		{
			SetUpPlayer(false);
			Ship(MakeItem(TEXT("Test.Turnip"), 25), 10);
			const int32 Before = Wallet->GetMoney();
			SetClock(22.f);

			Clock->EndDay();

			const TMap<FName, FVestigeSaveRecord>& Records = UVestigeSaveRegistry::Get(&Spawner.GetWorld())->CollectRecords();
			const FVestigeSaveRecord* WalletRecord = Records.Find(TEXT("Base.Wallet"));
			const FVestigeSaveRecord* ShippingRecord = Records.Find(TEXT("Base.Shipping"));
			ASSERT_THAT(IsNotNull(WalletRecord));
			ASSERT_THAT(IsNotNull(ShippingRecord));
			FVestigeWalletSaveData SavedWallet;
			ASSERT_THAT(IsTrue(WalletRecord->Read(SavedWallet)));
			FFarmShippingSaveData SavedShipping;
			ASSERT_THAT(IsTrue(ShippingRecord->Read(SavedShipping)));
			ASSERT_THAT(AreEqual(SavedWallet.Money, Before + 250));
			ASSERT_THAT(IsTrue(SavedShipping.Stacks.IsEmpty()));
		}
	};
}

#endif
