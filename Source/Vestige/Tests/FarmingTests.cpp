// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/VestigeGameplayTags.h"
#include "AbilitySystem/VestigeStaminaSet.h"
#include "AbilitySystemComponent.h"
#include "Components/ActorTestSpawner.h"
#include "Content/FarmCropData.h"
#include "Content/FarmSeedFragment.h"
#include "Content/VestigeCalendar.h"
#include "Content/VestigeContentCatalog.h"
#include "Content/VestigeGameData.h"
#include "Content/VestigeItemData.h"
#include "Content/VestigeToolData.h"
#include "Content/VestigeToolFragment.h"
#include "Core/VestigeFeedbackSubsystem.h"
#include "Core/VestigeMilestoneSubsystem.h"
#include "CQTest.h"
#include "Engine/AssetManager.h"
#include "Farming/FarmField.h"
#include "Farming/FarmGrid.h"
#include "Farming/FarmingComponent.h"
#include "Farming/FarmTargetingComponent.h"
#include "Farming/FarmTileState.h"
#include "Farming/VestigeBed.h"
#include "Farming/VestigeGameplayAbility_UseTool.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interaction/VestigeInteractable.h"
#include "Interaction/VestigeInteractionComponent.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/VestigeInventoryComponent.h"
#include "Inventory/VestigeInventorySaveData.h"
#include "Misc/ScopeExit.h"
#include "Player/VestigePlayerCharacter.h"
#include "Player/VestigePlayerState.h"
#include "Tests/VestigeTestEventRecorder.h"
#include "Tests/VestigeTestToolAbility.h"
#include "Time/VestigeClockSubsystem.h"
#include "TimerManager.h"
#include "UObject/StrongObjectPtr.h"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace FarmingTests
{
	/** More than one use takes at full stamina (0.4 s), and less than two. */
	constexpr float OneUseSeconds = 0.6f;

	TEST_CLASS(Farming, "Vestige")
	{
		FActorTestSpawner Spawner;
		AFarmGrid* Grid = nullptr;
		AVestigePlayerState* PlayerState = nullptr;
		AVestigePlayerCharacter* Farmer = nullptr;
		UFarmingComponent* FarmingComponent = nullptr;
		UVestigeInventoryComponent* Inventory = nullptr;
		UAbilitySystemComponent* AbilitySystem = nullptr;

		/** The tags of the feedback reports since ListenToFeedback, kept on the fixture so latent steps can read them. */
		TArray<FGameplayTag> Reports;

		/**
		 * A default grid and the character Blueprint the game uses, standing in front of tile (3, 0), with the game's
		 * player state Blueprint, whose inventory knows the base items and whose ability set grants stamina. The Blueprints
		 * are used because they hold the real toolbar, seed item and ability set, which load from the base content.
		 */
		BEFORE_EACH()
		{
			// The stamina cost is a GE_ asset named by the game data, which the asset manager loaded at startup.
			ASSERT_THAT(IsNotNull(UVestigeGameData::Get().GetStaminaCostEffect().Get()));
			// The grid's field and the clock live in the game instance.
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

			// Possessed, so the farming component follows the inventory's hand through the controller change, as in play.
			Controller = &Spawner.SpawnActor<APlayerController>();
			Controller->PlayerState = PlayerState;
			Farmer = &SpawnFarmer();
			Controller->Possess(Farmer);

			// Possessing sets the default movement mode, which is falling here, since the test level has no floor.
			Farmer->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
			FarmingComponent = Farmer->FindComponentByClass<UFarmingComponent>();
			ASSERT_THAT(IsNotNull(FarmingComponent));
		}

		APlayerController* Controller = nullptr;

		static FPrimaryAssetId TurnipSeedsId() { return FPrimaryAssetId(UVestigeItemData::AssetType, TEXT("Base.TurnipSeeds")); }

		/** A loaded base item by its ContentId. */
		/** The content catalogue of the fixture's game instance. */
		UVestigeContentCatalog& Catalog() { return *UVestigeContentCatalog::Get(&Spawner.GetWorld()); }

		static const UVestigeItemData* GetBaseItem(FName ContentId)
		{
			return UAssetManager::Get().GetPrimaryAssetObject<UVestigeItemData>(FPrimaryAssetId(UVestigeItemData::AssetType, ContentId));
		}

		/** Restores the bag to exactly these slots, with SelectedSlot selected, the way loading a save does. */
		void SetBag(TArray<FVestigeInventorySlotSaveData> Slots, int32 SelectedSlot = 0)
		{
			FVestigeInventorySaveData Bag;
			Bag.Slots = MoveTemp(Slots);
			Bag.SelectedHotbarSlot = SelectedSlot;
			ASSERT_THAT(IsTrue(Inventory->RestoreFromSaveData(Bag) == 0));
		}

		/** A farmer from the game's character Blueprint, in front of tile (3, 0), begun play but without a player state. */
		AVestigePlayerCharacter& SpawnFarmer()
		{
			UClass* FarmerClass =
				LoadClass<AVestigePlayerCharacter>(nullptr, TEXT("/Game/Vestige/Blueprints/BP_PlayerCharacter.BP_PlayerCharacter_C"));
			check(FarmerClass);
			AVestigePlayerCharacter& NewFarmer = Spawner.SpawnActor<AVestigePlayerCharacter>(FActorSpawnParameters(), FarmerClass);
			NewFarmer.DispatchBeginPlay();
			NewFarmer.SetActorLocationAndRotation(FVector(250.f, 50.f, 0.f), FRotator::ZeroRotator);
			return NewFarmer;
		}

		/**
		 * Puts the item of this tool in hotbar slot 0 and selects it, through the same restore loading uses, and makes the bag
		 * hold only this many seeds. Base.Seeds is the turnip seeds themselves (an empty hand when Seeds is 0), and Base.Hand
		 * an empty slot 0; other tools are the item with the tool's name, with the seeds in slot 1.
		 */
		void Hold(FName ToolId, int32 Seeds)
		{
			TArray<FVestigeInventorySlotSaveData> Slots;
			if (ToolId == TEXT("Base.Seeds"))
			{
				if (Seeds > 0)
				{
					Slots.Add({0, TurnipSeedsId().PrimaryAssetName, Seeds});
				}
			}
			else
			{
				if (ToolId != TEXT("Base.Hand"))
				{
					Slots.Add({0, ToolId, 1});
				}
				if (Seeds > 0)
				{
					Slots.Add({1, TurnipSeedsId().PrimaryAssetName, Seeds});
				}
			}
			SetBag(MoveTemp(Slots));
		}

		/**
		 * Uses the tool in hand once, then ends the use at once, as its wait would once its time is up: these tests don't let
		 * time pass, and the next use can't start before the last one ends.
		 */
		void UseToolAndFinish()
		{
			FarmingComponent->UseTool();
			AbilitySystem->CancelAllAbilities();
		}

		/**
		 * Lets one use's time pass, then runs Check. The test world isn't ticked, and ticking it would move the farmer, so
		 * only its timers advance (a use's wait is a timer). A timer manager ticks at most once a frame, and a timer set
		 * outside its tick only becomes active at the end of its next tick, so this takes two frames: one tick makes the
		 * use's wait active, the next lets OneUseSeconds pass.
		 */
		void AfterOneUse(TFunction<void()> Check)
		{
			TestCommandBuilder
				.Until(
					[this]()
					{
						return TickTimersOnce();
					})
				.Until(
					[this]()
					{
						return TickTimersOnce();
					})
				.Then(MoveTemp(Check));
		}

		/** Ticks the world's timers by OneUseSeconds, unless they were ticked this frame. Returns whether it ticked them. */
		bool TickTimersOnce()
		{
			FTimerManager& Timers = Spawner.GetWorld().GetTimerManager();
			if (Timers.HasBeenTickedThisFrame())
			{
				return false;
			}
			Timers.Tick(OneUseSeconds);
			return true;
		}
		int32 GetSeeds() const { return Inventory->GetItemCount(TurnipSeedsId()); }

		float GetStamina() const { return AbilitySystem->GetNumericAttribute(UVestigeStaminaSet::GetStaminaAttribute()); }
		float GetMaxStamina() const { return AbilitySystem->GetNumericAttribute(UVestigeStaminaSet::GetMaxStaminaAttribute()); }

		/** The tool asset in hand, after Hold. */
		const UVestigeToolData& GetHeldTool() const { return *FarmingComponent->GetCurrentTool(); }

		/** Takes all of the player's stamina, through the cost effect a tool use spends it with. */
		void EmptyStamina()
		{
			const FGameplayEffectSpecHandle Spec =
				AbilitySystem->MakeOutgoingSpec(UVestigeGameData::Get().GetStaminaCostEffect(), 1.f, AbilitySystem->MakeEffectContext());
			Spec.Data->SetSetByCallerMagnitude(VestigeGameplayTags::Data_StaminaCost, -GetMaxStamina());
			AbilitySystem->ApplyGameplayEffectSpecToSelf(*Spec.Data);
		}

		/** Whether a tool ability is active, so no other tool use can start yet. */
		bool IsUsingTool() const { return AbilitySystem->HasMatchingGameplayTag(VestigeGameplayTags::State_UsingTool); }

		/**
		 * The abilities of the tool in hand granted on the player's ability system, without the harvest, which is granted all
		 * the time. Read at once: specs move when abilities are removed.
		 */
		TArray<const FGameplayAbilitySpec*> GetToolAbilities() const { return GetUseToolAbilities(false); }

		/** The harvest abilities granted on the player's ability system: the ones whose source is the hand's tool asset. */
		TArray<const FGameplayAbilitySpec*> GetHarvestAbilities() const { return GetUseToolAbilities(true); }

		TArray<const FGameplayAbilitySpec*> GetUseToolAbilities(bool bHarvest) const
		{
			const UVestigeToolData* HarvestTool = GetBaseTool(TEXT("Base.Hand"));
			TArray<const FGameplayAbilitySpec*> Specs;
			for (const FGameplayAbilitySpec& Spec : AbilitySystem->GetActivatableAbilities())
			{
				if (Spec.Ability && Spec.Ability->GetAssetTags().HasTagExact(VestigeGameplayTags::Ability_UseTool)
					&& (Spec.SourceObject.Get() == HarvestTool) == bHarvest)
				{
					Specs.Add(&Spec);
				}
			}
			return Specs;
		}

		/** Harvests the target tile once, then ends the use at once, as UseToolAndFinish does. */
		void HarvestAndFinish()
		{
			FarmingComponent->Harvest();
			AbilitySystem->CancelAllAbilities();
		}

		/** Tills, plants a turnip at Coordinates and grows it until it is ripe, through the field's rules. */
		const UFarmCropData* GrowRipeCropAt(const FIntPoint& Coordinates)
		{
			UFarmField* Field = Grid->GetField();
			const UFarmCropData* Crop = Catalog().Find<UFarmCropData>(FPrimaryAssetId(UFarmCropData::AssetType, TEXT("Base.Turnip")));
			Field->Till(Coordinates);
			if (!Crop || !Field->Plant(Coordinates, Crop))
			{
				return nullptr;
			}
			for (int32 Day = 0; Day < Crop->GetDaysToMature(); ++Day)
			{
				Field->Water(Coordinates);
				Field->AdvanceDay(1, 0);
			}
			return Crop;
		}

		/** The tile the farmer faces at the start. */
		static inline const FIntPoint TargetCoordinates = FIntPoint(3, 0);

		const FFarmTileState* TargetTile() const { return Grid->GetTile(TargetCoordinates); }

		/** Plants a seed in the target tile and grows it until it is ripe. */
		const UFarmCropData* GrowRipeCrop()
		{
			Grid->GetField()->Till(TargetCoordinates);
			Hold(TEXT("Base.Seeds"), 1);
			UseToolAndFinish();
			const UFarmCropData* Crop = TargetTile()->GetCrop();
			if (Crop)
			{
				for (int32 Day = 0; Day < Crop->GetDaysToMature(); ++Day)
				{
					Grid->GetField()->Water(TargetCoordinates);
					Grid->GetField()->AdvanceDay(1, 0);
				}
			}
			return Crop;
		}

		/** A recorder bound to the farming component's Blueprint events, the way a Blueprint binds them. */
		TStrongObjectPtr<UVestigeTestEventRecorder> ListenToBlueprintEvents() const
		{
			TStrongObjectPtr<UVestigeTestEventRecorder> Recorder(NewObject<UVestigeTestEventRecorder>());
			FarmingComponent->OnCropHarvested.AddDynamic(Recorder.Get(), &UVestigeTestEventRecorder::RecordCropHarvested);
			FarmingComponent->OnToolUsed.AddDynamic(Recorder.Get(), &UVestigeTestEventRecorder::RecordToolUsed);
			return Recorder;
		}

		/** Collects the tag of every feedback report from now on into Reports. */
		void ListenToFeedback()
		{
			UVestigeFeedbackSubsystem* Feedback = Spawner.GetWorld().GetSubsystem<UVestigeFeedbackSubsystem>();
			ASSERT_THAT(IsNotNull(Feedback));
			Feedback->OnFeedback.AddLambda(
				[this](const FVestigeFeedbackReport& Report)
				{
					Reports.Add(Report.Tag);
					LastArguments = Report.Arguments;
				});
		}

		/** The values of the last report heard since ListenToFeedback. */
		FFormatNamedArguments LastArguments;

		static FString TextOf(const FFormatNamedArguments& Values, const TCHAR* Key)
		{
			const FFormatArgumentValue* Value = Values.Find(Key);
			return Value && Value->GetType() == EFormatArgumentType::Text ? Value->GetTextValue().ToString() : FString();
		}

		static int32 NumberOf(const FFormatNamedArguments& Values, const TCHAR* Key)
		{
			const FFormatArgumentValue* Value = Values.Find(Key);
			return Value && Value->GetType() == EFormatArgumentType::Int ? static_cast<int32>(Value->GetIntValue()) : INDEX_NONE;
		}

		static inline const FName CabbageSeedsName = TEXT("Base.CabbageSeeds");
		static inline const FName PotatoSeedsName = TEXT("Base.PotatoSeeds");

		UVestigeClockSubsystem& Clock() { return *UVestigeClockSubsystem::Get(&Spawner.GetWorld()); }

		const FVestigeSeason& GetSeason(int32 Index) { return Clock().GetCalendar().GetSeasons()[Index]; }

		/** Replaces the bag with this many seeds of the item, in slot 0 and in hand. */
		void HoldSeeds(FName SeedsName, int32 Count) { SetBag({{0, SeedsName, Count}}); }

		int32 GetSeedsOf(FName SeedsName) const { return Inventory->GetItemCount(FPrimaryAssetId(UVestigeItemData::AssetType, SeedsName)); }

		/** The crop the seeds of this item grow. */
		const UFarmCropData* GetCropOf(FName SeedsName) { return UFarmSeedFragment::FindCropOf(GetBaseItem(SeedsName), &Catalog()); }

		/**
		 * Moves the clock to the day of the first season with this many days left, where a spring crop of more days to ripen
		 * than that is too late. Returns whether it could.
		 */
		bool JumpToDaysLeft(int32 DaysLeft) { return Clock().JumpToDay(Clock().GetCalendar().GetDaysPerSeason() - DaysLeft); }

		/** Tills the target tile, holds the seeds, and goes to a day with one day left in the first season. */
		void SetUpLatePlanting(FName SeedsName)
		{
			ASSERT_THAT(IsTrue(GetCropOf(SeedsName)->GetDaysToMature() > 1));
			ASSERT_THAT(IsTrue(JumpToDaysLeft(1)));
			Grid->GetField()->Till(TargetCoordinates);
			HoldSeeds(SeedsName, 3);
		}

		/** Moves the farmer one tile on along the row, so the target becomes tile (4, 0). */
		const FFarmTileState* StepToNextTile()
		{
			Farmer->SetActorLocation(FVector(350.f, 50.f, 0.f));
			return Grid->GetTile(FIntPoint(4, 0));
		}

		const FFarmTileState* NextTile() const { return Grid->GetTile(FIntPoint(4, 0)); }

		TEST_METHOD(PlantingUsesASeed)
		{
			Grid->GetField()->Till(TargetCoordinates);
			Hold(TEXT("Base.Seeds"), 5);

			UseToolAndFinish();

			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Planted));
			ASSERT_THAT(IsTrue(GetSeeds() == 4));
		}

		TEST_METHOD(TheSeedItemPlantsTheCropItsFragmentNames)
		{
			Grid->GetField()->Till(TargetCoordinates);
			Hold(TEXT("Base.Seeds"), 1);

			UseToolAndFinish();

			ASSERT_THAT(IsNotNull(TargetTile()->GetCrop()));
			ASSERT_THAT(IsTrue(TargetTile()->GetCrop()->GetContentId() == TEXT("Base.Turnip")));
		}

		TEST_METHOD(PlantingPotatoSeedsGrowsAPotato)
		{
			Grid->GetField()->Till(TargetCoordinates);
			SetBag({{0, TEXT("Base.PotatoSeeds"), 1}});
			UseToolAndFinish();
			const UFarmCropData* Crop = TargetTile()->GetCrop();
			ASSERT_THAT(IsNotNull(Crop));
			ASSERT_THAT(IsTrue(Crop->GetContentId() == TEXT("Base.Potato")));
			ASSERT_THAT(IsTrue(Inventory->GetItemCount(FPrimaryAssetId(UVestigeItemData::AssetType, TEXT("Base.PotatoSeeds"))) == 0));

			for (int32 Day = 0; Day < Crop->GetDaysToMature(); ++Day)
			{
				ASSERT_THAT(IsFalse(TargetTile()->IsRipe()));
				Grid->GetField()->Water(TargetCoordinates);
				Grid->GetField()->AdvanceDay(1, 0);
			}
			ASSERT_THAT(IsTrue(TargetTile()->IsRipe()));
			Hold(TEXT("Base.Hand"), 0);
			HarvestAndFinish();

			const FPrimaryAssetId PotatoId(UVestigeItemData::AssetType, TEXT("Base.Potato"));
			ASSERT_THAT(IsTrue(Crop->GetHarvestItemId() == PotatoId));
			ASSERT_THAT(IsTrue(Inventory->GetItemCount(PotatoId) == Crop->GetYield()));
			ASSERT_THAT(IsTrue(Crop->GetYield() > 0));
		}

		TEST_METHOD(WithoutSeedsNothingIsPlanted)
		{
			Grid->GetField()->Till(TargetCoordinates);
			Hold(TEXT("Base.Seeds"), 0);

			UseToolAndFinish();

			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Tilled));
			ASSERT_THAT(IsTrue(GetSeeds() == 0));
		}

		TEST_METHOD(WithoutAPlayerStateNothingIsPlanted)
		{
			Grid->GetField()->Till(TargetCoordinates);
			Hold(TEXT("Base.Seeds"), 5);
			Farmer->SetPlayerState(nullptr);

			UseToolAndFinish();

			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Tilled));
		}

		TEST_METHOD(TheHoeTillsTheTileInFront)
		{
			Hold(TEXT("Base.Hoe"), 0);

			UseToolAndFinish();

			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Tilled));
		}

		TEST_METHOD(HarvestingAddsTheCropsYieldOfItsItem)
		{
			const UFarmCropData* Crop = GrowRipeCrop();
			ASSERT_THAT(IsNotNull(Crop));
			Hold(TEXT("Base.Hand"), 0);

			HarvestAndFinish();

			ASSERT_THAT(IsTrue(Inventory->GetItemCount(Crop->GetHarvestItemId()) == Crop->GetYield()));
			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Tilled));
		}

		TEST_METHOD(AFullBagLeavesTheCropInTheGround)
		{
			const UFarmCropData* Crop = GrowRipeCrop();
			ASSERT_THAT(IsNotNull(Crop));
			Hold(TEXT("Base.Hand"), 0);

			// Every slot holds a full stack, so the harvest has nowhere to go.
			const UVestigeItemData* Seed = GetBaseItem(TurnipSeedsId().PrimaryAssetName);
			const UVestigeItemData* Turnip = GetBaseItem(TEXT("Base.Turnip"));
			ASSERT_THAT(IsNotNull(Seed));
			ASSERT_THAT(IsNotNull(Turnip));
			TArray<FVestigeInventorySlotSaveData> FullBag;
			FullBag.Add({0, Turnip->GetContentId(), Turnip->GetMaxStackSize()});
			for (int32 Slot = 1; Slot < Inventory->GetSlots().Num(); ++Slot)
			{
				FullBag.Add({Slot, Seed->GetContentId(), Seed->GetMaxStackSize()});
			}
			SetBag(MoveTemp(FullBag));
			const int32 TurnipsBefore = Inventory->GetItemCount(Crop->GetHarvestItemId());
			ListenToFeedback();

			HarvestAndFinish();

			ASSERT_THAT(IsTrue(Inventory->GetItemCount(Crop->GetHarvestItemId()) == TurnipsBefore));
			ASSERT_THAT(IsTrue(TargetTile()->IsRipe()));
			ASSERT_THAT(IsTrue(Reports == TArray<FGameplayTag>{VestigeGameplayTags::Feedback_Inventory_Full}));
		}

		TEST_METHOD(UsingAToolAwayFromTheFieldChangesNothing)
		{
			Hold(TEXT("Base.Seeds"), 3);
			Farmer->SetActorLocation(FVector(-500.f, -500.f, 0.f));

			UseToolAndFinish();

			ASSERT_THAT(IsTrue(GetSeeds() == 3));
		}

		TEST_METHOD(AUseThatChangesTheFarmCostsTheToolsStamina)
		{
			Hold(TEXT("Base.Hoe"), 0);
			ASSERT_THAT(IsTrue(GetHeldTool().GetStaminaCost() > 0.f));

			UseToolAndFinish();

			ASSERT_THAT(IsTrue(GetStamina() == GetMaxStamina() - GetHeldTool().GetStaminaCost()));
		}

		TEST_METHOD(AUseThatChangesNothingCostsNothing)
		{
			Hold(TEXT("Base.WateringCan"), 0);

			UseToolAndFinish();

			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Grass));
			ASSERT_THAT(IsTrue(GetStamina() == GetMaxStamina()));
		}

		TEST_METHOD(AUseThatWorkedReportsNothing)
		{
			ListenToFeedback();

			Hold(TEXT("Base.Hoe"), 0);
			UseToolAndFinish();
			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Tilled));
			Hold(TEXT("Base.Seeds"), 1);
			UseToolAndFinish();
			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Planted));
			Hold(TEXT("Base.WateringCan"), 0);
			UseToolAndFinish();
			ASSERT_THAT(IsTrue(TargetTile()->IsWatered()));

			const UFarmCropData* Crop = TargetTile()->GetCrop();
			ASSERT_THAT(IsNotNull(Crop));
			for (int32 Day = 0; Day < Crop->GetDaysToMature(); ++Day)
			{
				Grid->GetField()->Water(TargetCoordinates);
				Grid->GetField()->AdvanceDay(1, 0);
			}
			ASSERT_THAT(IsTrue(TargetTile()->IsRipe()));
			Hold(TEXT("Base.Hand"), 0);
			HarvestAndFinish();
			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Tilled));

			ASSERT_THAT(IsTrue(Reports.IsEmpty()));
		}

		TEST_METHOD(EachPressOfTheWateringCanAddsOneStepAndPaysForIt)
		{
			Grid->GetField()->Till(TargetCoordinates);
			Hold(TEXT("Base.WateringCan"), 0);

			UseToolAndFinish();
			ASSERT_THAT(IsTrue(TargetTile()->GetWaterLevel() == 1));
			UseToolAndFinish();
			ASSERT_THAT(IsTrue(TargetTile()->GetWaterLevel() == 2));

			ASSERT_THAT(IsTrue(GetStamina() == GetMaxStamina() - 2.f * GetHeldTool().GetStaminaCost()));
		}

		TEST_METHOD(ASoakedTileIsRefusedAsSoakedAndCostsNothing)
		{
			Grid->GetField()->Till(TargetCoordinates);
			Hold(TEXT("Base.WateringCan"), 0);
			UseToolAndFinish();
			UseToolAndFinish();
			const float StaminaWhenSoaked = GetStamina();
			ListenToFeedback();
			const TStrongObjectPtr<UVestigeTestEventRecorder> Recorder = ListenToBlueprintEvents();

			UseToolAndFinish();

			ASSERT_THAT(IsTrue(Reports == TArray<FGameplayTag>{VestigeGameplayTags::Feedback_Farm_Soaked}));
			ASSERT_THAT(IsTrue(Recorder->GetEvents().IsEmpty()));
			ASSERT_THAT(IsTrue(GetStamina() == StaminaWhenSoaked));
			ASSERT_THAT(IsTrue(TargetTile()->GetWaterLevel() == 2));
		}

		TEST_METHOD(AHeldPressWatersEachTileOnceAndANewPressAddsTheNextStep)
		{
			Grid->GetField()->Till(TargetCoordinates);
			Grid->GetField()->Till(FIntPoint(4, 0));
			Hold(TEXT("Base.WateringCan"), 0);
			ListenToFeedback();
			FarmingComponent->StartUsingTool();
			ASSERT_THAT(IsTrue(TargetTile()->GetWaterLevel() == 1));

			AfterOneUse(
				[this]()
				{
					// The repeat on the same tile is refused quietly, so the held key doesn't soak it.
					ASSERT_THAT(IsTrue(TargetTile()->GetWaterLevel() == 1));
					ASSERT_THAT(IsTrue(Reports.IsEmpty()));
					ASSERT_THAT(IsTrue(GetStamina() == GetMaxStamina() - GetHeldTool().GetStaminaCost()));

					// The next tile it faces gets its first step.
					const FFarmTileState* Next = StepToNextTile();
					ASSERT_THAT(IsNotNull(Next));
					ASSERT_THAT(IsTrue(Next->GetWaterLevel() == 1));
					Farmer->SetActorLocation(FVector(250.f, 50.f, 0.f));
					ASSERT_THAT(IsTrue(TargetTile()->GetWaterLevel() == 1));

					// Letting go and pressing again is a new press: the second step.
					FarmingComponent->StopUsingTool();
					AbilitySystem->CancelAllAbilities();
					FarmingComponent->StartUsingTool();
					ASSERT_THAT(IsTrue(TargetTile()->GetWaterLevel() == 2));
					AbilitySystem->CancelAllAbilities();
				});
		}

		TEST_METHOD(APressReportsWhyAUseChangesNothing)
		{
			ListenToFeedback();
			const auto ExpectOnlyReport = [this](const FGameplayTag& Reason)
			{
				ASSERT_THAT(IsTrue(Reports == TArray<FGameplayTag>{Reason}));
				Reports.Reset();
			};

			Hold(TEXT("Base.WateringCan"), 0);
			UseToolAndFinish();
			ExpectOnlyReport(VestigeGameplayTags::Feedback_Farm_CantWater);

			Hold(TEXT("Base.Seeds"), 3);
			UseToolAndFinish();
			ExpectOnlyReport(VestigeGameplayTags::Feedback_Farm_CantPlant);

			Hold(TEXT("Base.Hand"), 0);
			HarvestAndFinish();
			ExpectOnlyReport(VestigeGameplayTags::Feedback_Farm_NothingRipe);

			Grid->GetField()->Till(TargetCoordinates);
			Hold(TEXT("Base.Hoe"), 0);
			UseToolAndFinish();
			ExpectOnlyReport(VestigeGameplayTags::Feedback_Farm_CantTill);

			Farmer->SetActorLocation(FVector(-500.f, -500.f, 0.f));
			UseToolAndFinish();
			ExpectOnlyReport(VestigeGameplayTags::Feedback_Tool_NoTarget);

			// Hold replaces the whole bag, so the seeds are gone by now; the tile shows nothing was planted.
			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Tilled));
			ASSERT_THAT(IsTrue(GetStamina() == GetMaxStamina()));
		}

		TEST_METHOD(OutOfSeasonSeedsAreRefused)
		{
			ListenToFeedback();
			Grid->GetField()->Till(TargetCoordinates);
			HoldSeeds(TurnipSeedsId().PrimaryAssetName, 3);
			ASSERT_THAT(IsTrue(Clock().JumpToDay(Clock().GetCalendar().GetDaysPerSeason() + 1)));

			UseToolAndFinish();

			ASSERT_THAT(IsTrue(Reports == TArray<FGameplayTag>{VestigeGameplayTags::Feedback_Farm_OutOfSeason}));
			ASSERT_THAT(IsTrue(TextOf(LastArguments, TEXT("SeasonInSentence")) == GetSeason(1).NameInSentence.ToString()));
			ASSERT_THAT(IsTrue(TextOf(LastArguments, TEXT("PlantSeasonInSentence")) == GetSeason(0).NameInSentence.ToString()));
			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Tilled));
			ASSERT_THAT(IsTrue(GetSeeds() == 3));
			ASSERT_THAT(IsTrue(GetStamina() == GetMaxStamina()));
		}

		TEST_METHOD(TheSeasonIsReportedBeforeTheTile)
		{
			ListenToFeedback();
			HoldSeeds(TurnipSeedsId().PrimaryAssetName, 3);
			ASSERT_THAT(IsTrue(Clock().JumpToDay(Clock().GetCalendar().GetDaysPerSeason() + 1)));

			UseToolAndFinish();

			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Grass));
			ASSERT_THAT(IsTrue(Reports == TArray<FGameplayTag>{VestigeGameplayTags::Feedback_Farm_OutOfSeason}));
		}

		TEST_METHOD(TheFirstLatePressWarnsAndPlantsNothing)
		{
			ListenToFeedback();
			SetUpLatePlanting(CabbageSeedsName);
			const TStrongObjectPtr<UVestigeTestEventRecorder> Recorder = ListenToBlueprintEvents();

			UseToolAndFinish();

			ASSERT_THAT(IsTrue(Reports == TArray<FGameplayTag>{VestigeGameplayTags::Feedback_Farm_TooLate}));
			ASSERT_THAT(IsTrue(NumberOf(LastArguments, TEXT("DaysToRipe")) == GetCropOf(CabbageSeedsName)->GetDaysToMature()));
			ASSERT_THAT(IsTrue(NumberOf(LastArguments, TEXT("DaysToDeadline")) == 1));
			ASSERT_THAT(IsTrue(TextOf(LastArguments, TEXT("DeadlineSeasonInSentence")) == GetSeason(1).NameInSentence.ToString()));
			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Tilled));
			ASSERT_THAT(IsTrue(GetSeedsOf(CabbageSeedsName) == 3));
			ASSERT_THAT(IsTrue(GetStamina() == GetMaxStamina()));
			ASSERT_THAT(IsTrue(Recorder->GetEvents().IsEmpty()));
		}

		TEST_METHOD(TheNextPressThatDayPlantsWithoutAWarning)
		{
			ListenToFeedback();
			SetUpLatePlanting(CabbageSeedsName);
			UseToolAndFinish();
			ASSERT_THAT(IsTrue(Reports.Num() == 1));

			UseToolAndFinish();

			ASSERT_THAT(IsTrue(Reports.Num() == 1));
			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Planted));
			ASSERT_THAT(IsTrue(TargetTile()->GetCrop() == GetCropOf(CabbageSeedsName)));
			ASSERT_THAT(IsTrue(GetSeedsOf(CabbageSeedsName) == 2));
		}

		TEST_METHOD(AHeldPressAfterTheWarningPlantsNothing)
		{
			ListenToFeedback();
			SetUpLatePlanting(CabbageSeedsName);
			Grid->GetField()->Till(FIntPoint(4, 0));
			const TStrongObjectPtr<UVestigeTestEventRecorder> Recorder = ListenToBlueprintEvents();

			FarmingComponent->StartUsingTool();
			ASSERT_THAT(IsTrue(Reports.Num() == 1));
			ASSERT_THAT(IsTrue(IsUsingTool()));

			// The held press reaches the next tilled tile: the repeat doesn't plant there, and says nothing.
			const FFarmTileState* Next = StepToNextTile();
			ASSERT_THAT(IsNotNull(Next));
			ASSERT_THAT(IsTrue(Next->GetState() == EFarmTileState::Tilled));
			ASSERT_THAT(IsTrue(Reports.Num() == 1));
			ASSERT_THAT(IsTrue(GetSeedsOf(CabbageSeedsName) == 3));
			ASSERT_THAT(IsTrue(GetStamina() == GetMaxStamina()));
			ASSERT_THAT(IsTrue(Recorder->GetEvents().IsEmpty()));

			// A new press that day plants.
			FarmingComponent->StopUsingTool();
			ASSERT_THAT(IsFalse(IsUsingTool()));
			UseToolAndFinish();
			ASSERT_THAT(IsTrue(NextTile()->GetState() == EFarmTileState::Planted));
			ASSERT_THAT(IsTrue(GetSeedsOf(CabbageSeedsName) == 2));
			ASSERT_THAT(IsTrue(Reports.Num() == 1));
		}

		TEST_METHOD(OnlyThePressRefusalIsToldToTheAbility)
		{
			UVestigeToolData* Tool = NewObject<UVestigeToolData>();
			Tool->SetContentIdForTests(TEXT("Test.Tiller"));
			Tool->SetAbilityForTests(UVestigeTestToolAbility::StaticClass());
			ASSERT_THAT(IsTrue(Catalog().AddForTests(*Tool)));
			UVestigeItemData* Item = NewObject<UVestigeItemData>();
			Item->SetContentIdForTests(TEXT("Test.Tiller"));
			UVestigeToolFragment* ToolFragment = NewObject<UVestigeToolFragment>(Item);
			ToolFragment->SetToolIdForTests(Tool->GetPrimaryAssetId());
			Item->AddFragmentForTests(ToolFragment);
			ASSERT_THAT(IsTrue(Catalog().AddForTests(*Item)));
			SetBag({{0, Item->GetContentId(), 1}});
			ListenToFeedback();
			UVestigeTestToolAbility::ResetRefusals();
			Grid->GetField()->Till(TargetCoordinates);
			Grid->GetField()->Till(FIntPoint(4, 0));

			// The press on a tilled tile is refused with its reason.
			FarmingComponent->StartUsingTool();
			ASSERT_THAT(IsTrue(UVestigeTestToolAbility::GetRefusedPresses() == 1));
			ASSERT_THAT(IsTrue(UVestigeTestToolAbility::GetLastRefusal() == VestigeGameplayTags::Feedback_Farm_CantTill));
			ASSERT_THAT(IsTrue(IsUsingTool()));

			// The held repeat reaching another refused tile is not.
			StepToNextTile();
			ASSERT_THAT(IsTrue(UVestigeTestToolAbility::GetRefusedPresses() == 1));
			FarmingComponent->StopUsingTool();

			// A press off the field is the base's own report, which the ability isn't told about.
			UVestigeTestToolAbility::ResetRefusals();
			Reports.Reset();
			Farmer->SetActorLocation(FVector(-500.f, -500.f, 0.f));
			UseToolAndFinish();
			ASSERT_THAT(IsTrue(Reports == TArray<FGameplayTag>{VestigeGameplayTags::Feedback_Tool_NoTarget}));
			ASSERT_THAT(IsTrue(UVestigeTestToolAbility::GetRefusedPresses() == 0));
		}

		TEST_METHOD(TheWarningIsPerCrop)
		{
			ListenToFeedback();
			SetUpLatePlanting(CabbageSeedsName);
			ASSERT_THAT(IsTrue(GetCropOf(PotatoSeedsName)->GetDaysToMature() > 1));
			UseToolAndFinish();

			HoldSeeds(PotatoSeedsName, 3);
			UseToolAndFinish();
			ASSERT_THAT(IsTrue(
				Reports == TArray<FGameplayTag>{VestigeGameplayTags::Feedback_Farm_TooLate, VestigeGameplayTags::Feedback_Farm_TooLate}));
			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Tilled));

			HoldSeeds(CabbageSeedsName, 3);
			UseToolAndFinish();
			ASSERT_THAT(IsTrue(Reports.Num() == 2));
			ASSERT_THAT(IsTrue(TargetTile()->GetCrop() == GetCropOf(CabbageSeedsName)));
		}

		TEST_METHOD(TheWarningIsKeptAcrossAToolSwitch)
		{
			ListenToFeedback();
			SetUpLatePlanting(CabbageSeedsName);
			UseToolAndFinish();

			Hold(TEXT("Base.Hoe"), 0);
			HoldSeeds(CabbageSeedsName, 3);
			UseToolAndFinish();

			ASSERT_THAT(IsTrue(Reports.Num() == 1));
			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Planted));
		}

		TEST_METHOD(AnotherDayWarnsAgain)
		{
			ListenToFeedback();
			SetUpLatePlanting(CabbageSeedsName);
			UseToolAndFinish();

			ASSERT_THAT(IsTrue(JumpToDaysLeft(0)));
			UseToolAndFinish();

			ASSERT_THAT(IsTrue(Reports.Num() == 2));
			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Tilled));
			ASSERT_THAT(IsTrue(GetSeedsOf(CabbageSeedsName) == 3));
		}

		TEST_METHOD(OnTheLastDayTheWarningSaysTomorrow)
		{
			ListenToFeedback();
			SetUpLatePlanting(CabbageSeedsName);
			ASSERT_THAT(IsTrue(JumpToDaysLeft(0)));

			UseToolAndFinish();
			ASSERT_THAT(IsTrue(Reports == TArray<FGameplayTag>{VestigeGameplayTags::Feedback_Farm_TooLate_LastDay}));
			ASSERT_THAT(IsTrue(NumberOf(LastArguments, TEXT("DaysToDeadline")) == 0));
			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Tilled));

			UseToolAndFinish();
			ASSERT_THAT(IsTrue(Reports.Num() == 1));
			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Planted));
		}

		TEST_METHOD(ATileReasonComesBeforeTheWarningAndLeavesItUnasked)
		{
			ListenToFeedback();
			SetUpLatePlanting(CabbageSeedsName);
			// Facing grass: the tile is the reason, and the player isn't warned about the crop yet.
			Farmer->SetActorLocation(FVector(350.f, 50.f, 0.f));
			UseToolAndFinish();
			ASSERT_THAT(IsTrue(Reports == TArray<FGameplayTag>{VestigeGameplayTags::Feedback_Farm_CantPlant}));

			Farmer->SetActorLocation(FVector(250.f, 50.f, 0.f));
			UseToolAndFinish();
			ASSERT_THAT(IsTrue(Reports.Num() == 2));
			ASSERT_THAT(IsTrue(Reports[1] == VestigeGameplayTags::Feedback_Farm_TooLate));
			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Tilled));
		}

		TEST_METHOD(APlantingThatFitsGivesNoWarning)
		{
			ListenToFeedback();
			Grid->GetField()->Till(TargetCoordinates);
			HoldSeeds(CabbageSeedsName, 3);
			ASSERT_THAT(IsTrue(
				GetCropOf(CabbageSeedsName)->GetSeasonFit(Clock().GetCalendar(), Clock().GetCurrentDay()).Fit == EFarmSeasonFit::Fits));

			UseToolAndFinish();

			ASSERT_THAT(IsTrue(Reports.IsEmpty()));
			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Planted));
		}

		TEST_METHOD(TheNextUseWaitsUntilTheLastHasFinished)
		{
			Hold(TEXT("Base.Hoe"), 0);
			FarmingComponent->UseTool();
			ASSERT_THAT(IsTrue(IsUsingTool()));

			// One tile on, so the hoe faces grass again.
			const FFarmTileState* Next = StepToNextTile();
			ASSERT_THAT(IsNotNull(Next));
			FarmingComponent->UseTool();
			ASSERT_THAT(IsTrue(Next->GetState() == EFarmTileState::Grass));

			AbilitySystem->CancelAllAbilities();
			UseToolAndFinish();
			ASSERT_THAT(IsTrue(Next->GetState() == EFarmTileState::Tilled));
		}

		TEST_METHOD(AUseEndsOnceItsTimeHasPassed)
		{
			Hold(TEXT("Base.Hoe"), 0);
			FarmingComponent->UseTool();
			ASSERT_THAT(IsTrue(IsUsingTool()));

			AfterOneUse(
				[this]()
				{
					ASSERT_THAT(IsFalse(IsUsingTool()));
				});
		}

		TEST_METHOD(AtZeroStaminaToolsStillWork)
		{
			Hold(TEXT("Base.Hoe"), 0);
			EmptyStamina();

			UseToolAndFinish();

			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Tilled));
			ASSERT_THAT(IsTrue(GetStamina() == 0.f));
		}

		TEST_METHOD(AtZeroStaminaAUseTakesTwiceAsLong)
		{
			Hold(TEXT("Base.Hoe"), 0);
			const UVestigeGameplayAbility_UseTool* UseTool = GetDefault<UVestigeGameplayAbility_UseTool>();
			const float RestedSeconds = UseTool->GetUseSeconds(GetHeldTool(), *AbilitySystem);

			EmptyStamina();

			ASSERT_THAT(IsTrue(RestedSeconds == GetHeldTool().GetUseSeconds()));
			ASSERT_THAT(IsTrue(UseTool->GetUseSeconds(GetHeldTool(), *AbilitySystem) == 2.f * RestedSeconds));
		}

		TEST_METHOD(AUseThatChangesTheFarmTellsBlueprintsTheToolAndTheTile)
		{
			Hold(TEXT("Base.Hoe"), 0);
			const TStrongObjectPtr<UVestigeTestEventRecorder> Recorder = ListenToBlueprintEvents();

			UseToolAndFinish();

			const TArray<FString> Expected = {FString::Printf(TEXT("ToolUsed Base.Hoe %s"), *TargetCoordinates.ToString())};
			ASSERT_THAT(IsTrue(Recorder->GetEvents() == Expected));
		}

		TEST_METHOD(HarvestingTellsBlueprintsTheCropThenTheTool)
		{
			const UFarmCropData* Crop = GrowRipeCrop();
			ASSERT_THAT(IsNotNull(Crop));
			Hold(TEXT("Base.Hand"), 0);
			const TStrongObjectPtr<UVestigeTestEventRecorder> Recorder = ListenToBlueprintEvents();

			HarvestAndFinish();

			const FString Tile = TargetCoordinates.ToString();
			const TArray<FString> Expected = {FString::Printf(TEXT("CropHarvested %s %s"), *Crop->GetContentId().ToString(), *Tile),
				FString::Printf(TEXT("ToolUsed Base.Hand %s"), *Tile)};
			ASSERT_THAT(IsTrue(Recorder->GetEvents() == Expected));
		}

		TEST_METHOD(AUseThatChangesNothingTellsBlueprintsNothing)
		{
			Hold(TEXT("Base.WateringCan"), 0);
			const TStrongObjectPtr<UVestigeTestEventRecorder> Recorder = ListenToBlueprintEvents();

			UseToolAndFinish();

			ASSERT_THAT(IsTrue(Recorder->GetEvents().IsEmpty()));
		}

		/** A bag with the hoe, the watering can and two stacks of turnip seeds in hotbar slots 0 to 3, the hoe in hand. */
		void FillHotbar()
		{
			SetBag({{0, TEXT("Base.Hoe"), 1}, {1, TEXT("Base.WateringCan"), 1}, {2, TurnipSeedsId().PrimaryAssetName, 5},
				{3, TurnipSeedsId().PrimaryAssetName, 5}});
		}

		/** The tool asset with this ContentId. */
		static const UVestigeToolData* GetBaseTool(FName ContentId)
		{
			return UAssetManager::Get().GetPrimaryAssetObject<UVestigeToolData>(FPrimaryAssetId(UVestigeToolData::AssetType, ContentId));
		}

		TEST_METHOD(TheToolOfTheItemInHandIsGranted)
		{
			FillHotbar();
			TArray<const FGameplayAbilitySpec*> Specs = GetToolAbilities();
			ASSERT_THAT(IsTrue(Specs.Num() == 1));
			ASSERT_THAT(IsTrue(Specs[0]->SourceObject.Get() == GetBaseTool(TEXT("Base.Hoe"))));
			ASSERT_THAT(IsTrue(Specs[0]->Ability->GetClass() == GetHeldTool().GetAbility()));

			// Selected without a press: the farming component follows the hand.
			ASSERT_THAT(IsTrue(Inventory->SelectHotbarSlot(1)));
			Specs = GetToolAbilities();
			ASSERT_THAT(IsTrue(Specs.Num() == 1));
			ASSERT_THAT(IsTrue(Specs[0]->SourceObject.Get() == GetBaseTool(TEXT("Base.WateringCan"))));

			ASSERT_THAT(IsTrue(Inventory->SelectHotbarSlot(2)));
			const FGameplayAbilitySpecHandle SeedsSpec = GetToolAbilities()[0]->Handle;
			ASSERT_THAT(IsTrue(GetToolAbilities()[0]->SourceObject.Get() == GetBaseTool(TEXT("Base.Seeds"))));

			// Another stack of seeds uses the same tool, so the grant stays.
			ASSERT_THAT(IsTrue(Inventory->SelectHotbarSlot(3)));
			Specs = GetToolAbilities();
			ASSERT_THAT(IsTrue(Specs.Num() == 1));
			ASSERT_THAT(IsTrue(Specs[0]->Handle == SeedsSpec));
		}

		TEST_METHOD(SelectingASlotOfAnotherToolBroadcastsOnToolChanged)
		{
			FillHotbar();
			int32 Changes = 0;
			const FDelegateHandle Counter = FarmingComponent->OnToolChanged.AddLambda(
				[&Changes]()
				{
					++Changes;
				});

			Inventory->SelectHotbarSlot(1);
			ASSERT_THAT(IsTrue(Changes == 1));
			Inventory->SelectHotbarSlot(2);
			ASSERT_THAT(IsTrue(Changes == 2));
			Inventory->SelectHotbarSlot(3);
			ASSERT_THAT(IsTrue(Changes == 2));
			FarmingComponent->OnToolChanged.Remove(Counter);
		}

		TEST_METHOD(AFarmerPossessedBeforeBeginPlayFollowsTheHand)
		{
			// As a map's first pawn is: possessed before the world begins play.
			UClass* FarmerClass =
				LoadClass<AVestigePlayerCharacter>(nullptr, TEXT("/Game/Vestige/Blueprints/BP_PlayerCharacter.BP_PlayerCharacter_C"));
			ASSERT_THAT(IsNotNull(FarmerClass));
			AVestigePlayerCharacter& NewFarmer = Spawner.SpawnActor<AVestigePlayerCharacter>(FActorSpawnParameters(), FarmerClass);
			Controller->Possess(&NewFarmer);
			NewFarmer.DispatchBeginPlay();
			UFarmingComponent* NewFarming = NewFarmer.FindComponentByClass<UFarmingComponent>();
			ASSERT_THAT(IsNotNull(NewFarming));
			FillHotbar();
			int32 Changes = 0;
			const FDelegateHandle Counter = NewFarming->OnToolChanged.AddLambda(
				[&Changes]()
				{
					++Changes;
				});

			Inventory->SelectHotbarSlot(1);

			ASSERT_THAT(IsTrue(Changes == 1));
			ASSERT_THAT(IsTrue(NewFarming->GetCurrentTool() == GetBaseTool(TEXT("Base.WateringCan"))));
			NewFarming->OnToolChanged.Remove(Counter);
		}

		TEST_METHOD(HarvestingWorksWithAnyItemInHand)
		{
			const UFarmCropData* Crop = GrowRipeCropAt(TargetCoordinates);
			ASSERT_THAT(IsNotNull(Crop));
			Hold(TEXT("Base.Hoe"), 0);

			HarvestAndFinish();

			ASSERT_THAT(IsTrue(Inventory->GetItemCount(Crop->GetHarvestItemId()) == Crop->GetYield()));
			ASSERT_THAT(IsTrue(GrowRipeCropAt(TargetCoordinates) != nullptr));
			Hold(TEXT("Base.Seeds"), 3);

			HarvestAndFinish();

			ASSERT_THAT(IsTrue(Inventory->GetItemCount(Crop->GetHarvestItemId()) == Crop->GetYield()));
			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Tilled));
		}

		TEST_METHOD(AnEmptyHandUsesNothing)
		{
			Hold(TEXT("Base.Hand"), 0);
			ASSERT_THAT(IsNull(FarmingComponent->GetCurrentTool()));
			ListenToFeedback();

			FarmingComponent->StartUsingTool();

			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Grass));
			ASSERT_THAT(IsTrue(GetStamina() == GetMaxStamina()));
			ASSERT_THAT(IsTrue(Reports == TArray<FGameplayTag>{VestigeGameplayTags::Feedback_Tool_NothingInHand}));
			ASSERT_THAT(IsFalse(FarmingComponent->IsToolUseHeld()));
			FarmingComponent->StopUsingTool();
		}

		TEST_METHOD(TheHarvestIsAlwaysGranted)
		{
			FillHotbar();
			TArray<const FGameplayAbilitySpec*> Harvests = GetHarvestAbilities();
			ASSERT_THAT(IsTrue(Harvests.Num() == 1));
			const FGameplayAbilitySpecHandle HarvestSpec = Harvests[0]->Handle;

			for (int32 Slot : {1, 2, 5, 0})
			{
				ASSERT_THAT(IsTrue(Inventory->SelectHotbarSlot(Slot)));
				Harvests = GetHarvestAbilities();
				ASSERT_THAT(IsTrue(Harvests.Num() == 1));
				ASSERT_THAT(IsTrue(Harvests[0]->Handle == HarvestSpec));
			}
			ASSERT_THAT(IsTrue(GetToolAbilities().Num() == 1));
		}

		TEST_METHOD(WithoutACatalogueThereIsNoHarvestTool)
		{
			// A world without a game instance (an editor preview) has no catalogue.
			FActorTestSpawner WorldWithoutAGame;
			AActor& Owner = WorldWithoutAGame.SpawnActor<AActor>();
			UFarmingComponent* Component = NewObject<UFarmingComponent>(&Owner);
			ASSERT_THAT(IsNull(UVestigeContentCatalog::Get(&WorldWithoutAGame.GetWorld())));

			ASSERT_THAT(IsNull(Component->GetHarvestTool()));
			ASSERT_THAT(IsNull(Component->GetCurrentTool()));
		}

		TEST_METHOD(TheFarmerBlueprintNamesTheHarvestTool)
		{
			ASSERT_THAT(IsNotNull(FarmingComponent->GetHarvestTool()));
			ASSERT_THAT(IsTrue(FarmingComponent->GetHarvestTool() == GetBaseTool(TEXT("Base.Hand"))));
		}

		TEST_METHOD(PlantingUsesTheSeedInHand)
		{
			// A second kind of seed, growing a crop of its own, beside the turnip seeds.
			UFarmCropData* Beet = NewObject<UFarmCropData>();
			Beet->SetContentIdForTests(TEXT("Test.Beet"));
			ASSERT_THAT(IsTrue(Catalog().AddForTests(*Beet)));
			UVestigeItemData* BeetSeeds = NewObject<UVestigeItemData>();
			BeetSeeds->SetContentIdForTests(TEXT("Test.BeetSeeds"));
			UFarmSeedFragment* BeetSeed = NewObject<UFarmSeedFragment>(BeetSeeds);
			BeetSeed->SetCropIdForTests(Beet->GetPrimaryAssetId());
			BeetSeeds->AddFragmentForTests(BeetSeed);
			UVestigeToolFragment* BeetTool = NewObject<UVestigeToolFragment>(BeetSeeds);
			BeetTool->SetToolIdForTests(FPrimaryAssetId(UVestigeToolData::AssetType, TEXT("Base.Seeds")));
			BeetSeeds->AddFragmentForTests(BeetTool);
			const UVestigeItemData* TurnipSeeds = GetBaseItem(TurnipSeedsId().PrimaryAssetName);
			ASSERT_THAT(IsNotNull(TurnipSeeds));
			ASSERT_THAT(IsTrue(Catalog().AddForTests(*BeetSeeds)));
			SetBag({{0, TurnipSeedsId().PrimaryAssetName, 3}, {1, BeetSeeds->GetContentId(), 3}, {5, TurnipSeedsId().PrimaryAssetName, 3}}, 1);
			Grid->GetField()->Till(TargetCoordinates);

			UseToolAndFinish();

			ASSERT_THAT(IsTrue(TargetTile()->GetCrop() == Beet));
			ASSERT_THAT(IsTrue(Inventory->GetSlots()[1].Count == 2));

			// The turnip seeds in hand plant a turnip and come from the stack in hand, not the other stack.
			const FFarmTileState* Next = StepToNextTile();
			ASSERT_THAT(IsNotNull(Next));
			Grid->GetField()->Till(FIntPoint(4, 0));
			ASSERT_THAT(IsTrue(Inventory->SelectHotbarSlot(0)));

			UseToolAndFinish();

			ASSERT_THAT(IsTrue(NextTile()->GetCrop() != nullptr));
			ASSERT_THAT(IsTrue(NextTile()->GetCrop()->GetContentId() == TEXT("Base.Turnip")));
			ASSERT_THAT(IsTrue(Inventory->GetSlots()[0].Count == 2));
			ASSERT_THAT(IsTrue(Inventory->GetSlots()[5].Count == 3));
		}

		TEST_METHOD(PlantingTheLastSeedLeavesTheHandEmpty)
		{
			Grid->GetField()->Till(TargetCoordinates);
			Hold(TEXT("Base.Seeds"), 1);

			FarmingComponent->UseTool();
			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Planted));
			ASSERT_THAT(IsTrue(Inventory->GetItemInHand() == nullptr));

			AfterOneUse(
				[this]()
				{
					ASSERT_THAT(IsFalse(IsUsingTool()));
					ASSERT_THAT(IsTrue(GetToolAbilities().IsEmpty()));
					ASSERT_THAT(IsTrue(GetHarvestAbilities().Num() == 1));
				});
		}

		TEST_METHOD(PlantingTheLastSeedOnARepeatLeavesTheHandEmpty)
		{
			Grid->GetField()->Till(TargetCoordinates);
			Grid->GetField()->Till(FIntPoint(4, 0));
			Hold(TEXT("Base.Seeds"), 2);

			FarmingComponent->StartUsingTool();
			ASSERT_THAT(IsTrue(GetSeeds() == 1));

			// The repeat plants the last seed on the next tile once the first use's time has passed.
			AfterOneUse(
				[this]()
				{
					const FFarmTileState* Next = StepToNextTile();
					ASSERT_THAT(IsTrue(Next->GetState() == EFarmTileState::Planted));
					ASSERT_THAT(IsTrue(GetSeeds() == 0));
				});
			AfterOneUse(
				[this]()
				{
					ASSERT_THAT(IsFalse(IsUsingTool()));
					ASSERT_THAT(IsTrue(GetToolAbilities().IsEmpty()));
					ASSERT_THAT(IsTrue(GetHarvestAbilities().Num() == 1));
					FarmingComponent->StopUsingTool();
				});
		}

		TEST_METHOD(ASecondHandChangeDuringTheFirstUseLeavesOneToolAbility)
		{
			// The last seed empties the hand inside the first use, which holds the ability list's lock. The handler then selects
			// the watering can, whose grant is queued, and when the can comes into hand, the hoe, which takes the queued grant
			// back.
			Grid->GetField()->Till(TargetCoordinates);
			SetBag({{0, TurnipSeedsId().PrimaryAssetName, 1}, {1, TEXT("Base.Hoe"), 1}, {2, TEXT("Base.WateringCan"), 1}});
			const UVestigeToolData* Can = GetBaseTool(TEXT("Base.WateringCan"));
			const FDelegateHandle Switcher = FarmingComponent->OnToolChanged.AddLambda(
				[this, Can]()
				{
					const UVestigeToolData* Tool = FarmingComponent->GetCurrentTool();
					if (!Tool)
					{
						Inventory->SelectHotbarSlot(2);
					}
					else if (Tool == Can)
					{
						Inventory->SelectHotbarSlot(1);
					}
				});

			FarmingComponent->UseTool();
			FarmingComponent->OnToolChanged.Remove(Switcher);
			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Planted));

			AfterOneUse(
				[this]()
				{
					ASSERT_THAT(IsFalse(IsUsingTool()));
					const UVestigeToolData* Hoe = GetBaseTool(TEXT("Base.Hoe"));
					const TArray<const FGameplayAbilitySpec*> Specs = GetToolAbilities();
					ASSERT_THAT(IsTrue(Specs.Num() == 1));
					ASSERT_THAT(IsTrue(Specs[0]->SourceObject.Get() == Hoe));
					ASSERT_THAT(IsTrue(GetHarvestAbilities().Num() == 1));
					ASSERT_THAT(IsTrue(Farmer->GetHeldToolMeshForTests() == Hoe->GetHeldMesh()));
				});
		}

		TEST_METHOD(ANewPawnLeavesNoToolAbilityBehind)
		{
			// The controller with the player state possesses a second farmer, as respawning would.
			ASSERT_THAT(IsTrue(GetToolAbilities().Num() == 1));

			AVestigePlayerCharacter& SecondFarmer = SpawnFarmer();
			Controller->Possess(&SecondFarmer);

			const TArray<const FGameplayAbilitySpec*> Specs = GetToolAbilities();
			ASSERT_THAT(IsTrue(Specs.Num() == 1));
			ASSERT_THAT(IsTrue(GetHarvestAbilities().Num() == 1));
			const UFarmingComponent* SecondFarming = SecondFarmer.FindComponentByClass<UFarmingComponent>();
			ASSERT_THAT(IsNotNull(SecondFarming));
			ASSERT_THAT(IsTrue(Specs[0]->SourceObject.Get() == SecondFarming->GetCurrentTool()));
		}

		TEST_METHOD(SwitchingToolsDuringAUseWaitsForTheUseToFinish)
		{
			Hold(TEXT("Base.Hoe"), 0);
			FarmingComponent->UseTool();
			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Tilled));

			Hold(TEXT("Base.WateringCan"), 0);
			FarmingComponent->UseTool();
			ASSERT_THAT(IsFalse(TargetTile()->IsWatered()));
			ASSERT_THAT(IsTrue(IsUsingTool()));

			AfterOneUse(
				[this]()
				{
					ASSERT_THAT(IsFalse(IsUsingTool()));
					ASSERT_THAT(IsTrue(GetToolAbilities().Num() == 1));
					FarmingComponent->UseTool();
					ASSERT_THAT(IsTrue(TargetTile()->IsWatered()));
					AbilitySystem->CancelAllAbilities();
				});
		}

		TEST_METHOD(ANewToolNeedsOnlyItsAbility)
		{
			// A tool no base tool is like, through a new ability, a tool asset and an item alone.
			UVestigeToolData* Tool = NewObject<UVestigeToolData>();
			Tool->SetContentIdForTests(TEXT("Test.Tiller"));
			Tool->SetAbilityForTests(UVestigeTestToolAbility::StaticClass());
			ASSERT_THAT(IsTrue(Catalog().AddForTests(*Tool)));
			UVestigeItemData* Item = NewObject<UVestigeItemData>();
			Item->SetContentIdForTests(TEXT("Test.Tiller"));
			UVestigeToolFragment* ToolFragment = NewObject<UVestigeToolFragment>(Item);
			ToolFragment->SetToolIdForTests(Tool->GetPrimaryAssetId());
			Item->AddFragmentForTests(ToolFragment);
			ASSERT_THAT(IsTrue(Catalog().AddForTests(*Item)));
			SetBag({{0, Item->GetContentId(), 1}});
			ASSERT_THAT(IsTrue(FarmingComponent->GetCurrentTool() == Tool));

			UseToolAndFinish();

			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Tilled));
			ASSERT_THAT(IsTrue(TargetTile()->IsWatered()));
			ASSERT_THAT(IsTrue(GetStamina() == GetMaxStamina() - Tool->GetStaminaCost()));
		}

		TEST_METHOD(HoldingTheUseKeyWorksTheNextTileOnceTheUseHasFinished)
		{
			Hold(TEXT("Base.Hoe"), 0);
			FarmingComponent->StartUsingTool();
			const FFarmTileState* Next = StepToNextTile();
			ASSERT_THAT(IsNotNull(Next));
			ASSERT_THAT(IsTrue(Next->GetState() == EFarmTileState::Grass));

			AfterOneUse(
				[this]()
				{
					ASSERT_THAT(IsTrue(NextTile()->GetState() == EFarmTileState::Tilled));
					AbilitySystem->CancelAllAbilities();
				});
		}

		TEST_METHOD(HoldingTheUseKeyWorksTheNextTileWhenTheTargetChanges)
		{
			Hold(TEXT("Base.Hoe"), 0);
			FarmingComponent->StartUsingTool();

			AfterOneUse(
				[this]()
				{
					const FFarmTileState* Next = StepToNextTile();
					ASSERT_THAT(IsNotNull(Next));
					ASSERT_THAT(IsTrue(Next->GetState() == EFarmTileState::Tilled));
					AbilitySystem->CancelAllAbilities();
				});
		}

		TEST_METHOD(ARepeatThatWouldChangeNothingIsSkippedQuietly)
		{
			Hold(TEXT("Base.Hoe"), 0);
			ListenToFeedback();
			FarmingComponent->StartUsingTool();

			// The use ends on the tile it just tilled, which the hoe can't change again, so the repeat waits for another.
			AfterOneUse(
				[this]()
				{
					// The till worked, which reports nothing, and the repeat that would change nothing waits quietly.
					ASSERT_THAT(IsTrue(Reports.IsEmpty()));
					ASSERT_THAT(IsTrue(GetStamina() == GetMaxStamina() - GetHeldTool().GetStaminaCost()));

					FarmingComponent->StopUsingTool();
					ASSERT_THAT(IsFalse(IsUsingTool()));
				});
		}

		TEST_METHOD(ThePressSaysWhyAUseChangesNothingAndTheRepeatWaitsForTheNextTile)
		{
			Hold(TEXT("Base.Hoe"), 0);
			Grid->GetField()->Till(TargetCoordinates);
			ListenToFeedback();

			FarmingComponent->StartUsingTool();
			ASSERT_THAT(IsTrue(Reports == TArray<FGameplayTag>{VestigeGameplayTags::Feedback_Farm_CantTill}));

			const FFarmTileState* Next = StepToNextTile();
			ASSERT_THAT(IsNotNull(Next));
			ASSERT_THAT(IsTrue(Next->GetState() == EFarmTileState::Tilled));
			AbilitySystem->CancelAllAbilities();
		}

		TEST_METHOD(NoToolWorksInTheAir)
		{
			Hold(TEXT("Base.Hoe"), 0);
			Farmer->GetCharacterMovement()->SetMovementMode(MOVE_Falling);

			FarmingComponent->UseTool();

			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Grass));
			ASSERT_THAT(IsTrue(GetStamina() == GetMaxStamina()));
			ASSERT_THAT(IsFalse(IsUsingTool()));
		}

		TEST_METHOD(LandingWithTheUseKeyHeldWorksTheTarget)
		{
			Hold(TEXT("Base.Hoe"), 0);
			Farmer->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
			FarmingComponent->StartUsingTool();
			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Grass));

			Farmer->GetCharacterMovement()->SetMovementMode(MOVE_Walking);

			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Tilled));
			AbilitySystem->CancelAllAbilities();
		}

		TEST_METHOD(ReleasingTheUseKeyStopsTheRepeat)
		{
			Hold(TEXT("Base.Hoe"), 0);
			FarmingComponent->StartUsingTool();
			FarmingComponent->StopUsingTool();
			ASSERT_THAT(IsFalse(FarmingComponent->IsToolUseHeld()));

			AfterOneUse(
				[this]()
				{
					ASSERT_THAT(IsFalse(IsUsingTool()));
					const FFarmTileState* Next = StepToNextTile();
					ASSERT_THAT(IsNotNull(Next));
					ASSERT_THAT(IsTrue(Next->GetState() == EFarmTileState::Grass));
				});
		}

		TEST_METHOD(APressDuringTheLastUsesTimeKeepsTheRepeat)
		{
			Hold(TEXT("Base.Hoe"), 0);
			FarmingComponent->StartUsingTool();
			FarmingComponent->StopUsingTool();
			FarmingComponent->StartUsingTool();
			const FFarmTileState* Next = StepToNextTile();
			ASSERT_THAT(IsNotNull(Next));
			ASSERT_THAT(IsTrue(Next->GetState() == EFarmTileState::Grass));

			AfterOneUse(
				[this]()
				{
					ASSERT_THAT(IsTrue(NextTile()->GetState() == EFarmTileState::Tilled));
					AbilitySystem->CancelAllAbilities();
				});
		}
		/** The farmer's interaction component, which finds what F would use. */
		UVestigeInteractionComponent& GetInteraction() const { return *Farmer->FindComponentByClass<UVestigeInteractionComponent>(); }

		/** Stands the farmer at Location (on the ground of the grid) aiming along AimDirection, and lets the target follow. */
		void StandAt(const FVector& Location, const FVector& AimDirection)
		{
			UFarmTargetingComponent* Targeting = Farmer->FindComponentByClass<UFarmTargetingComponent>();
			Farmer->SetActorLocation(Location);
			Targeting->AimAt(AimDirection);
		}

		/**
		 * A bed lying along the field's near edge (row 0's outer side), its long side on the edge, centred 400 cm along it, as
		 * L_Farm places one: it covers X 300 to 500 and Y -100 to 0.
		 */
		AVestigeBed& PlaceBedAlongTheEdge()
		{
			AVestigeBed& Bed = Spawner.SpawnActor<AVestigeBed>();
			Bed.SetActorLocation(FVector(400.f, -50.f, 0.f));
			return Bed;
		}

		TEST_METHOD(FacingARipeTileOffersHarvest)
		{
			ASSERT_THAT(IsNotNull(GrowRipeCropAt(TargetCoordinates)));
			TArray<FVestigeInteractionOption> Options;

			IVestigeInteractable::Execute_GetInteractionOptions(Grid, Farmer, Options);

			ASSERT_THAT(IsTrue(Options.Num() == 1));
			ASSERT_THAT(IsTrue(Options[0].Action == AFarmGrid::HarvestAction));
			ASSERT_THAT(IsTrue(Options[0].bHasLocation));
			ASSERT_THAT(IsTrue(Options[0].Location.Equals(Grid->GetTileTransform(TargetCoordinates).GetLocation())));
		}

		TEST_METHOD(AnUnripeOrEmptyTileOffersNothing)
		{
			TArray<FVestigeInteractionOption> Options;
			IVestigeInteractable::Execute_GetInteractionOptions(Grid, Farmer, Options);
			ASSERT_THAT(IsTrue(Options.IsEmpty()));

			Grid->GetField()->Till(TargetCoordinates);
			Grid->GetField()->Plant(
				TargetCoordinates, Catalog().Find<UFarmCropData>(FPrimaryAssetId(UFarmCropData::AssetType, TEXT("Base.Turnip"))));
			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Planted));
			IVestigeInteractable::Execute_GetInteractionOptions(Grid, Farmer, Options);
			ASSERT_THAT(IsTrue(Options.IsEmpty()));
		}

		TEST_METHOD(NextToTheBedTheFacedRipeTileWins)
		{
			PlaceBedAlongTheEdge();

			// On row 0 beside the bed, 42 cm in from its side, aiming along the edge with the bed's centre this far ahead.
			for (float Ahead : {60.f, 100.f, 150.f})
			{
				StandAt(FVector(400.f - Ahead, 42.f, 0.f), FVector(1.f, 0.f, 0.f));
				const TOptional<FIntPoint> Target = Farmer->FindComponentByClass<UFarmTargetingComponent>()->GetTargetTile();
				ASSERT_THAT(IsTrue(Target.IsSet()));
				if (!Grid->GetTile(*Target)->IsRipe())
				{
					ASSERT_THAT(IsNotNull(GrowRipeCropAt(*Target)));
				}

				// Turned off the aim as while walking: the aim decides, not the body.
				Farmer->SetActorRotation(FRotator(0.f, 22.5f, 0.f));
				GetInteraction().Scan();

				ASSERT_THAT(IsTrue(GetInteraction().GetFocusedActor() == Grid));
				ASSERT_THAT(IsTrue(GetInteraction().GetFocusedOption().Action == AFarmGrid::HarvestAction));
			}
		}

		TEST_METHOD(NextToTheBedAnUnripeTargetFocusesNothing)
		{
			PlaceBedAlongTheEdge();

			for (float Ahead : {60.f, 100.f, 150.f})
			{
				StandAt(FVector(400.f - Ahead, 42.f, 0.f), FVector(1.f, 0.f, 0.f));
				GetInteraction().Scan();

				ASSERT_THAT(IsNull(GetInteraction().GetFocusedActor()));
			}
		}

		TEST_METHOD(FacingTheBedFocusesTheBed)
		{
			AVestigeBed& Bed = PlaceBedAlongTheEdge();

			// Facing its long side near one end, from beyond the field.
			StandAt(FVector(320.f, -142.f, 0.f), FVector(0.f, 1.f, 0.f));
			GetInteraction().Scan();
			ASSERT_THAT(IsTrue(GetInteraction().GetFocusedActor() == &Bed));

			// Facing its end, from the ring of tiles around the field.
			StandAt(FVector(560.f, -50.f, 0.f), FVector(-1.f, 0.f, 0.f));
			GetInteraction().Scan();
			ASSERT_THAT(IsTrue(GetInteraction().GetFocusedActor() == &Bed));
		}

		TEST_METHOD(InteractingWithARipeTileHarvestsIt)
		{
			const UFarmCropData* Crop = GrowRipeCropAt(TargetCoordinates);
			ASSERT_THAT(IsNotNull(Crop));
			Hold(TEXT("Base.Hoe"), 0);
			const TStrongObjectPtr<UVestigeTestEventRecorder> Recorder = ListenToBlueprintEvents();

			// The interact key, through the player's interact ability.
			GetInteraction().Interact();

			ASSERT_THAT(IsTrue(Inventory->GetItemCount(Crop->GetHarvestItemId()) == Crop->GetYield()));
			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Tilled));
			ASSERT_THAT(IsTrue(GetStamina() == GetMaxStamina() - GetBaseTool(TEXT("Base.Hand"))->GetStaminaCost()));
			const FString Tile = TargetCoordinates.ToString();
			const TArray<FString> Expected = {FString::Printf(TEXT("CropHarvested %s %s"), *Crop->GetContentId().ToString(), *Tile),
				FString::Printf(TEXT("ToolUsed Base.Hand %s"), *Tile)};
			ASSERT_THAT(IsTrue(Recorder->GetEvents() == Expected));
			ASSERT_THAT(IsNull(GetInteraction().GetFocusedActor()));
			AbilitySystem->CancelAllAbilities();
		}

		TEST_METHOD(FDuringAToolUseDoesntHarvest)
		{
			ASSERT_THAT(IsNotNull(GrowRipeCropAt(TargetCoordinates)));
			Hold(TEXT("Base.Hoe"), 0);

			// The hoe can't change a ripe tile, so with E held its ability waits for another tile, holding the tool use.
			FarmingComponent->StartUsingTool();
			ASSERT_THAT(IsTrue(IsUsingTool()));

			GetInteraction().Interact();

			ASSERT_THAT(IsTrue(TargetTile()->IsRipe()));
			bool bInteractActive = false;
			for (const FGameplayAbilitySpec& Spec : AbilitySystem->GetActivatableAbilities())
			{
				bInteractActive |=
					Spec.IsActive() && Spec.Ability && Spec.Ability->GetAssetTags().HasTagExact(VestigeGameplayTags::Ability_Interact);
			}
			ASSERT_THAT(IsFalse(bInteractActive));
			FarmingComponent->StopUsingTool();
		}

		/** Tills, plants a turnip in, waters once and withers the target tile, through the field's rules. */
		void WitherTargetCrop()
		{
			UFarmField* Field = Grid->GetField();
			const UFarmCropData* Crop = Catalog().Find<UFarmCropData>(FPrimaryAssetId(UFarmCropData::AssetType, TEXT("Base.Turnip")));
			ASSERT_THAT(IsNotNull(Crop));
			Field->Till(TargetCoordinates);
			ASSERT_THAT(IsTrue(Field->Plant(TargetCoordinates, Crop)));
			Field->Water(TargetCoordinates);
			ASSERT_THAT(IsTrue(Field->WitherCropsOutOfSeason(GetSeason(1).Tag, Clock().GetCalendar()) == 1));
			ASSERT_THAT(IsTrue(TargetTile()->IsWithered()));
		}

		TEST_METHOD(FacingAWitheredTileOffersClearNotHarvest)
		{
			WitherTargetCrop();
			TArray<FVestigeInteractionOption> Options;

			IVestigeInteractable::Execute_GetInteractionOptions(Grid, Farmer, Options);

			ASSERT_THAT(IsTrue(Options.Num() == 1));
			ASSERT_THAT(IsTrue(Options[0].Action == AFarmGrid::ClearAction));
			ASSERT_THAT(IsTrue(Options[0].Action != AFarmGrid::HarvestAction));
			ASSERT_THAT(IsTrue(Options[0].bHasLocation));
			ASSERT_THAT(IsTrue(Options[0].Location.Equals(Grid->GetTileTransform(TargetCoordinates).GetLocation())));
		}

		TEST_METHOD(InteractingWithAWitheredTileClearsItForTheHandsStamina)
		{
			WitherTargetCrop();
			Hold(TEXT("Base.Hoe"), 0);
			const TStrongObjectPtr<UVestigeTestEventRecorder> Recorder = ListenToBlueprintEvents();
			const int32 WaterBefore = TargetTile()->GetWaterLevel();

			GetInteraction().Interact();

			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Tilled));
			ASSERT_THAT(IsNull(TargetTile()->GetCrop()));
			ASSERT_THAT(IsTrue(TargetTile()->GetWaterLevel() == WaterBefore));
			ASSERT_THAT(IsTrue(GetStamina() == GetMaxStamina() - GetBaseTool(TEXT("Base.Hand"))->GetStaminaCost()));
			const TArray<FString> Expected = {FString::Printf(TEXT("ToolUsed Base.Hand %s"), *TargetCoordinates.ToString())};
			ASSERT_THAT(IsTrue(Recorder->GetEvents() == Expected));
			AbilitySystem->CancelAllAbilities();
		}

		TEST_METHOD(AFullBagStillClearsAWitheredCrop)
		{
			WitherTargetCrop();
			Hold(TEXT("Base.Hand"), 0);
			const UVestigeItemData* Seed = GetBaseItem(TurnipSeedsId().PrimaryAssetName);
			ASSERT_THAT(IsNotNull(Seed));
			TArray<FVestigeInventorySlotSaveData> FullBag;
			for (int32 Slot = 0; Slot < Inventory->GetSlots().Num(); ++Slot)
			{
				FullBag.Add({Slot, Seed->GetContentId(), Seed->GetMaxStackSize()});
			}
			SetBag(MoveTemp(FullBag));
			ListenToFeedback();

			HarvestAndFinish();

			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Tilled));
			ASSERT_THAT(IsTrue(Reports.IsEmpty()));
		}

		TEST_METHOD(TheHoeOnAWitheredTileIsRefusedAndCostsNothing)
		{
			WitherTargetCrop();
			Hold(TEXT("Base.Hoe"), 0);
			ListenToFeedback();
			const TStrongObjectPtr<UVestigeTestEventRecorder> Recorder = ListenToBlueprintEvents();

			UseToolAndFinish();

			ASSERT_THAT(IsTrue(Reports == TArray<FGameplayTag>{VestigeGameplayTags::Feedback_Farm_ClearFirst}));
			ASSERT_THAT(IsTrue(TargetTile()->IsWithered()));
			ASSERT_THAT(IsTrue(GetStamina() == GetMaxStamina()));
			ASSERT_THAT(IsTrue(Recorder->GetEvents().IsEmpty()));
		}

		TEST_METHOD(SeedsOnAWitheredTileAreRefusedAndKept)
		{
			WitherTargetCrop();
			Hold(TEXT("Base.Seeds"), 3);
			ListenToFeedback();
			const TStrongObjectPtr<UVestigeTestEventRecorder> Recorder = ListenToBlueprintEvents();

			UseToolAndFinish();

			ASSERT_THAT(IsTrue(Reports == TArray<FGameplayTag>{VestigeGameplayTags::Feedback_Farm_ClearFirst}));
			ASSERT_THAT(IsTrue(TargetTile()->IsWithered()));
			ASSERT_THAT(IsTrue(GetSeeds() == 3));
			ASSERT_THAT(IsTrue(GetStamina() == GetMaxStamina()));
			ASSERT_THAT(IsTrue(Recorder->GetEvents().IsEmpty()));
		}

		TEST_METHOD(TheWateringCanWatersAWitheredTile)
		{
			WitherTargetCrop();
			Hold(TEXT("Base.WateringCan"), 0);
			const int32 WaterBefore = TargetTile()->GetWaterLevel();

			UseToolAndFinish();

			ASSERT_THAT(IsTrue(TargetTile()->GetWaterLevel() == WaterBefore + 1));
			ASSERT_THAT(IsTrue(TargetTile()->IsWithered()));
		}

		TEST_METHOD(TheGridsInteractionVolumeCoversTheFieldAndOneTileAround)
		{
			// Standing on the ring of tiles left of the field, facing in at a ripe edge tile.
			ASSERT_THAT(IsNotNull(GrowRipeCropAt(FIntPoint(0, 3))));
			const float HalfHeight = Farmer->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
			StandAt(FVector(-50.f, 350.f, Grid->GetActorLocation().Z + HalfHeight), FVector(1.f, 0.f, 0.f));
			ASSERT_THAT(
				IsTrue(Farmer->FindComponentByClass<UFarmTargetingComponent>()->GetTargetTile() == TOptional<FIntPoint>(FIntPoint(0, 3))));

			GetInteraction().Scan();

			ASSERT_THAT(IsTrue(GetInteraction().GetFocusedActor() == Grid));
		}

		// Contamination (Systems/27-Contamination.md). The test grid's field starts clean; tiles get a level by SetContamination.
		static inline const FName CleanupSeedsName = TEXT("Base.CleanupCropSeeds");

		/** Sets the contamination of the tile at Coordinates, checking that the level changed. */
		void Contaminate(const FIntPoint& Coordinates, int32 Level)
		{
			ASSERT_THAT(IsTrue(Grid->GetField()->SetContamination(Coordinates, Level)));
		}

		/** Puts a ripe CropId on the target tile at contamination Level, mutated or not, the way loading a save does. */
		const UFarmCropData* RestoreRipeTarget(FName CropId, int32 Level, bool bMutated)
		{
			const UFarmCropData* Crop = Catalog().Find<UFarmCropData>(FPrimaryAssetId(UFarmCropData::AssetType, CropId));
			if (!Crop)
			{
				return nullptr;
			}
			FFarmTileSaveData Saved;
			Saved.Coordinates = TargetCoordinates;
			Saved.State = EFarmTileState::Planted;
			Saved.GrowthDays = Crop->GetDaysToMature();
			Saved.CropId = CropId;
			Saved.Contamination = Level;
			Saved.bMutated = bMutated;
			Grid->GetField()->RestoreTilesFromSaveData({Saved}, {{CropId, Crop}});
			return Crop;
		}

		TEST_METHOD(CleaningSeedsAreRefusedOnCleanSoilAndPlantOnLightSoil)
		{
			ListenToFeedback();
			Grid->GetField()->Till(TargetCoordinates);
			HoldSeeds(CleanupSeedsName, 3);
			const TStrongObjectPtr<UVestigeTestEventRecorder> Recorder = ListenToBlueprintEvents();

			UseToolAndFinish();

			ASSERT_THAT(IsTrue(Reports == TArray<FGameplayTag>{VestigeGameplayTags::Feedback_Farm_NeedsContamination}));
			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Tilled));
			ASSERT_THAT(IsTrue(GetSeedsOf(CleanupSeedsName) == 3));
			ASSERT_THAT(IsTrue(GetStamina() == GetMaxStamina()));
			ASSERT_THAT(IsTrue(Recorder->GetEvents().IsEmpty()));

			Contaminate(TargetCoordinates, 1);
			UseToolAndFinish();

			ASSERT_THAT(IsTrue(Reports.Num() == 1));
			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Planted));
			ASSERT_THAT(IsTrue(TargetTile()->GetCrop() == GetCropOf(CleanupSeedsName)));
			ASSERT_THAT(IsTrue(GetSeedsOf(CleanupSeedsName) == 2));
		}

		TEST_METHOD(NormalSeedsOnHeavySoilAreRefusedOnEveryPress)
		{
			ListenToFeedback();
			Grid->GetField()->Till(TargetCoordinates);
			Contaminate(TargetCoordinates, 2);
			HoldSeeds(TurnipSeedsId().PrimaryAssetName, 3);
			const TStrongObjectPtr<UVestigeTestEventRecorder> Recorder = ListenToBlueprintEvents();

			UseToolAndFinish();
			UseToolAndFinish();

			ASSERT_THAT(IsTrue(Reports
				== TArray<FGameplayTag>{
					VestigeGameplayTags::Feedback_Farm_TooContaminated, VestigeGameplayTags::Feedback_Farm_TooContaminated}));
			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Tilled));
			ASSERT_THAT(IsTrue(GetSeeds() == 3));
			ASSERT_THAT(IsTrue(GetStamina() == GetMaxStamina()));
			ASSERT_THAT(IsTrue(Recorder->GetEvents().IsEmpty()));
		}

		TEST_METHOD(TooContaminatedComesBeforeTooLate)
		{
			ListenToFeedback();
			SetUpLatePlanting(CabbageSeedsName);
			Contaminate(TargetCoordinates, 2);

			UseToolAndFinish();
			UseToolAndFinish();

			ASSERT_THAT(IsTrue(Reports
				== TArray<FGameplayTag>{
					VestigeGameplayTags::Feedback_Farm_TooContaminated, VestigeGameplayTags::Feedback_Farm_TooContaminated}));
			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Tilled));
			ASSERT_THAT(IsTrue(GetSeedsOf(CabbageSeedsName) == 3));
		}

		TEST_METHOD(OnLightSoilTooLateCountsTwiceTheDays)
		{
			const UFarmCropData* Turnip = GetCropOf(TurnipSeedsId().PrimaryAssetName);
			ASSERT_THAT(IsNotNull(Turnip));
			ASSERT_THAT(IsTrue(Turnip->GetWateredNightsPerGrowthDay(1) == 2));
			// Exactly the turnip's days are left: clean soil ripens on the season's last day, light soil can't.
			ASSERT_THAT(IsTrue(JumpToDaysLeft(Turnip->GetDaysToMature())));
			Grid->GetField()->Till(TargetCoordinates);
			Grid->GetField()->Till(FIntPoint(4, 0));
			Contaminate(TargetCoordinates, 1);
			HoldSeeds(TurnipSeedsId().PrimaryAssetName, 3);
			ListenToFeedback();

			StepToNextTile();
			UseToolAndFinish();
			ASSERT_THAT(IsTrue(Reports.IsEmpty()));
			ASSERT_THAT(IsTrue(NextTile()->GetState() == EFarmTileState::Planted));

			Farmer->SetActorLocation(FVector(250.f, 50.f, 0.f));
			UseToolAndFinish();
			ASSERT_THAT(IsTrue(Reports == TArray<FGameplayTag>{VestigeGameplayTags::Feedback_Farm_TooLate}));
			ASSERT_THAT(IsTrue(NumberOf(LastArguments, TEXT("DaysToRipe")) == 2 * Turnip->GetDaysToMature()));
			ASSERT_THAT(IsTrue(NumberOf(LastArguments, TEXT("DaysToDeadline")) == Turnip->GetDaysToMature()));
			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Tilled));
			ASSERT_THAT(IsTrue(GetSeeds() == 2));
		}

		TEST_METHOD(TheTileReasonsComeBeforeTheContaminationReasons)
		{
			ListenToFeedback();
			Contaminate(TargetCoordinates, 2);

			// Grass: the cleaning seeds would need contamination and the turnip a cleaner tile, but the tile isn't tilled.
			HoldSeeds(CleanupSeedsName, 3);
			UseToolAndFinish();
			Contaminate(FIntPoint(4, 0), 2);
			StepToNextTile();
			HoldSeeds(TurnipSeedsId().PrimaryAssetName, 3);
			UseToolAndFinish();
			ASSERT_THAT(IsTrue(Reports
				== TArray<FGameplayTag>{VestigeGameplayTags::Feedback_Farm_CantPlant, VestigeGameplayTags::Feedback_Farm_CantPlant}));

			// A withered crop on heavy soil is cleared first.
			Farmer->SetActorLocation(FVector(250.f, 50.f, 0.f));
			Reports.Reset();
			WitherTargetCrop();
			ASSERT_THAT(IsTrue(TargetTile()->GetContamination() == 2));
			UseToolAndFinish();
			ASSERT_THAT(IsTrue(Reports == TArray<FGameplayTag>{VestigeGameplayTags::Feedback_Farm_ClearFirst}));
		}

		TEST_METHOD(AMutatedCropGivesItsMutatedItem)
		{
			const UFarmCropData* Crop = RestoreRipeTarget(TEXT("Base.Turnip"), 1, true);
			ASSERT_THAT(IsNotNull(Crop));
			ASSERT_THAT(IsTrue(TargetTile()->IsMutated()));
			Hold(TEXT("Base.Hand"), 0);

			HarvestAndFinish();

			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Tilled));
			ASSERT_THAT(IsTrue(Inventory->GetItemCount(Crop->GetMutatedHarvestItemId()) == Crop->GetYield()));
			ASSERT_THAT(IsTrue(Inventory->GetItemCount(Crop->GetHarvestItemId()) == 0));
		}

		TEST_METHOD(TheRoomCheckAsksForTheMutatedItem)
		{
			const UFarmCropData* Crop = RestoreRipeTarget(TEXT("Base.Turnip"), 1, true);
			ASSERT_THAT(IsNotNull(Crop));
			Hold(TEXT("Base.Hand"), 0);
			const UVestigeItemData* Seed = GetBaseItem(TurnipSeedsId().PrimaryAssetName);
			const UVestigeItemData* Turnip = GetBaseItem(TEXT("Base.Turnip"));
			const UVestigeItemData* Mutated = GetBaseItem(Crop->GetMutatedHarvestItemId().PrimaryAssetName);
			ASSERT_THAT(IsTrue(Seed && Turnip && Mutated));

			// Room for a normal harvest only: the mutated one stays in the ground.
			const auto FillBagLeavingRoomFor = [&](const UVestigeItemData& Item)
			{
				TArray<FVestigeInventorySlotSaveData> Bag;
				Bag.Add({0, Item.GetContentId(), Item.GetMaxStackSize() - Crop->GetYield()});
				for (int32 Slot = 1; Slot < Inventory->GetSlots().Num(); ++Slot)
				{
					Bag.Add({Slot, Seed->GetContentId(), Seed->GetMaxStackSize()});
				}
				SetBag(MoveTemp(Bag));
			};
			FillBagLeavingRoomFor(*Turnip);
			ListenToFeedback();

			HarvestAndFinish();
			ASSERT_THAT(IsTrue(Reports == TArray<FGameplayTag>{VestigeGameplayTags::Feedback_Inventory_Full}));
			ASSERT_THAT(IsTrue(TargetTile()->IsMutated()));

			// Room for the mutated harvest: it goes in.
			FillBagLeavingRoomFor(*Mutated);
			HarvestAndFinish();
			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Tilled));
			ASSERT_THAT(IsTrue(Inventory->GetItemCount(Mutated->GetPrimaryAssetId()) == Mutated->GetMaxStackSize()));
		}

		TEST_METHOD(ACleaningHarvestSaysTheSoilIsCleaner)
		{
			ASSERT_THAT(IsNotNull(RestoreRipeTarget(TEXT("Base.CleanupCrop"), 2, false)));
			Hold(TEXT("Base.Hand"), 0);
			ListenToFeedback();

			HarvestAndFinish();

			ASSERT_THAT(IsTrue(Reports == TArray<FGameplayTag>{VestigeGameplayTags::Feedback_Farm_Cleaned}));
			ASSERT_THAT(IsTrue(TargetTile()->GetContamination() == 1));
		}

		TEST_METHOD(ACleaningHarvestThatLeavesTheSoilCleanSaysSo)
		{
			const UFarmCropData* Crop = RestoreRipeTarget(TEXT("Base.CleanupCrop"), 1, false);
			ASSERT_THAT(IsNotNull(Crop));
			Hold(TEXT("Base.Hand"), 0);
			ListenToFeedback();

			HarvestAndFinish();

			ASSERT_THAT(IsTrue(Reports == TArray<FGameplayTag>{VestigeGameplayTags::Feedback_Farm_Cleaned_Clean}));
			ASSERT_THAT(IsTrue(TargetTile()->GetContamination() == 0));
			ASSERT_THAT(IsTrue(Inventory->GetItemCount(Crop->GetHarvestItemId()) == Crop->GetYield()));
		}

		TEST_METHOD(ANormalHarvestOnContaminatedSoilSaysNothingAndLeavesTheLevel)
		{
			ASSERT_THAT(IsNotNull(RestoreRipeTarget(TEXT("Base.Turnip"), 1, false)));
			Hold(TEXT("Base.Hand"), 0);
			ListenToFeedback();

			HarvestAndFinish();

			ASSERT_THAT(IsTrue(Reports.IsEmpty()));
			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Tilled));
			ASSERT_THAT(IsTrue(TargetTile()->GetContamination() == 1));
		}

		// ---- The farm's firsts ----

		UVestigeMilestoneSubsystem& GetMilestones() { return *UVestigeMilestoneSubsystem::Get(&Spawner.GetWorld()); }

		TEST_METHOD(AHarvestReachesTheFirstHarvest)
		{
			ASSERT_THAT(IsNotNull(GrowRipeCrop()));
			Hold(TEXT("Base.Hand"), 0);
			ASSERT_THAT(IsFalse(GetMilestones().HasReached(VestigeGameplayTags::Milestone_Farm_Harvested)));

			HarvestAndFinish();

			ASSERT_THAT(IsTrue(GetMilestones().HasReached(VestigeGameplayTags::Milestone_Farm_Harvested)));
			// A harvest on clean soil of a plain crop is neither a cleaning nor a mutated one.
			ASSERT_THAT(IsFalse(GetMilestones().HasReached(VestigeGameplayTags::Milestone_Farm_Cleaned)));
			ASSERT_THAT(IsFalse(GetMilestones().HasReached(VestigeGameplayTags::Milestone_Farm_HarvestedMutated)));
		}

		TEST_METHOD(AHarvestThatFailsReachesNothing)
		{
			// A full bag leaves the crop in the ground, so nothing was harvested.
			const UFarmCropData* Crop = GrowRipeCrop();
			ASSERT_THAT(IsNotNull(Crop));
			Hold(TEXT("Base.Hand"), 0);
			const UVestigeItemData* Seed = GetBaseItem(TurnipSeedsId().PrimaryAssetName);
			ASSERT_THAT(IsNotNull(Seed));
			TArray<FVestigeInventorySlotSaveData> Full;
			for (int32 Slot = 0; Slot < Inventory->GetSlots().Num(); ++Slot)
			{
				Full.Add({Slot, Seed->GetContentId(), Seed->GetMaxStackSize()});
			}
			SetBag(MoveTemp(Full));

			HarvestAndFinish();

			ASSERT_THAT(IsTrue(TargetTile()->IsRipe()));
			ASSERT_THAT(IsFalse(GetMilestones().HasReached(VestigeGameplayTags::Milestone_Farm_Harvested)));
		}

		TEST_METHOD(ACleaningHarvestReachesTheFirstCleaning)
		{
			// A plain crop on heavy soil is harvested but cleans nothing.
			ASSERT_THAT(IsNotNull(RestoreRipeTarget(TEXT("Base.Turnip"), 1, false)));
			Hold(TEXT("Base.Hand"), 0);
			HarvestAndFinish();
			ASSERT_THAT(IsTrue(GetMilestones().HasReached(VestigeGameplayTags::Milestone_Farm_Harvested)));
			ASSERT_THAT(IsFalse(GetMilestones().HasReached(VestigeGameplayTags::Milestone_Farm_Cleaned)));

			ASSERT_THAT(IsNotNull(RestoreRipeTarget(TEXT("Base.CleanupCrop"), 2, false)));
			HarvestAndFinish();

			ASSERT_THAT(IsTrue(GetMilestones().HasReached(VestigeGameplayTags::Milestone_Farm_Cleaned)));
		}

		TEST_METHOD(AMutatedHarvestReachesItsMilestone)
		{
			ASSERT_THAT(IsNotNull(RestoreRipeTarget(TEXT("Base.Turnip"), 0, false)));
			Hold(TEXT("Base.Hand"), 0);
			HarvestAndFinish();
			ASSERT_THAT(IsFalse(GetMilestones().HasReached(VestigeGameplayTags::Milestone_Farm_HarvestedMutated)));

			ASSERT_THAT(IsNotNull(RestoreRipeTarget(TEXT("Base.Turnip"), 0, true)));
			HarvestAndFinish();

			ASSERT_THAT(IsTrue(GetMilestones().HasReached(VestigeGameplayTags::Milestone_Farm_HarvestedMutated)));
		}

		TEST_METHOD(PlantingReachesThePlantedMilestones)
		{
			ASSERT_THAT(IsFalse(GetMilestones().HasReached(VestigeGameplayTags::Milestone_Farm_Planted)));
			Grid->GetField()->Till(TargetCoordinates);
			Hold(TEXT("Base.Seeds"), 1);

			UseToolAndFinish();

			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Planted));
			ASSERT_THAT(IsTrue(GetMilestones().HasReached(VestigeGameplayTags::Milestone_Farm_Planted)));
			// A turnip does not clean the soil.
			ASSERT_THAT(IsFalse(GetMilestones().HasReached(VestigeGameplayTags::Milestone_Farm_PlantedCleaningCrop)));
		}

		TEST_METHOD(PlantingACropThatCleansReachesItsMilestone)
		{
			Grid->GetField()->Till(TargetCoordinates);
			Contaminate(TargetCoordinates, 1);
			HoldSeeds(CleanupSeedsName, 1);

			UseToolAndFinish();

			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Planted));
			ASSERT_THAT(IsTrue(GetMilestones().HasReached(VestigeGameplayTags::Milestone_Farm_PlantedCleaningCrop)));
		}

		TEST_METHOD(APlantingThatIsRefusedReachesNothing)
		{
			// Seeds on soil that is not tilled are refused.
			Hold(TEXT("Base.Seeds"), 1);

			UseToolAndFinish();

			ASSERT_THAT(IsFalse(GetMilestones().HasReached(VestigeGameplayTags::Milestone_Farm_Planted)));
		}
	};
}

#endif
