// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/VestigeGameplayTags.h"
#include "AbilitySystem/VestigeStaminaCostEffect.h"
#include "AbilitySystem/VestigeStaminaSet.h"
#include "AbilitySystemComponent.h"
#include "Components/ActorTestSpawner.h"
#include "Content/FarmCropData.h"
#include "Content/VestigeItemData.h"
#include "Content/VestigeToolData.h"
#include "Core/VestigeFeedbackSubsystem.h"
#include "CQTest.h"
#include "Engine/AssetManager.h"
#include "Farming/FarmFarmingSaveData.h"
#include "Farming/FarmField.h"
#include "Farming/FarmGrid.h"
#include "Farming/FarmingComponent.h"
#include "Farming/FarmTile.h"
#include "Farming/VestigeGameplayAbility_UseTool.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Inventory/VestigeInventoryComponent.h"
#include "Inventory/VestigeInventorySaveData.h"
#include "Player/VestigePlayerCharacter.h"
#include "Player/VestigePlayerState.h"
#include "Tests/VestigeTestEventRecorder.h"
#include "UObject/StrongObjectPtr.h"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace FarmingTests
{
	TEST_CLASS(Farming, "Vestige")
	{
		FActorTestSpawner Spawner;
		AFarmGrid* Grid = nullptr;
		AVestigePlayerCharacter* Farmer = nullptr;
		UFarmingComponent* FarmingComponent = nullptr;
		UVestigeInventoryComponent* Inventory = nullptr;
		UAbilitySystemComponent* AbilitySystem = nullptr;

		/**
		 * A default grid and the character Blueprint the game uses, standing in front of tile (3, 0), with the game's
		 * player state Blueprint, whose inventory knows the base items and whose ability set grants the use-tool ability
		 * and stamina. The Blueprints are used because they hold the real toolbar, seed item and ability set, which load
		 * from the base content.
		 */
		BEFORE_EACH()
		{
			// The grid's field and the clock live in the game instance.
			Spawner.InitializeGameSubsystems();
			Grid = &Spawner.SpawnActor<AFarmGrid>();
			Grid->DispatchBeginPlay();

			UClass* PlayerStateClass =
				LoadClass<AVestigePlayerState>(nullptr, TEXT("/Game/Vestige/Blueprints/BP_PlayerState.BP_PlayerState_C"));
			ASSERT_THAT(IsNotNull(PlayerStateClass));
			AVestigePlayerState& PlayerState = Spawner.SpawnActor<AVestigePlayerState>(FActorSpawnParameters(), PlayerStateClass);
			PlayerState.DispatchBeginPlay();
			Inventory = PlayerState.GetInventory();
			ASSERT_THAT(IsNotNull(Inventory));
			AbilitySystem = PlayerState.GetAbilitySystemComponent();
			ASSERT_THAT(IsNotNull(AbilitySystem));

			UClass* FarmerClass =
				LoadClass<AVestigePlayerCharacter>(nullptr, TEXT("/Game/Vestige/Blueprints/BP_PlayerCharacter.BP_PlayerCharacter_C"));
			ASSERT_THAT(IsNotNull(FarmerClass));
			Farmer = &Spawner.SpawnActor<AVestigePlayerCharacter>(FActorSpawnParameters(), FarmerClass);
			Farmer->DispatchBeginPlay();
			Farmer->SetActorLocationAndRotation(FVector(250.f, 50.f, 0.f), FRotator::ZeroRotator);
			Farmer->SetPlayerState(&PlayerState);
			FarmingComponent = Farmer->FindComponentByClass<UFarmingComponent>();
			ASSERT_THAT(IsNotNull(FarmingComponent));
		}

		/** Puts this tool in hand, through the same restore loading uses, and makes the bag hold only this many seeds. */
		void Hold(FName ToolId, int32 Seeds)
		{
			FFarmFarmingSaveData Data;
			Data.CurrentToolId = ToolId;
			ASSERT_THAT(IsTrue(FarmingComponent->RestoreFromSaveData(Data)));

			FVestigeInventorySaveData Bag;
			Bag.Slots.Add({0, FarmingComponent->GetSeedItemId().PrimaryAssetName, Seeds});
			ASSERT_THAT(IsTrue(Inventory->RestoreFromSaveData(Bag) == 0));
		}

		/**
		 * Uses the tool in hand, then ends the use at once, as its timer would once its time is up: these tests don't let
		 * time pass, and the next use can't start before the last one ends.
		 */
		void UseToolAndFinish()
		{
			FarmingComponent->UseTool();
			AbilitySystem->CancelAllAbilities();
		}

		int32 GetSeeds() const { return Inventory->GetItemCount(FarmingComponent->GetSeedItemId()); }

		float GetStamina() const { return AbilitySystem->GetNumericAttribute(UVestigeStaminaSet::GetStaminaAttribute()); }
		float GetMaxStamina() const { return AbilitySystem->GetNumericAttribute(UVestigeStaminaSet::GetMaxStaminaAttribute()); }

		/** The tool asset in hand, after Hold. */
		const UVestigeToolData& GetHeldTool() const { return *FarmingComponent->GetCurrentTool(); }

		/** Takes all of the player's stamina, through the cost effect a tool use spends it with. */
		void EmptyStamina()
		{
			const FGameplayEffectSpecHandle Spec =
				AbilitySystem->MakeOutgoingSpec(UVestigeStaminaCostEffect::StaticClass(), 1.f, AbilitySystem->MakeEffectContext());
			Spec.Data->SetSetByCallerMagnitude(VestigeGameplayTags::Data_StaminaCost, -GetMaxStamina());
			AbilitySystem->ApplyGameplayEffectSpecToSelf(*Spec.Data);
		}

		/** Whether a tool use is under way, so the next one can't start yet. */
		bool IsUsingTool() const
		{
			const FGameplayAbilitySpec* Spec = AbilitySystem->FindAbilitySpecFromClass(UVestigeGameplayAbility_UseTool::StaticClass());
			return Spec && Spec->IsActive();
		}

		AFarmTile* TargetTile() const { return Grid->GetTileAt(FIntPoint(3, 0)); }

		/** Plants a seed in the target tile and grows it until it is ripe. */
		const UFarmCropData* GrowRipeCrop()
		{
			TargetTile()->Till();
			Hold(TEXT("Base.Seeds"), 1);
			UseToolAndFinish();
			const UFarmCropData* Crop = TargetTile()->GetCrop();
			if (Crop)
			{
				for (int32 Day = 0; Day < Crop->GetDaysToMature(); ++Day)
				{
					TargetTile()->Water();
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

		TEST_METHOD(PlantingUsesASeed)
		{
			TargetTile()->Till();
			Hold(TEXT("Base.Seeds"), 5);

			UseToolAndFinish();

			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Planted));
			ASSERT_THAT(IsTrue(GetSeeds() == 4));
		}

		TEST_METHOD(TheSeedItemPlantsTheCropItsFragmentNames)
		{
			TargetTile()->Till();
			Hold(TEXT("Base.Seeds"), 1);

			UseToolAndFinish();

			ASSERT_THAT(IsNotNull(TargetTile()->GetCrop()));
			ASSERT_THAT(IsTrue(TargetTile()->GetCrop()->GetContentId() == TEXT("Base.Turnip")));
		}

		TEST_METHOD(WithoutSeedsNothingIsPlanted)
		{
			TargetTile()->Till();
			Hold(TEXT("Base.Seeds"), 0);

			UseToolAndFinish();

			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Tilled));
			ASSERT_THAT(IsTrue(GetSeeds() == 0));
		}

		TEST_METHOD(WithoutAPlayerStateNothingIsPlanted)
		{
			TargetTile()->Till();
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

			// Every slot holds a full stack of seeds, so the harvest has nowhere to go.
			const UVestigeItemData* Seed = UAssetManager::Get().GetPrimaryAssetObject<UVestigeItemData>(FarmingComponent->GetSeedItemId());
			ASSERT_THAT(IsNotNull(Seed));
			FVestigeInventorySaveData FullBag;
			for (int32 Slot = 0; Slot < Inventory->GetSlots().Num(); ++Slot)
			{
				FullBag.Slots.Add({Slot, Seed->GetContentId(), Seed->GetMaxStackSize()});
			}
			ASSERT_THAT(IsTrue(Inventory->RestoreFromSaveData(FullBag) == 0));

			UseToolAndFinish();

			ASSERT_THAT(IsTrue(Inventory->GetItemCount(Crop->GetHarvestItemId()) == 0));
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
			Farmer->SetActorLocation(FVector(350.f, 50.f, 0.f));
			AFarmTile* NextTile = Grid->GetTileAt(FIntPoint(4, 0));
			ASSERT_THAT(IsNotNull(NextTile));
			FarmingComponent->UseTool();
			ASSERT_THAT(IsTrue(NextTile->GetState() == EFarmTileState::Grass));

			AbilitySystem->CancelAllAbilities();
			UseToolAndFinish();
			ASSERT_THAT(IsTrue(NextTile->GetState() == EFarmTileState::Tilled));
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

			const TArray<FString> Expected = {FString::Printf(TEXT("ToolUsed Base.Hoe %s"), *GetNameSafe(TargetTile()))};
			ASSERT_THAT(IsTrue(Recorder->GetEvents() == Expected));
		}

		TEST_METHOD(HarvestingTellsBlueprintsTheCropThenTheTool)
		{
			const UFarmCropData* Crop = GrowRipeCrop();
			ASSERT_THAT(IsNotNull(Crop));
			Hold(TEXT("Base.Hand"), 0);
			const TStrongObjectPtr<UVestigeTestEventRecorder> Recorder = ListenToBlueprintEvents();

			UseToolAndFinish();

			const FString Tile = GetNameSafe(TargetTile());
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

		TEST_METHOD(AnUnknownSavedToolPutsTheFirstSlotInHand)
		{
			FFarmFarmingSaveData Data;
			Data.CurrentToolId = TEXT("SomeMod.Scythe");

			ASSERT_THAT(IsFalse(FarmingComponent->RestoreFromSaveData(Data)));
			ASSERT_THAT(IsTrue(FarmingComponent->GetToolbar().GetCurrentSlot() == 0));
		}

		/** Collects every feedback message from now on into Messages. */
		void ListenToFeedback(TArray<FString>& Messages)
		{
			UVestigeFeedbackSubsystem* Feedback = Spawner.GetWorld().GetSubsystem<UVestigeFeedbackSubsystem>();
			ASSERT_THAT(IsNotNull(Feedback));
			Feedback->OnFeedback.AddLambda(
				[&Messages](const FText& Message)
				{
					Messages.Add(Message.ToString());
				});
		}

		/** Moves the farmer one tile on along the row, so the target becomes tile (4, 0). */
		AFarmTile* StepToNextTile()
		{
			Farmer->SetActorLocation(FVector(350.f, 50.f, 0.f));
			return Grid->GetTileAt(FIntPoint(4, 0));
		}

		TEST_METHOD(HoldingTheUseKeyWorksTheNextTileOnceTheUseHasFinished)
		{
			Hold(TEXT("Base.Hoe"), 0);
			FarmingComponent->StartUsingTool();
			AFarmTile* NextTile = StepToNextTile();
			ASSERT_THAT(IsNotNull(NextTile));
			ASSERT_THAT(IsTrue(NextTile->GetState() == EFarmTileState::Grass));

			AbilitySystem->CancelAllAbilities();

			ASSERT_THAT(IsTrue(NextTile->GetState() == EFarmTileState::Tilled));
			AbilitySystem->CancelAllAbilities();
		}

		TEST_METHOD(HoldingTheUseKeyWorksTheNextTileWhenTheTargetChanges)
		{
			Hold(TEXT("Base.Hoe"), 0);
			FarmingComponent->StartUsingTool();
			AbilitySystem->CancelAllAbilities();

			AFarmTile* NextTile = StepToNextTile();

			ASSERT_THAT(IsNotNull(NextTile));
			ASSERT_THAT(IsTrue(NextTile->GetState() == EFarmTileState::Tilled));
			AbilitySystem->CancelAllAbilities();
		}

		TEST_METHOD(ARepeatThatWouldChangeNothingIsSkippedQuietly)
		{
			Hold(TEXT("Base.Hoe"), 0);
			TArray<FString> Messages;
			ListenToFeedback(Messages);
			FarmingComponent->StartUsingTool();

			// The use ends on the tile it just tilled, which the hoe can't change again.
			AbilitySystem->CancelAllAbilities();

			ASSERT_THAT(IsTrue(Messages.Num() == 1));
			ASSERT_THAT(IsFalse(IsUsingTool()));
			ASSERT_THAT(IsTrue(GetStamina() == GetMaxStamina() - GetHeldTool().GetStaminaCost()));
		}

		TEST_METHOD(ThePressSaysWhyAUseChangesNothingAndTheRepeatWaitsForTheNextTile)
		{
			Hold(TEXT("Base.Hoe"), 0);
			TargetTile()->Till();
			TArray<FString> Messages;
			ListenToFeedback(Messages);

			FarmingComponent->StartUsingTool();
			ASSERT_THAT(IsTrue(Messages.Num() == 1));

			AFarmTile* NextTile = StepToNextTile();
			ASSERT_THAT(IsNotNull(NextTile));
			ASSERT_THAT(IsTrue(NextTile->GetState() == EFarmTileState::Tilled));
			AbilitySystem->CancelAllAbilities();
		}

		TEST_METHOD(NoToolWorksInTheAir)
		{
			Hold(TEXT("Base.Hoe"), 0);
			Farmer->GetCharacterMovement()->SetMovementMode(MOVE_Falling);

			UseToolAndFinish();

			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Grass));
			ASSERT_THAT(IsTrue(GetStamina() == GetMaxStamina()));
		}

		TEST_METHOD(LandingWithTheUseKeyHeldWorksTheTarget)
		{
			Hold(TEXT("Base.Hoe"), 0);
			Farmer->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
			FarmingComponent->StartUsingTool();
			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Grass));

			Farmer->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
			Farmer->Landed(FHitResult());

			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Tilled));
			AbilitySystem->CancelAllAbilities();
		}

		TEST_METHOD(ReleasingTheUseKeyStopsTheRepeat)
		{
			Hold(TEXT("Base.Hoe"), 0);
			FarmingComponent->StartUsingTool();
			FarmingComponent->StopUsingTool();
			AbilitySystem->CancelAllAbilities();

			AFarmTile* NextTile = StepToNextTile();

			ASSERT_THAT(IsNotNull(NextTile));
			ASSERT_THAT(IsTrue(NextTile->GetState() == EFarmTileState::Grass));
			ASSERT_THAT(IsFalse(FarmingComponent->IsToolUseHeld()));
		}
	};
}

#endif
