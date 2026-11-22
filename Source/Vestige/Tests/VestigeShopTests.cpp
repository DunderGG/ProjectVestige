// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/VestigeGameplayTags.h"
#include "Components/ActorTestSpawner.h"
#include "Content/VestigeContentCatalog.h"
#include "Content/VestigeItemData.h"
#include "Content/VestigeShopData.h"
#include "Core/VestigeFeedbackSubsystem.h"
#include "CQTest.h"
#include "Farming/FarmGrid.h"
#include "Farming/FarmShopDispenser.h"
#include "Farming/FarmTargetingComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/VestigeInteractionComponent.h"
#include "Inventory/VestigeInventoryComponent.h"
#include "Inventory/VestigeInventorySaveData.h"
#include "Inventory/VestigeShop.h"
#include "Inventory/VestigeWalletComponent.h"
#include "Player/VestigePlayerCharacter.h"
#include "Player/VestigePlayerState.h"
#include "UI/VestigeFeedbackResponses.h"
#include "UI/VestigeHUD.h"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace VestigeShopTests
{
	const TCHAR* const PlayerStatePath = TEXT("/Game/Vestige/Blueprints/BP_PlayerState.BP_PlayerState_C");
	const TCHAR* const FarmerPath = TEXT("/Game/Vestige/Blueprints/BP_PlayerCharacter.BP_PlayerCharacter_C");
	const TCHAR* const GameModePath = TEXT("/Game/Vestige/Blueprints/BP_GameMode.BP_GameMode_C");

	/** The buying rule (VestigeShop::Buy) and the seed dispenser that uses it. */
	TEST_CLASS(Shop, "Vestige")
	{
		FActorTestSpawner Spawner;
		UVestigeContentCatalog* Catalog = nullptr;
		AVestigePlayerState* PlayerState = nullptr;
		AVestigePlayerCharacter* Farmer = nullptr;
		UVestigeWalletComponent* Wallet = nullptr;
		UVestigeInventoryComponent* Inventory = nullptr;
		TArray<FVestigeFeedbackReport> Reports;

		BEFORE_EACH()
		{
			Spawner.InitializeGameSubsystems();
			Catalog = UVestigeContentCatalog::Get(&Spawner.GetWorld());
			ASSERT_THAT(IsNotNull(Catalog));

			UVestigeFeedbackSubsystem* Feedback = Spawner.GetWorld().GetSubsystem<UVestigeFeedbackSubsystem>();
			ASSERT_THAT(IsNotNull(Feedback));
			Feedback->OnFeedback.AddLambda(
				[this](const FVestigeFeedbackReport& Report)
				{
					Reports.Add(Report);
				});
		}

		// ---- Setting up the rule ----

		/** A test item in the catalogue. */
		const UVestigeItemData& MakeItem(FName ContentId, int32 StackSize = 99)
		{
			UVestigeItemData& Item = Spawner.SpawnObject<UVestigeItemData>();
			Item.SetContentIdForTests(ContentId);
			Item.SetMaxStackSizeForTests(StackSize);
			Catalog->AddForTests(Item);
			return Item;
		}

		static FVestigeShopOffer MakeOffer(const UVestigeItemData& Item, int32 Price)
		{
			FVestigeShopOffer Offer;
			Offer.ItemId = Item.GetPrimaryAssetId();
			Offer.Price = Price;
			return Offer;
		}

		/** A bag of its own with Slots slots, not the player's. */
		UVestigeInventoryComponent& MakeBag(int32 Slots = 4)
		{
			AActor& Owner = Spawner.SpawnActor<AActor>();
			UVestigeInventoryComponent* Bag = NewObject<UVestigeInventoryComponent>(&Owner);
			Bag->ResetForTests(Slots);
			return *Bag;
		}

		/** A wallet of its own holding Money, not the player's. */
		UVestigeWalletComponent& MakeWallet(int32 Money)
		{
			AActor& Owner = Spawner.SpawnActor<AActor>();
			UVestigeWalletComponent& Purse = *NewObject<UVestigeWalletComponent>(&Owner);
			if (Money > 0)
			{
				verify(Purse.AddMoney(Money));
			}
			return Purse;
		}

		/** A bag of one slot already full of something else that doesn't stack. */
		UVestigeInventoryComponent& MakeFullBag()
		{
			UVestigeInventoryComponent& Bag = MakeBag(1);
			verify(Bag.AddItem(MakeItem(TEXT("Test.Keepsake"), 1).GetPrimaryAssetId(), 1));
			return Bag;
		}

		/** Counts the wallet's OnMoneyChanged broadcasts into Broadcasts. */
		static void CountBroadcasts(UVestigeWalletComponent& Purse, int32& Broadcasts)
		{
			Purse.OnMoneyChanged.AddLambda(
				[&Broadcasts]()
				{
					++Broadcasts;
				});
		}

		// ---- The rule ----

		TEST_METHOD(BuyingAddsTheItemAndTakesThePrice)
		{
			const UVestigeItemData& Seeds = MakeItem(TEXT("Test.Seeds"));
			UVestigeInventoryComponent& Bag = MakeBag();
			UVestigeWalletComponent& Purse = MakeWallet(100);
			int32 Broadcasts = 0;
			CountBroadcasts(Purse, Broadcasts);

			ASSERT_THAT(IsTrue(VestigeShop::Buy(Bag, Purse, MakeOffer(Seeds, 10), 1) == EVestigeBuyResult::Bought));

			ASSERT_THAT(AreEqual(Bag.GetItemCount(Seeds.GetPrimaryAssetId()), 1));
			ASSERT_THAT(AreEqual(Purse.GetMoney(), 90));
			ASSERT_THAT(AreEqual(Broadcasts, 1));
		}

		TEST_METHOD(ABuyCostsPriceTimesCount)
		{
			const UVestigeItemData& Seeds = MakeItem(TEXT("Test.Seeds"));
			UVestigeInventoryComponent& Bag = MakeBag();
			UVestigeWalletComponent& Purse = MakeWallet(100);

			ASSERT_THAT(IsTrue(VestigeShop::Buy(Bag, Purse, MakeOffer(Seeds, 20), 3) == EVestigeBuyResult::Bought));

			ASSERT_THAT(AreEqual(Bag.GetItemCount(Seeds.GetPrimaryAssetId()), 3));
			ASSERT_THAT(AreEqual(Purse.GetMoney(), 40));
		}

		TEST_METHOD(ACostAboveThePurseLimitIsNotEnoughMoney)
		{
			// Twice the limit doesn't fit an int32 clamped or not; clamped, a full purse would pay for it.
			const UVestigeItemData& Gem = MakeItem(TEXT("Test.Gem"));
			UVestigeInventoryComponent& Bag = MakeBag();
			UVestigeWalletComponent& Purse = MakeWallet(VestigeWallet::MaxMoney);

			ASSERT_THAT(
				IsTrue(VestigeShop::Buy(Bag, Purse, MakeOffer(Gem, VestigeWallet::MaxMoney), 2) == EVestigeBuyResult::NotEnoughMoney));

			ASSERT_THAT(AreEqual(Purse.GetMoney(), VestigeWallet::MaxMoney));
			ASSERT_THAT(AreEqual(Bag.GetItemCount(Gem.GetPrimaryAssetId()), 0));
		}

		TEST_METHOD(AFullBagBuysNothingAndCostsNothing)
		{
			const UVestigeItemData& Seeds = MakeItem(TEXT("Test.Seeds"));
			UVestigeInventoryComponent& Bag = MakeFullBag();
			UVestigeWalletComponent& Purse = MakeWallet(100);
			int32 Broadcasts = 0;
			CountBroadcasts(Purse, Broadcasts);

			ASSERT_THAT(IsTrue(VestigeShop::Buy(Bag, Purse, MakeOffer(Seeds, 10), 1) == EVestigeBuyResult::NoRoom));

			ASSERT_THAT(AreEqual(Purse.GetMoney(), 100));
			ASSERT_THAT(AreEqual(Broadcasts, 0));
			ASSERT_THAT(AreEqual(Bag.GetItemCount(Seeds.GetPrimaryAssetId()), 0));
		}

		TEST_METHOD(AShortPurseBuysNothing)
		{
			const UVestigeItemData& Seeds = MakeItem(TEXT("Test.Seeds"));
			UVestigeInventoryComponent& Bag = MakeBag();
			UVestigeWalletComponent& Purse = MakeWallet(9);
			int32 Broadcasts = 0;
			CountBroadcasts(Purse, Broadcasts);

			ASSERT_THAT(IsTrue(VestigeShop::Buy(Bag, Purse, MakeOffer(Seeds, 10), 1) == EVestigeBuyResult::NotEnoughMoney));

			ASSERT_THAT(AreEqual(Purse.GetMoney(), 9));
			ASSERT_THAT(AreEqual(Broadcasts, 0));
			ASSERT_THAT(AreEqual(Bag.GetItemCount(Seeds.GetPrimaryAssetId()), 0));
		}

		TEST_METHOD(TheBagIsCheckedBeforeThePurse)
		{
			const UVestigeItemData& Seeds = MakeItem(TEXT("Test.Seeds"));
			UVestigeInventoryComponent& Bag = MakeFullBag();
			UVestigeWalletComponent& Purse = MakeWallet(0);

			ASSERT_THAT(IsTrue(VestigeShop::Buy(Bag, Purse, MakeOffer(Seeds, 10), 1) == EVestigeBuyResult::NoRoom));
		}

		TEST_METHOD(BuyingWithExactlyThePriceLeavesZero)
		{
			const UVestigeItemData& Seeds = MakeItem(TEXT("Test.Seeds"));
			UVestigeInventoryComponent& Bag = MakeBag();
			UVestigeWalletComponent& Purse = MakeWallet(40);

			ASSERT_THAT(IsTrue(VestigeShop::Buy(Bag, Purse, MakeOffer(Seeds, 40), 1) == EVestigeBuyResult::Bought));

			ASSERT_THAT(AreEqual(Purse.GetMoney(), 0));
			ASSERT_THAT(AreEqual(Bag.GetItemCount(Seeds.GetPrimaryAssetId()), 1));
		}

		TEST_METHOD(ACountOrPriceBelowOneIsInvalid)
		{
			const UVestigeItemData& Seeds = MakeItem(TEXT("Test.Seeds"));
			UVestigeInventoryComponent& Bag = MakeBag();
			UVestigeWalletComponent& Purse = MakeWallet(100);
			int32 Broadcasts = 0;
			CountBroadcasts(Purse, Broadcasts);

			ASSERT_THAT(IsTrue(VestigeShop::Buy(Bag, Purse, MakeOffer(Seeds, 10), 0) == EVestigeBuyResult::InvalidOffer));
			ASSERT_THAT(IsTrue(VestigeShop::Buy(Bag, Purse, MakeOffer(Seeds, 10), -2) == EVestigeBuyResult::InvalidOffer));
			// A missing price is never "free".
			ASSERT_THAT(IsTrue(VestigeShop::Buy(Bag, Purse, MakeOffer(Seeds, 0), 1) == EVestigeBuyResult::InvalidOffer));
			ASSERT_THAT(IsTrue(VestigeShop::Buy(Bag, Purse, MakeOffer(Seeds, -5), 1) == EVestigeBuyResult::InvalidOffer));

			ASSERT_THAT(AreEqual(Purse.GetMoney(), 100));
			ASSERT_THAT(AreEqual(Broadcasts, 0));
			ASSERT_THAT(AreEqual(Bag.GetItemCount(Seeds.GetPrimaryAssetId()), 0));
		}

		TEST_METHOD(BuyingIntoAPartStackMergesIt)
		{
			const UVestigeItemData& Seeds = MakeItem(TEXT("Test.Seeds"), 10);
			UVestigeInventoryComponent& Bag = MakeBag();
			ASSERT_THAT(IsTrue(Bag.AddItem(Seeds.GetPrimaryAssetId(), 3)));
			UVestigeWalletComponent& Purse = MakeWallet(100);

			ASSERT_THAT(IsTrue(VestigeShop::Buy(Bag, Purse, MakeOffer(Seeds, 10), 2) == EVestigeBuyResult::Bought));

			const TArray<FVestigeInventorySlot>& Slots = Bag.GetSlots();
			ASSERT_THAT(IsTrue(Slots[0].Item == &Seeds));
			ASSERT_THAT(AreEqual(Slots[0].Count, 5));
			ASSERT_THAT(AreEqual(Slots[1].Count, 0));
		}

		TEST_METHOD(AnItemThatDoesNotExistIsNoRoomAndCostsNothing)
		{
			UVestigeInventoryComponent& Bag = MakeBag();
			UVestigeWalletComponent& Purse = MakeWallet(100);
			FVestigeShopOffer Offer;
			Offer.ItemId = FPrimaryAssetId(UVestigeItemData::AssetType, TEXT("Gone.Seeds"));
			Offer.Price = 10;
			TestRunner->AddExpectedMessagePlain(TEXT("No item has the ID"), ELogVerbosity::Warning);

			ASSERT_THAT(IsTrue(VestigeShop::Buy(Bag, Purse, Offer, 1) == EVestigeBuyResult::NoRoom));

			ASSERT_THAT(AreEqual(Purse.GetMoney(), 100));
		}

		// ---- The dispenser ----

		/** A BP_PlayerState in play with a controller that has it, and the Blueprint farmer it possesses. */
		void SetUpPlayer()
		{
			UClass* PlayerStateClass = LoadClass<AVestigePlayerState>(nullptr, PlayerStatePath);
			check(PlayerStateClass);
			PlayerState = &Spawner.SpawnActor<AVestigePlayerState>(FActorSpawnParameters(), PlayerStateClass);
			PlayerState->DispatchBeginPlay();
			Wallet = PlayerState->GetWallet();
			Inventory = PlayerState->GetInventory();
			check(Wallet);
			check(Inventory);

			APlayerController& Controller = Spawner.SpawnActor<APlayerController>();
			Controller.PlayerState = PlayerState;
			// The farmer's facing comes from the farm grid, so the grid is in play before he is.
			Spawner.SpawnActor<AFarmGrid>().DispatchBeginPlay();
			UClass* FarmerClass = LoadClass<AVestigePlayerCharacter>(nullptr, FarmerPath);
			check(FarmerClass);
			Farmer = &Spawner.SpawnActor<AVestigePlayerCharacter>(FActorSpawnParameters(), FarmerClass);
			Farmer->DispatchBeginPlay();
			Controller.Possess(Farmer);
			Farmer->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		}

		static FPrimaryAssetId FarmShopId() { return FPrimaryAssetId(UVestigeShopData::AssetType, TEXT("Base.FarmShop")); }
		static FPrimaryAssetId ItemId(const TCHAR* ContentId) { return FPrimaryAssetId(UVestigeItemData::AssetType, ContentId); }

		/** A dispenser of the C++ class selling ItemOfShop of ShopOfDispenser, in play at Location facing -X, as in L_Farm. */
		AFarmShopDispenser& PlaceDispenser(const FPrimaryAssetId& ItemOfShop, const FVector& Location = FVector(1880.f, 900.f, 0.f),
			const FPrimaryAssetId& ShopOfDispenser = FarmShopId())
		{
			AFarmShopDispenser& Dispenser = Spawner.SpawnActor<AFarmShopDispenser>();
			Dispenser.SetOfferForTests(ShopOfDispenser, ItemOfShop);
			Dispenser.SetActorLocationAndRotation(Location, FRotator(0.f, 180.f, 0.f));
			Dispenser.DispatchBeginPlay();
			return Dispenser;
		}

		static TArray<FVestigeInteractionOption> GetOptions(AFarmShopDispenser& Dispenser, AActor* Interactor)
		{
			TArray<FVestigeInteractionOption> Options;
			IVestigeInteractable::Execute_GetInteractionOptions(&Dispenser, Interactor, Options);
			return Options;
		}

		void Press(AFarmShopDispenser& Dispenser, FName Action = AFarmShopDispenser::BuyAction)
		{
			FVestigeInteractionOption Option;
			Option.Action = Action;
			IVestigeInteractable::Execute_Interact(&Dispenser, Farmer, Option);
		}

		/** Restores the player's bag to exactly these slots, as loading a save does. */
		void SetBag(TArray<FVestigeInventorySlotSaveData> Slots)
		{
			FVestigeInventorySaveData Bag;
			Bag.Slots = MoveTemp(Slots);
			verify(Inventory->RestoreFromSaveData(Bag) == 0);
		}

		/** Fills every slot of the player's bag with a hoe, which doesn't stack. */
		void FillBag()
		{
			TArray<FVestigeInventorySlotSaveData> Slots;
			for (int32 Slot = 0; Slot < Inventory->GetSlots().Num(); ++Slot)
			{
				Slots.Add({Slot, TEXT("Base.Hoe"), 1});
			}
			SetBag(MoveTemp(Slots));
		}

		/** Empties the player's purse and puts Money in it. */
		void SetMoney(int32 Money)
		{
			if (Wallet->GetMoney() > 0)
			{
				verify(Wallet->SpendMoney(Wallet->GetMoney()));
			}
			if (Money > 0)
			{
				verify(Wallet->AddMoney(Money));
			}
		}

		static int32 GetIntArgument(const FVestigeFeedbackReport& Report, const TCHAR* Name)
		{
			const FFormatArgumentValue* Value = Report.Arguments.Find(Name);
			return Value ? static_cast<int32>(Value->GetIntValue()) : INDEX_NONE;
		}

		TEST_METHOD(TheDispenserOffersBuyWithTheItemAndPrice)
		{
			SetUpPlayer();
			AFarmShopDispenser& Dispenser = PlaceDispenser(ItemId(TEXT("Base.TurnipSeeds")));
			// Offered whatever the purse holds, so a press can say why nothing is bought.
			SetMoney(0);

			const TArray<FVestigeInteractionOption> Options = GetOptions(Dispenser, Farmer);

			ASSERT_THAT(AreEqual(Options.Num(), 1));
			ASSERT_THAT(IsTrue(Options[0].Action == AFarmShopDispenser::BuyAction));
			ASSERT_THAT(IsFalse(Options[0].bHasLocation));
			ASSERT_THAT(AreEqual(Options[0].Text.ToString(), FString(TEXT("Buy Turnip Seeds (10)"))));
		}

		TEST_METHOD(TheDispenserOffersNothingWithoutABagOrAWallet)
		{
			AFarmShopDispenser& Dispenser = PlaceDispenser(ItemId(TEXT("Base.TurnipSeeds")));

			ASSERT_THAT(IsTrue(GetOptions(Dispenser, &Spawner.SpawnActor<AActor>()).IsEmpty()));
			ASSERT_THAT(IsTrue(GetOptions(Dispenser, nullptr).IsEmpty()));
			// A pawn with no player state has neither.
			ASSERT_THAT(IsTrue(GetOptions(Dispenser, &Spawner.SpawnActor<APawn>()).IsEmpty()));
		}

		TEST_METHOD(ADispenserWithoutItsOfferOffersNothingAndWarns)
		{
			SetUpPlayer();
			TestRunner->AddExpectedMessagePlain(
				TEXT("so it sells nothing"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 3);

			// The shop doesn't offer the crop; the shop doesn't exist; the item doesn't exist.
			AFarmShopDispenser& NotOffered = PlaceDispenser(ItemId(TEXT("Base.Turnip")));
			AFarmShopDispenser& NoShop = PlaceDispenser(ItemId(TEXT("Base.TurnipSeeds")), FVector(1880.f, 900.f, 0.f),
				FPrimaryAssetId(UVestigeShopData::AssetType, TEXT("Gone.Shop")));
			AFarmShopDispenser& NoItem = PlaceDispenser(ItemId(TEXT("Gone.Seeds")));
			const int32 Before = Wallet->GetMoney();

			for (AFarmShopDispenser* Dispenser : {&NotOffered, &NoShop, &NoItem})
			{
				ASSERT_THAT(IsTrue(GetOptions(*Dispenser, Farmer).IsEmpty()));
				// A missing offer is never free: pressing anyway buys nothing and says nothing.
				Press(*Dispenser);
			}
			ASSERT_THAT(AreEqual(Wallet->GetMoney(), Before));
			ASSERT_THAT(IsTrue(Reports.IsEmpty()));
			ASSERT_THAT(AreEqual(Inventory->GetItemCount(ItemId(TEXT("Base.Turnip"))), 0));
		}

		TEST_METHOD(BuyingReportsTheItemCountAndPrice)
		{
			SetUpPlayer();
			AFarmShopDispenser& Dispenser = PlaceDispenser(ItemId(TEXT("Base.PotatoSeeds")), FVector(1880.f, 1000.f, 0.f));
			SetBag({});
			SetMoney(100);

			Press(Dispenser);

			// One per press.
			ASSERT_THAT(AreEqual(Inventory->GetItemCount(ItemId(TEXT("Base.PotatoSeeds"))), 1));
			ASSERT_THAT(AreEqual(Wallet->GetMoney(), 80));
			ASSERT_THAT(AreEqual(Reports.Num(), 1));
			ASSERT_THAT(IsTrue(Reports[0].Tag == VestigeGameplayTags::Feedback_Shop_Bought));
			const FFormatArgumentValue* Item = Reports[0].Arguments.Find(TEXT("Item"));
			ASSERT_THAT(IsNotNull(Item));
			ASSERT_THAT(AreEqual(Item->GetTextValue().ToString(), FString(TEXT("Potato Seeds"))));
			ASSERT_THAT(AreEqual(GetIntArgument(Reports[0], TEXT("Count")), 1));
			ASSERT_THAT(AreEqual(GetIntArgument(Reports[0], TEXT("Price")), 20));
		}

		TEST_METHOD(AShortPurseReportsThePrice)
		{
			SetUpPlayer();
			AFarmShopDispenser& Dispenser = PlaceDispenser(ItemId(TEXT("Base.CabbageSeeds")), FVector(1880.f, 1100.f, 0.f));
			SetBag({});
			SetMoney(39);

			Press(Dispenser);

			ASSERT_THAT(AreEqual(Wallet->GetMoney(), 39));
			ASSERT_THAT(AreEqual(Inventory->GetItemCount(ItemId(TEXT("Base.CabbageSeeds"))), 0));
			ASSERT_THAT(AreEqual(Reports.Num(), 1));
			ASSERT_THAT(IsTrue(Reports[0].Tag == VestigeGameplayTags::Feedback_Shop_NotEnoughMoney));
			ASSERT_THAT(AreEqual(GetIntArgument(Reports[0], TEXT("Price")), 40));
		}

		TEST_METHOD(AFullBagReportsNoRoom)
		{
			SetUpPlayer();
			AFarmShopDispenser& Dispenser = PlaceDispenser(ItemId(TEXT("Base.TurnipSeeds")));
			FillBag();
			// A short purse too: the bag is what the player is told about.
			SetMoney(0);

			Press(Dispenser);

			ASSERT_THAT(AreEqual(Reports.Num(), 1));
			ASSERT_THAT(IsTrue(Reports[0].Tag == VestigeGameplayTags::Feedback_Inventory_Full));
			ASSERT_THAT(AreEqual(Inventory->GetItemCount(ItemId(TEXT("Base.TurnipSeeds"))), 0));
		}

		/** The game's own responses turn the dispenser's reports into GameDesign.md's texts. */
		TEST_METHOD(TheShopMessagesReadAsDesigned)
		{
			SetUpPlayer();
			AFarmShopDispenser& Dispenser = PlaceDispenser(ItemId(TEXT("Base.TurnipSeeds")));
			SetBag({});
			SetMoney(15);
			Press(Dispenser);
			Press(Dispenser);
			FillBag();
			Press(Dispenser);
			ASSERT_THAT(AreEqual(Reports.Num(), 3));

			const UClass* GameModeClass = LoadClass<AGameModeBase>(nullptr, GameModePath);
			ASSERT_THAT(IsNotNull(GameModeClass));
			const UClass* HudClass = GameModeClass->GetDefaultObject<AGameModeBase>()->HUDClass;
			ASSERT_THAT(IsTrue(HudClass && HudClass->IsChildOf<AVestigeHUD>()));
			const UVestigeFeedbackResponses* Responses = HudClass->GetDefaultObject<AVestigeHUD>()->GetFeedbackResponses();
			ASSERT_THAT(IsNotNull(Responses));
			const TCHAR* const Expected[] = {
				TEXT("Bought: Turnip Seeds \u00D71 for 10."), TEXT("Not enough coins. These seeds cost 10."), TEXT("No room in your bag.")};
			for (int32 Index = 0; Index < 3; ++Index)
			{
				const FVestigeFeedbackResponse* Response = Responses->FindResponse(Reports[Index].Tag);
				ASSERT_THAT(IsNotNull(Response, Reports[Index].Tag.ToString()));
				ASSERT_THAT(AreEqual(Responses->FormatMessage(*Response, Reports[Index]).ToString(), FString(Expected[Index])));
			}
		}

		TEST_METHOD(AnotherActionAtTheDispenserDoesNothing)
		{
			SetUpPlayer();
			AFarmShopDispenser& Dispenser = PlaceDispenser(ItemId(TEXT("Base.TurnipSeeds")));
			SetBag({});
			SetMoney(100);

			Press(Dispenser, TEXT("Ship"));

			ASSERT_THAT(IsTrue(Reports.IsEmpty()));
			ASSERT_THAT(AreEqual(Wallet->GetMoney(), 100));
			ASSERT_THAT(AreEqual(Inventory->GetItemCount(ItemId(TEXT("Base.TurnipSeeds"))), 0));
		}

		TEST_METHOD(AFarmerFacingADispenserFocusesItNotItsNeighbour)
		{
			SetUpPlayer();
			// The row in L_Farm: X 1880, Y 900, 1000 and 1100, fronts towards -X.
			const TCHAR* const Seeds[] = {TEXT("Base.TurnipSeeds"), TEXT("Base.PotatoSeeds"), TEXT("Base.CabbageSeeds")};
			TArray<AFarmShopDispenser*> Row;
			for (int32 Index = 0; Index < 3; ++Index)
			{
				Row.Add(&PlaceDispenser(ItemId(Seeds[Index]), FVector(1880.f, 900.f + 100.f * Index, 0.f)));
			}
			const UVestigeInteractionComponent& Interaction = *Farmer->FindComponentByClass<UVestigeInteractionComponent>();
			UFarmTargetingComponent& Targeting = *Farmer->FindComponentByClass<UFarmTargetingComponent>();

			auto StandInFrontAt = [this, &Targeting](float Y)
			{
				// Against the front: the cabinet's 45 cm and the capsule's 42.
				Farmer->SetActorLocation(FVector(1880.f - 87.f, Y, Farmer->GetActorLocation().Z));
				Targeting.AimAt(FVector(1.f, 0.f, 0.f));
				Farmer->FindComponentByClass<UVestigeInteractionComponent>()->Scan();
			};

			for (int32 Index = 0; Index < 3; ++Index)
			{
				const FString Where = FString::Printf(TEXT("in front of dispenser %d"), Index);
				StandInFrontAt(900.f + 100.f * Index);
				ASSERT_THAT(IsTrue(Interaction.GetFocusedActor() == Row[Index], Where));
				ASSERT_THAT(IsTrue(Interaction.GetFocusedOption().Action == AFarmShopDispenser::BuyAction, Where));
			}

			// Centred on a gap between two: neither, never the wrong one.
			for (const float GapY : {950.f, 1050.f})
			{
				StandInFrontAt(GapY);
				ASSERT_THAT(IsNull(Interaction.GetFocusedActor(), FString::Printf(TEXT("at the gap at Y %.0f"), GapY)));
			}
		}
	};
}

#endif
