// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/VestigeGameplayTags.h"
#include "AbilitySystem/VestigeStaminaSet.h"
#include "AbilitySystemComponent.h"
#include "Components/ActorTestSpawner.h"
#include "Components/StaticMeshComponent.h"
#include "Content/FarmWaterContainerFragment.h"
#include "Content/VestigeGameData.h"
#include "Content/VestigeItemData.h"
#include "Core/VestigeFeedbackSubsystem.h"
#include "Core/VestigeSaveRecord.h"
#include "CQTest.h"
#include "Engine/StaticMesh.h"
#include "Farming/FarmField.h"
#include "Farming/FarmGrid.h"
#include "Farming/FarmTargetingComponent.h"
#include "Farming/FarmTileState.h"
#include "Farming/FarmWaterSaveData.h"
#include "Farming/FarmWaterSettings.h"
#include "Farming/FarmWaterStore.h"
#include "Farming/FarmWaterSubsystem.h"
#include "Farming/FarmWateringCan.h"
#include "Farming/FarmWell.h"
#include "Farming/FarmingComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/VestigeInteractionComponent.h"
#include "Inventory/VestigeInventoryComponent.h"
#include "Inventory/VestigeInventorySaveData.h"
#include "Player/VestigePlayerCharacter.h"
#include "Player/VestigePlayerState.h"
#include "Time/VestigeClockSubsystem.h"
#include "TimerManager.h"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace FarmWaterTests
{
	/** More than one use takes at full stamina (0.4 s), and less than two. */
	constexpr float OneUseSeconds = 0.6f;

	/** The well's supply the tests that count water start from, whatever the project settings say. */
	constexpr int32 TestSupply = 24;

	constexpr int32 CanCapacity = 8;

	/** The farmer's stand, a well and the tiles in front of him are as in the farming tests; the well is placed far from the grid. */
	TEST_CLASS(Water, "Vestige")
	{
		FActorTestSpawner Spawner;
		AFarmGrid* Grid = nullptr;
		AVestigePlayerState* PlayerState = nullptr;
		AVestigePlayerCharacter* Farmer = nullptr;
		UFarmingComponent* FarmingComponent = nullptr;
		UVestigeInventoryComponent* Inventory = nullptr;
		UAbilitySystemComponent* AbilitySystem = nullptr;
		APlayerController* Controller = nullptr;
		TArray<FGameplayTag> Reports;
		int32 SavedSupply = TestSupply;

		BEFORE_EACH()
		{
			// The settings are the default object's, shared by every test, so each starts from the design's supply and puts it back.
			SavedSupply = UFarmWaterSettings::Get().GetWellWaterPerDay();
			SetSupply(TestSupply);

			ASSERT_THAT(IsNotNull(UVestigeGameData::Get().GetStaminaCostEffect().Get()));
			Spawner.InitializeGameSubsystems();
			Grid = &Spawner.SpawnActor<AFarmGrid>();
			Grid->DispatchBeginPlay();

			UClass* PlayerStateClass =
				LoadClass<AVestigePlayerState>(nullptr, TEXT("/Game/Vestige/Blueprints/BP_PlayerState.BP_PlayerState_C"));
			ASSERT_THAT(IsNotNull(PlayerStateClass));
			PlayerState = &Spawner.SpawnActor<AVestigePlayerState>(FActorSpawnParameters(), PlayerStateClass);
			PlayerState->DispatchBeginPlay();
			Inventory = PlayerState->GetInventory();
			ASSERT_THAT(IsNotNull(Inventory));
			AbilitySystem = PlayerState->GetAbilitySystemComponent();
			ASSERT_THAT(IsNotNull(AbilitySystem));

			UClass* FarmerClass =
				LoadClass<AVestigePlayerCharacter>(nullptr, TEXT("/Game/Vestige/Blueprints/BP_PlayerCharacter.BP_PlayerCharacter_C"));
			ASSERT_THAT(IsNotNull(FarmerClass));
			Controller = &Spawner.SpawnActor<APlayerController>();
			Controller->PlayerState = PlayerState;
			Farmer = &Spawner.SpawnActor<AVestigePlayerCharacter>(FActorSpawnParameters(), FarmerClass);
			Farmer->DispatchBeginPlay();
			Farmer->SetActorLocationAndRotation(FVector(250.f, 50.f, 0.f), FRotator::ZeroRotator);
			Controller->Possess(Farmer);
			Farmer->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
			FarmingComponent = Farmer->FindComponentByClass<UFarmingComponent>();
			ASSERT_THAT(IsNotNull(FarmingComponent));
		}

		AFTER_EACH() { SetSupply(SavedSupply); }

		static void SetSupply(int32 Supply) { GetMutableDefault<UFarmWaterSettings>()->SetWellWaterPerDayForTests(Supply); }

		UFarmWaterSubsystem& GetWater() { return *UFarmWaterSubsystem::Get(&Spawner.GetWorld()); }
		UVestigeClockSubsystem& Clock() { return *UVestigeClockSubsystem::Get(&Spawner.GetWorld()); }

		/** The tile the farmer faces at the start. */
		static inline const FIntPoint TargetCoordinates = FIntPoint(3, 0);

		const FFarmTileState* TargetTile() const { return Grid->GetTile(TargetCoordinates); }

		float GetStamina() const { return AbilitySystem->GetNumericAttribute(UVestigeStaminaSet::GetStaminaAttribute()); }
		float GetMaxStamina() const { return AbilitySystem->GetNumericAttribute(UVestigeStaminaSet::GetMaxStaminaAttribute()); }

		/** Restores the bag to exactly these slots, with slot 0 selected, the way loading a save does. */
		void SetBag(TArray<FVestigeInventorySlotSaveData> Slots)
		{
			FVestigeInventorySaveData Bag;
			Bag.Slots = MoveTemp(Slots);
			ASSERT_THAT(IsTrue(Inventory->RestoreFromSaveData(Bag) == 0));
		}

		/** A bag with one watering can in the hand's slot, and the can's water set to Water unless it is left as a new can's. */
		void HoldCan(int32 Water = INDEX_NONE)
		{
			SetBag({{0, TEXT("Base.WateringCan"), 1}});
			if (Water != INDEX_NONE)
			{
				ASSERT_THAT(IsTrue(Inventory->SetSlotStat(0, VestigeGameplayTags::ItemStat_Water_Clean, Water)));
			}
		}

		int32 GetHeldWater() const { return FarmWateringCan::GetWater(*Inventory, 0); }

		void UseToolAndFinish()
		{
			FarmingComponent->UseTool();
			AbilitySystem->CancelAllAbilities();
		}

		void ListenToFeedback()
		{
			UVestigeFeedbackSubsystem* Feedback = Spawner.GetWorld().GetSubsystem<UVestigeFeedbackSubsystem>();
			ASSERT_THAT(IsNotNull(Feedback));
			Feedback->OnFeedback.AddLambda(
				[this](const FVestigeFeedbackReport& Report)
				{
					Reports.Add(Report.Tag);
				});
		}

		/** Lets one use's time pass, then runs Check; only the world's timers advance, as in the farming tests. */
		void AfterOneUse(TFunction<void()> Check)
		{
			const auto TickTimersOnce = [this]()
			{
				FTimerManager& Timers = Spawner.GetWorld().GetTimerManager();
				if (Timers.HasBeenTickedThisFrame())
				{
					return false;
				}
				Timers.Tick(OneUseSeconds);
				return true;
			};
			TestCommandBuilder.Until(TickTimersOnce).Until(TickTimersOnce).Then(MoveTemp(Check));
		}

		// ---- The store's rule, without a world ----

		TEST_METHOD(TheWellGivesItsDailySupply)
		{
			const FFarmWaterStore Store;

			ASSERT_THAT(IsTrue(Store.GetLeft(1, 24) == 24));
			ASSERT_THAT(IsTrue(Store.GetLeft(1, 0) == 0));
			ASSERT_THAT(IsTrue(Store.GetLeft(1, -3) == 0));
		}

		TEST_METHOD(DrawingTakesNoMoreThanIsLeft)
		{
			FFarmWaterStore Store;

			ASSERT_THAT(IsTrue(Store.Draw(5, 24, 20) == 20));
			ASSERT_THAT(IsTrue(Store.GetLeft(5, 24) == 4));
			ASSERT_THAT(IsTrue(Store.Draw(5, 24, 10) == 4));
			ASSERT_THAT(IsTrue(Store.GetLeft(5, 24) == 0));
			ASSERT_THAT(IsTrue(Store.Draw(5, 24, 1) == 0));
		}

		TEST_METHOD(ANewDayStartsFullWithNothingCarriedOver)
		{
			FFarmWaterStore Store;
			Store.Draw(5, 24, 24);
			ASSERT_THAT(IsTrue(Store.GetLeft(5, 24) == 0));

			ASSERT_THAT(IsTrue(Store.GetLeft(6, 24) == 24));
			ASSERT_THAT(IsTrue(Store.Draw(6, 24, 3) == 3));
			ASSERT_THAT(IsTrue(Store.GetLeft(6, 24) == 21));

			// Unused water of a day isn't added to the next: 10 left on day 6 is nothing extra on day 7.
			Store.Draw(6, 24, 11);
			ASSERT_THAT(IsTrue(Store.GetLeft(7, 24) == 24));
		}

		TEST_METHOD(AnEarlierDayStartsFullToo)
		{
			FFarmWaterStore Store;
			Store.Draw(10, 24, 24);

			// A developer's day jump back reads the day, not an event, so it is a full day as well.
			ASSERT_THAT(IsTrue(Store.GetLeft(3, 24) == 24));
			ASSERT_THAT(IsTrue(Store.Draw(3, 24, 5) == 5));
			ASSERT_THAT(IsTrue(Store.GetLeft(3, 24) == 19));
		}

		TEST_METHOD(ANegativeOrZeroDrawTakesNothing)
		{
			FFarmWaterStore Store;

			ASSERT_THAT(IsTrue(Store.Draw(5, 24, 0) == 0));
			ASSERT_THAT(IsTrue(Store.Draw(5, 24, -4) == 0));
			ASSERT_THAT(IsTrue(Store.GetLeft(5, 24) == 24));
			ASSERT_THAT(IsTrue(Store.GetDay() == INDEX_NONE));
		}

		// ---- The subsystem ----

		TEST_METHOD(TheSubsystemDrawsFromTheDaysSupply)
		{
			ASSERT_THAT(IsTrue(GetWater().GetCleanWaterLeft() == TestSupply));

			ASSERT_THAT(IsTrue(GetWater().DrawCleanWater(10) == 10));
			ASSERT_THAT(IsTrue(GetWater().GetCleanWaterLeft() == TestSupply - 10));
			ASSERT_THAT(IsTrue(GetWater().DrawCleanWater(100) == TestSupply - 10));
			ASSERT_THAT(IsTrue(GetWater().GetCleanWaterLeft() == 0));
		}

		TEST_METHOD(TheClocksNewDayRefillsTheWell)
		{
			GetWater().DrawCleanWater(TestSupply);
			ASSERT_THAT(IsTrue(GetWater().GetCleanWaterLeft() == 0));

			ASSERT_THAT(IsTrue(Clock().JumpToDay(Clock().GetCurrentDay() + 1)));

			ASSERT_THAT(IsTrue(GetWater().GetCleanWaterLeft() == TestSupply));
		}

		TEST_METHOD(TheStoreIsSavedAndRestored)
		{
			ASSERT_THAT(IsTrue(GetWater().GetSaveKey() == FName(TEXT("Base.FarmWater"))));
			GetWater().DrawCleanWater(10);
			const FVestigeSaveRecord Record = GetWater().WriteSaveRecord();

			GetWater().StartNewGame();
			ASSERT_THAT(IsTrue(GetWater().GetCleanWaterLeft() == TestSupply));
			int32 LeftOut = 0;
			ASSERT_THAT(IsTrue(GetWater().ReadSaveRecord(Record, LeftOut)));

			ASSERT_THAT(IsTrue(GetWater().GetCleanWaterLeft() == TestSupply - 10));
			ASSERT_THAT(IsTrue(LeftOut == 0));
		}

		TEST_METHOD(ARestoredDayThatIsntTodayIsAFullStore)
		{
			FFarmWaterSaveData Data;
			Data.Day = Clock().GetCurrentDay() - 1;
			Data.CleanWaterDrawn = TestSupply;
			int32 LeftOut = 0;

			ASSERT_THAT(IsTrue(GetWater().ReadSaveRecord(FVestigeSaveRecord::Write(Data), LeftOut)));

			ASSERT_THAT(IsTrue(GetWater().GetCleanWaterLeft() == TestSupply));
		}

		TEST_METHOD(ANewGameStartsTheStoreFull)
		{
			GetWater().DrawCleanWater(TestSupply);

			GetWater().StartNewGame();

			ASSERT_THAT(IsTrue(GetWater().GetCleanWaterLeft() == TestSupply));
		}

		TEST_METHOD(TheSupplyComesFromTheSettings)
		{
			SetSupply(10);
			ASSERT_THAT(IsTrue(GetWater().GetCleanWaterLeft() == 10));

			// A hand-edited ini may hold a negative number: a dry well, not a debt.
			SetSupply(-5);
			ASSERT_THAT(IsTrue(GetWater().GetCleanWaterLeft() == 0));
			ASSERT_THAT(IsTrue(GetWater().DrawCleanWater(3) == 0));
		}

		// ---- The can's rules over a slot ----

		TEST_METHOD(ACanWithoutAStatIsFull)
		{
			HoldCan();

			ASSERT_THAT(IsTrue(FarmWateringCan::GetCapacity(Inventory->GetSlots()[0].Item) == CanCapacity));
			ASSERT_THAT(IsTrue(GetHeldWater() == CanCapacity));
		}

		TEST_METHOD(AnItemThatIsntACanHoldsNoWater)
		{
			SetBag({{0, TEXT("Base.Hoe"), 1}});

			ASSERT_THAT(IsTrue(FarmWateringCan::GetCapacity(Inventory->GetSlots()[0].Item) == INDEX_NONE));
			ASSERT_THAT(IsTrue(FarmWateringCan::GetWater(*Inventory, 0) == INDEX_NONE));
			ASSERT_THAT(IsTrue(FarmWateringCan::GetWater(*Inventory, 1) == INDEX_NONE));
			ASSERT_THAT(IsFalse(FarmWateringCan::TryPour(*Inventory, 0)));
			ASSERT_THAT(IsTrue(FarmWateringCan::Fill(*Inventory, 0, 5) == 0));
			ASSERT_THAT(IsTrue(FarmWateringCan::FindCanToFill(*Inventory) == INDEX_NONE));
		}

		TEST_METHOD(PouringTakesOneAndStopsAtZero)
		{
			HoldCan();

			for (int32 Left = CanCapacity - 1; Left >= 0; --Left)
			{
				ASSERT_THAT(IsTrue(FarmWateringCan::TryPour(*Inventory, 0)));
				ASSERT_THAT(IsTrue(GetHeldWater() == Left));
			}

			ASSERT_THAT(IsFalse(FarmWateringCan::TryPour(*Inventory, 0)));
			ASSERT_THAT(IsTrue(GetHeldWater() == 0));
			// An empty can is a stat of 0, not an absent stat, which would be a full can.
			ASSERT_THAT(IsTrue(Inventory->FindSlotStat(0, VestigeGameplayTags::ItemStat_Water_Clean) == 0));
		}

		TEST_METHOD(FillingNeverPassesTheCapacity)
		{
			HoldCan(3);

			ASSERT_THAT(IsTrue(FarmWateringCan::Fill(*Inventory, 0, 2) == 2));
			ASSERT_THAT(IsTrue(GetHeldWater() == 5));
			ASSERT_THAT(IsTrue(FarmWateringCan::Fill(*Inventory, 0, 0) == 0));
			ASSERT_THAT(IsTrue(FarmWateringCan::Fill(*Inventory, 0, -2) == 0));
			ASSERT_THAT(IsTrue(GetHeldWater() == 5));
			ASSERT_THAT(IsTrue(FarmWateringCan::Fill(*Inventory, 0, 100) == 3));
			ASSERT_THAT(IsTrue(GetHeldWater() == CanCapacity));
			ASSERT_THAT(IsTrue(FarmWateringCan::Fill(*Inventory, 0, 1) == 0));
		}

		TEST_METHOD(ACanKeepsItsWaterThroughASaveAndALoad)
		{
			HoldCan(0);
			FVestigeInventorySaveData Saved = Inventory->GetSaveData();

			ASSERT_THAT(IsTrue(Inventory->RestoreFromSaveData(Saved) == 0));
			ASSERT_THAT(IsTrue(GetHeldWater() == 0));

			Inventory->SetSlotStat(0, VestigeGameplayTags::ItemStat_Water_Clean, 5);
			Saved = Inventory->GetSaveData();
			HoldCan();
			ASSERT_THAT(IsTrue(Inventory->RestoreFromSaveData(Saved) == 0));
			ASSERT_THAT(IsTrue(GetHeldWater() == 5));
		}

		TEST_METHOD(ACanFromASaveWithoutStatsLoadsFull)
		{
			SetBag({{0, TEXT("Base.WateringCan"), 1}});

			ASSERT_THAT(IsTrue(GetHeldWater() == CanCapacity));
		}

		TEST_METHOD(TheCanToFillIsTheFirstThatIsntFull)
		{
			SetBag({{1, TEXT("Base.WateringCan"), 1}, {2, TEXT("Base.WateringCan"), 1}});
			ASSERT_THAT(IsTrue(Inventory->SetSlotStat(2, VestigeGameplayTags::ItemStat_Water_Clean, 3)));

			// The first can is full, so the second, which isn't, is the one to fill.
			ASSERT_THAT(IsTrue(FarmWateringCan::FindCanToFill(*Inventory) == 2));

			// An empty first can is filled before a half-full second.
			ASSERT_THAT(IsTrue(Inventory->SetSlotStat(1, VestigeGameplayTags::ItemStat_Water_Clean, 0)));
			ASSERT_THAT(IsTrue(FarmWateringCan::FindCanToFill(*Inventory) == 1));

			// With every can full it is the first can, so the well can say so.
			ASSERT_THAT(IsTrue(Inventory->SetSlotStat(1, VestigeGameplayTags::ItemStat_Water_Clean, CanCapacity)));
			ASSERT_THAT(IsTrue(Inventory->SetSlotStat(2, VestigeGameplayTags::ItemStat_Water_Clean, CanCapacity)));
			ASSERT_THAT(IsTrue(FarmWateringCan::FindCanToFill(*Inventory) == 1));
		}

		// ---- The watering ability ----

		TEST_METHOD(WateringPoursOneFromTheCanInHand)
		{
			Grid->GetField()->Till(TargetCoordinates);
			HoldCan();

			UseToolAndFinish();

			ASSERT_THAT(IsTrue(TargetTile()->GetWaterLevel() == 1));
			ASSERT_THAT(IsTrue(GetHeldWater() == CanCapacity - 1));
		}

		TEST_METHOD(AnEmptyCanWatersNothingAndCostsNothing)
		{
			Grid->GetField()->Till(TargetCoordinates);
			HoldCan(0);
			ListenToFeedback();

			UseToolAndFinish();

			ASSERT_THAT(IsTrue(Reports == TArray<FGameplayTag>{VestigeGameplayTags::Feedback_Water_CanEmpty}));
			ASSERT_THAT(IsTrue(TargetTile()->GetWaterLevel() == 0));
			ASSERT_THAT(IsTrue(GetStamina() == GetMaxStamina()));
			ASSERT_THAT(IsTrue(GetHeldWater() == 0));
		}

		TEST_METHOD(TheEmptyCanComesAfterNoTargetAndBeforeTheTilesReasons)
		{
			HoldCan(0);
			ListenToFeedback();
			const auto ExpectOnlyReport = [this](const FGameplayTag& Reason)
			{
				ASSERT_THAT(IsTrue(Reports == TArray<FGameplayTag>{Reason}));
				Reports.Reset();
			};

			// No tile to face: that reason comes first.
			Farmer->SetActorLocation(FVector(-500.f, -500.f, 0.f));
			UseToolAndFinish();
			ExpectOnlyReport(VestigeGameplayTags::Feedback_Tool_NoTarget);

			// A tile that can't be watered (grass): the can's emptiness comes before "Nothing to water here."
			Farmer->SetActorLocation(FVector(250.f, 50.f, 0.f));
			UseToolAndFinish();
			ExpectOnlyReport(VestigeGameplayTags::Feedback_Water_CanEmpty);

			// A soaked tile: the empty can, not "The soil is already soaked."
			Grid->GetField()->Till(TargetCoordinates);
			Grid->GetField()->Water(TargetCoordinates);
			Grid->GetField()->Water(TargetCoordinates);
			UseToolAndFinish();
			ExpectOnlyReport(VestigeGameplayTags::Feedback_Water_CanEmpty);
		}

		TEST_METHOD(AHeldPressWatersUntilEmptyThenSaysSoOnce)
		{
			Grid->GetField()->Till(TargetCoordinates);
			Grid->GetField()->Till(FIntPoint(4, 0));
			HoldCan(1);
			ListenToFeedback();

			FarmingComponent->StartUsingTool();
			ASSERT_THAT(IsTrue(TargetTile()->GetWaterLevel() == 1));
			ASSERT_THAT(IsTrue(GetHeldWater() == 0));

			AfterOneUse(
				[this]()
				{
					// The next tile gets nothing, and the empty can was reported once, whatever the repeats and the target did.
					Farmer->SetActorLocation(FVector(350.f, 50.f, 0.f));
					ASSERT_THAT(IsTrue(Grid->GetTile(FIntPoint(4, 0))->GetWaterLevel() == 0));
					Farmer->SetActorLocation(FVector(250.f, 50.f, 0.f));

					ASSERT_THAT(IsTrue(Reports == TArray<FGameplayTag>{VestigeGameplayTags::Feedback_Water_CanEmpty}));
					AbilitySystem->CancelAllAbilities();
				});
		}

		TEST_METHOD(ANewPressOnAnEmptyCanSaysSoAgain)
		{
			Grid->GetField()->Till(TargetCoordinates);
			HoldCan(0);
			ListenToFeedback();

			UseToolAndFinish();
			UseToolAndFinish();

			const TArray<FGameplayTag> Twice = {VestigeGameplayTags::Feedback_Water_CanEmpty, VestigeGameplayTags::Feedback_Water_CanEmpty};
			ASSERT_THAT(IsTrue(Reports == Twice));
		}

		// ---- The well ----

		/** A BP_Well far from the grid, which the farmer stands at by StandAt. */
		AFarmWell& PlaceWell()
		{
			UClass* WellClass = LoadClass<AFarmWell>(nullptr, TEXT("/Game/Vestige/Blueprints/BP_Well.BP_Well_C"));
			check(WellClass);
			AFarmWell& NewWell = Spawner.SpawnActor<AFarmWell>(FActorSpawnParameters(), WellClass);
			NewWell.SetActorLocation(WellLocation);
			return NewWell;
		}

		static inline const FVector WellLocation = FVector(1880.f, 360.f, 0.f);

		TArray<FVestigeInteractionOption> GetOptions(AFarmWell& Well) const
		{
			TArray<FVestigeInteractionOption> Options;
			IVestigeInteractable::Execute_GetInteractionOptions(&Well, Farmer, Options);
			return Options;
		}

		/** Presses F's Fill at the well: the option the well offers, used through its interface. */
		void Fill(AFarmWell& Well)
		{
			FVestigeInteractionOption Option;
			Option.Action = AFarmWell::FillAction;
			IVestigeInteractable::Execute_Interact(&Well, Farmer, Option);
		}

		TEST_METHOD(TheWellOffersFillCanWithWhatIsLeft)
		{
			AFarmWell& Well = PlaceWell();
			HoldCan();

			TArray<FVestigeInteractionOption> Options = GetOptions(Well);
			ASSERT_THAT(IsTrue(Options.Num() == 1));
			ASSERT_THAT(IsTrue(Options[0].Action == AFarmWell::FillAction));
			ASSERT_THAT(IsFalse(Options[0].bHasLocation));
			ASSERT_THAT(AreEqual(Options[0].Text.ToString(), FString(TEXT("Fill can (24 left)"))));

			GetWater().DrawCleanWater(5);
			Options = GetOptions(Well);
			ASSERT_THAT(AreEqual(Options[0].Text.ToString(), FString(TEXT("Fill can (19 left)"))));

			// A dry well still offers it, so F can say why it gives nothing.
			GetWater().DrawCleanWater(100);
			Options = GetOptions(Well);
			ASSERT_THAT(AreEqual(Options[0].Text.ToString(), FString(TEXT("Fill can (0 left)"))));
		}

		TEST_METHOD(TheWellOffersNothingWithoutACan)
		{
			AFarmWell& Well = PlaceWell();
			SetBag({{0, TEXT("Base.Hoe"), 1}});

			ASSERT_THAT(IsTrue(GetOptions(Well).IsEmpty()));
		}

		TEST_METHOD(FillingTopsUpTheCanAndDrawsOnlyWhatItLacks)
		{
			AFarmWell& Well = PlaceWell();
			HoldCan(3);
			ListenToFeedback();

			Fill(Well);

			ASSERT_THAT(IsTrue(GetHeldWater() == CanCapacity));
			ASSERT_THAT(IsTrue(GetWater().GetCleanWaterLeft() == TestSupply - 5));
			ASSERT_THAT(IsTrue(Reports == TArray<FGameplayTag>{VestigeGameplayTags::Feedback_Water_CanFilled}));
		}

		TEST_METHOD(AFullCanIsReportedBeforeADryWell)
		{
			AFarmWell& Well = PlaceWell();
			HoldCan();
			SetSupply(0);
			ListenToFeedback();

			Fill(Well);

			ASSERT_THAT(IsTrue(Reports == TArray<FGameplayTag>{VestigeGameplayTags::Feedback_Water_CanFull}));
			ASSERT_THAT(IsTrue(GetHeldWater() == CanCapacity));
		}

		TEST_METHOD(AFullCanDrawsNothing)
		{
			AFarmWell& Well = PlaceWell();
			HoldCan();
			ListenToFeedback();

			Fill(Well);

			ASSERT_THAT(IsTrue(Reports == TArray<FGameplayTag>{VestigeGameplayTags::Feedback_Water_CanFull}));
			ASSERT_THAT(IsTrue(GetWater().GetCleanWaterLeft() == TestSupply));
		}

		TEST_METHOD(ADryWellIsReported)
		{
			AFarmWell& Well = PlaceWell();
			HoldCan(2);
			SetSupply(0);
			ListenToFeedback();

			Fill(Well);

			ASSERT_THAT(IsTrue(Reports == TArray<FGameplayTag>{VestigeGameplayTags::Feedback_Water_WellDry}));
			ASSERT_THAT(IsTrue(GetHeldWater() == 2));
		}

		TEST_METHOD(AWellRunningDryFillsWhatItHas)
		{
			AFarmWell& Well = PlaceWell();
			HoldCan(0);
			SetSupply(3);
			ListenToFeedback();

			Fill(Well);

			ASSERT_THAT(IsTrue(GetHeldWater() == 3));
			ASSERT_THAT(IsTrue(GetWater().GetCleanWaterLeft() == 0));
			ASSERT_THAT(IsTrue(Reports == TArray<FGameplayTag>{VestigeGameplayTags::Feedback_Water_WellRanDry}));
		}

		TEST_METHOD(FillingFindsTheCanAnywhereInTheBag)
		{
			AFarmWell& Well = PlaceWell();
			SetBag({{0, TEXT("Base.Hoe"), 1}, {5, TEXT("Base.WateringCan"), 1}});
			ASSERT_THAT(IsTrue(Inventory->SetSlotStat(5, VestigeGameplayTags::ItemStat_Water_Clean, 1)));
			ASSERT_THAT(IsTrue(GetOptions(Well).Num() == 1));

			Fill(Well);

			ASSERT_THAT(IsTrue(FarmWateringCan::GetWater(*Inventory, 5) == CanCapacity));
		}

		TEST_METHOD(AnotherActionAtTheWellDoesNothing)
		{
			AFarmWell& Well = PlaceWell();
			HoldCan(2);
			ListenToFeedback();
			FVestigeInteractionOption Option;
			Option.Action = TEXT("Harvest");

			IVestigeInteractable::Execute_Interact(&Well, Farmer, Option);

			ASSERT_THAT(IsTrue(Reports.IsEmpty()));
			ASSERT_THAT(IsTrue(GetHeldWater() == 2));
			ASSERT_THAT(IsTrue(GetWater().GetCleanWaterLeft() == TestSupply));
		}

		TEST_METHOD(ACanFilledAtTheWellWatersEightTimes)
		{
			AFarmWell& Well = PlaceWell();
			HoldCan(0);
			Fill(Well);
			for (int32 Column = 0; Column < CanCapacity; ++Column)
			{
				Grid->GetField()->Till(FIntPoint(Column, 5));
				ASSERT_THAT(IsTrue(Grid->GetField()->Water(FIntPoint(Column, 5))));
				ASSERT_THAT(IsTrue(FarmWateringCan::TryPour(*Inventory, 0)));
			}

			ASSERT_THAT(IsTrue(GetHeldWater() == 0));
			ASSERT_THAT(IsFalse(FarmWateringCan::TryPour(*Inventory, 0)));
		}

		/** Stands the farmer at Location aiming at the well and lets the interaction scan. */
		void StandAtTheWell(const FVector& Offset)
		{
			Farmer->SetActorLocation(WellLocation + Offset);
			Farmer->FindComponentByClass<UFarmTargetingComponent>()->AimAt(-Offset.GetSafeNormal());
			Farmer->FindComponentByClass<UVestigeInteractionComponent>()->Scan();
		}

		TEST_METHOD(AFarmerAtTheRingFacingTheWellFocusesIt)
		{
			AFarmWell& Well = PlaceWell();
			HoldCan();
			const UVestigeInteractionComponent& Interaction = *Farmer->FindComponentByClass<UVestigeInteractionComponent>();

			// Against the ring on an axis (the ring's face is 43.5 cm out, the capsule 42 cm), and at a corner on a diagonal.
			const float Diagonal = 90.f / UE_SQRT_2;
			const FVector Offsets[] = {
				FVector(-87.f, 0.f, 0.f), FVector(0.f, 87.f, 0.f), FVector(-Diagonal, -Diagonal, 0.f), FVector(Diagonal, Diagonal, 0.f)};
			for (const FVector& Offset : Offsets)
			{
				StandAtTheWell(Offset);

				ASSERT_THAT(IsTrue(Interaction.GetFocusedActor() == &Well, Offset.ToString()));
				ASSERT_THAT(IsTrue(Interaction.GetFocusedOption().Action == AFarmWell::FillAction, Offset.ToString()));
			}
		}

		TEST_METHOD(TheWellBlueprintHasItsMesh)
		{
			AFarmWell& Well = PlaceWell();

			const UStaticMeshComponent* Mesh = Well.FindComponentByClass<UStaticMeshComponent>();
			ASSERT_THAT(IsNotNull(Mesh));
			ASSERT_THAT(IsNotNull(Mesh->GetStaticMesh().Get()));
			ASSERT_THAT(AreEqual(Mesh->GetStaticMesh()->GetName(), FString(TEXT("SM_Well"))));
		}
	};
}

#endif // WITH_DEV_AUTOMATION_TESTS
