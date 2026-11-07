// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/VestigeGameplayTags.h"
#include "AbilitySystem/VestigeStaminaSet.h"
#include "AbilitySystemComponent.h"
#include "Components/ActorTestSpawner.h"
#include "Content/FarmCropData.h"
#include "Content/FarmSeedFragment.h"
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
#include "Farming/FarmTileState.h"
#include "Farming/VestigeGameplayAbility_UseTool.h"
#include "GameFramework/CharacterMovementComponent.h"
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

		/** The feedback messages since ListenToFeedback, kept on the fixture so latent steps can read them. */
		TArray<FString> Messages;

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

		/** The tool abilities granted on the player's ability system. Read at once: specs move when abilities are removed. */
		TArray<const FGameplayAbilitySpec*> GetToolAbilities() const
		{
			TArray<const FGameplayAbilitySpec*> Specs;
			for (const FGameplayAbilitySpec& Spec : AbilitySystem->GetActivatableAbilities())
			{
				if (Spec.Ability && Spec.Ability->GetAssetTags().HasTagExact(VestigeGameplayTags::Ability_UseTool))
				{
					Specs.Add(&Spec);
				}
			}
			return Specs;
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

		/** Collects every feedback message from now on into Messages. */
		void ListenToFeedback()
		{
			UVestigeFeedbackSubsystem* Feedback = Spawner.GetWorld().GetSubsystem<UVestigeFeedbackSubsystem>();
			ASSERT_THAT(IsNotNull(Feedback));
			Feedback->OnFeedback.AddLambda(
				[this](const FText& Message)
				{
					Messages.Add(Message.ToString());
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

			UseToolAndFinish();

			ASSERT_THAT(IsTrue(Inventory->GetItemCount(Crop->GetHarvestItemId()) == Crop->GetYield()));
			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Tilled));
		}

		TEST_METHOD(AFullBagLeavesTheCropInTheGround)
		{
			const UFarmCropData* Crop = GrowRipeCrop();
			ASSERT_THAT(IsNotNull(Crop));
			Hold(TEXT("Base.Hand"), 0);

			// Every slot holds a full stack, so the harvest has nowhere to go. The slot in hand holds turnips, which aren't a
			// tool, so the hand harvests.
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

			UseToolAndFinish();

			ASSERT_THAT(IsTrue(Inventory->GetItemCount(Crop->GetHarvestItemId()) == TurnipsBefore));
			ASSERT_THAT(IsTrue(TargetTile()->IsRipe()));
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

			UseToolAndFinish();

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

		TEST_METHOD(AnEmptyHandOrAnItemWithoutAToolHarvests)
		{
			Hold(TEXT("Base.Hand"), 0);
			ASSERT_THAT(IsTrue(FarmingComponent->GetCurrentTool() == GetBaseTool(TEXT("Base.Hand"))));

			const UFarmCropData* Crop = GrowRipeCrop();
			ASSERT_THAT(IsNotNull(Crop));
			SetBag({{0, TEXT("Base.Turnip"), 1}});
			ASSERT_THAT(IsTrue(FarmingComponent->GetCurrentTool() == GetBaseTool(TEXT("Base.Hand"))));

			UseToolAndFinish();

			ASSERT_THAT(IsTrue(Inventory->GetItemCount(Crop->GetHarvestItemId()) == 1 + Crop->GetYield()));
			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Tilled));
		}

		TEST_METHOD(PlantingUsesTheSeedInHand)
		{
			// A second kind of seed, growing a crop of its own, beside the turnip seeds.
			UFarmCropData* Beet = NewObject<UFarmCropData>();
			Beet->SetContentIdForTests(TEXT("Test.Beet"));
			FarmingComponent->AddCropForTests(*Beet);
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
			Inventory->ResetForTests(Inventory->GetSlots().Num(), {TurnipSeeds, BeetSeeds});
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
					const TArray<const FGameplayAbilitySpec*> Specs = GetToolAbilities();
					ASSERT_THAT(IsTrue(Specs.Num() == 1));
					ASSERT_THAT(IsTrue(Specs[0]->SourceObject.Get() == GetBaseTool(TEXT("Base.Hand"))));
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
					const TArray<const FGameplayAbilitySpec*> Specs = GetToolAbilities();
					ASSERT_THAT(IsTrue(Specs.Num() == 1));
					ASSERT_THAT(IsTrue(Specs[0]->SourceObject.Get() == GetBaseTool(TEXT("Base.Hand"))));
					FarmingComponent->StopUsingTool();
				});
		}

		TEST_METHOD(ASecondHandChangeDuringTheFirstUseLeavesOneToolAbility)
		{
			// The hand's grant is queued while the first use holds the ability list's lock; selecting the hoe then takes the
			// queued grant back.
			Grid->GetField()->Till(TargetCoordinates);
			SetBag({{0, TurnipSeedsId.PrimaryAssetName, 1}, {1, TEXT("Base.Hoe"), 1}});
			const UVestigeToolData* Hand = GetBaseTool(TEXT("Base.Hand"));
			const FDelegateHandle Switcher = FarmingComponent->OnToolChanged.AddLambda(
				[this, Hand]()
				{
					if (FarmingComponent->GetCurrentTool() == Hand)
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
			// The hand, made to do something no base tool does, through a new ability alone.
			UVestigeToolData* Hand = const_cast<UVestigeToolData*>(Cast<UVestigeToolData>(
				UAssetManager::Get().GetPrimaryAssetObject(FPrimaryAssetId(UVestigeToolData::AssetType, TEXT("Base.Hand")))));
			ASSERT_THAT(IsNotNull(Hand));
			const TSubclassOf<UGameplayAbility> HandAbility = Hand->GetAbility();
			ON_SCOPE_EXIT
			{
				Hand->SetAbilityForTests(HandAbility);
			};
			Hand->SetAbilityForTests(UVestigeTestToolAbility::StaticClass());
			Hold(TEXT("Base.Hand"), 0);

			UseToolAndFinish();

			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Tilled));
			ASSERT_THAT(IsTrue(TargetTile()->IsWatered()));
			ASSERT_THAT(IsTrue(GetStamina() == GetMaxStamina() - Hand->GetStaminaCost()));
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
					ASSERT_THAT(IsTrue(Messages.Num() == 1));
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
			ASSERT_THAT(IsTrue(Messages.Num() == 1));

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
	};
}

#endif
