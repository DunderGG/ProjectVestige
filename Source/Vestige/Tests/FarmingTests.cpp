// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/VestigeGameplayTags.h"
#include "AbilitySystem/VestigeStaminaSet.h"
#include "AbilitySystemComponent.h"
#include "Components/ActorTestSpawner.h"
#include "Content/FarmCropData.h"
#include "Content/FarmSeedFragment.h"
#include "Content/VestigeContentCatalog.h"
#include "Content/VestigeGameData.h"
#include "Content/VestigeItemData.h"
#include "Content/VestigeToolData.h"
#include "Content/VestigeToolFragment.h"
#include "Core/VestigeFeedbackSubsystem.h"
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

		static inline const FPrimaryAssetId TurnipSeedsId = FPrimaryAssetId(UVestigeItemData::AssetType, TEXT("Base.TurnipSeeds"));

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
					Slots.Add({0, TurnipSeedsId.PrimaryAssetName, Seeds});
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
					Slots.Add({1, TurnipSeedsId.PrimaryAssetName, Seeds});
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
		int32 GetSeeds() const { return Inventory->GetItemCount(TurnipSeedsId); }

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
				Field->AdvanceDay();
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
					Grid->GetField()->AdvanceDay();
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
				});
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
				Grid->GetField()->AdvanceDay();
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
			const UVestigeItemData* Seed = GetBaseItem(TurnipSeedsId.PrimaryAssetName);
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
				Grid->GetField()->AdvanceDay();
			}
			ASSERT_THAT(IsTrue(TargetTile()->IsRipe()));
			Hold(TEXT("Base.Hand"), 0);
			HarvestAndFinish();
			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Tilled));

			ASSERT_THAT(IsTrue(Reports.IsEmpty()));
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
			SetBag({{0, TEXT("Base.Hoe"), 1}, {1, TEXT("Base.WateringCan"), 1}, {2, TurnipSeedsId.PrimaryAssetName, 5},
				{3, TurnipSeedsId.PrimaryAssetName, 5}});
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
			const UVestigeItemData* TurnipSeeds = GetBaseItem(TurnipSeedsId.PrimaryAssetName);
			ASSERT_THAT(IsNotNull(TurnipSeeds));
			ASSERT_THAT(IsTrue(Catalog().AddForTests(*BeetSeeds)));
			SetBag({{0, TurnipSeedsId.PrimaryAssetName, 3}, {1, BeetSeeds->GetContentId(), 3}, {5, TurnipSeedsId.PrimaryAssetName, 3}}, 1);
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
			SetBag({{0, TurnipSeedsId.PrimaryAssetName, 1}, {1, TEXT("Base.Hoe"), 1}, {2, TEXT("Base.WateringCan"), 1}});
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
	};
}

#endif
