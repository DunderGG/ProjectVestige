// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/ActorTestSpawner.h"
#include "Content/FarmCropData.h"
#include "Content/VestigeItemData.h"
#include "CQTest.h"
#include "Engine/AssetManager.h"
#include "Farming/FarmFarmingSaveData.h"
#include "Farming/FarmGrid.h"
#include "Farming/FarmingComponent.h"
#include "Farming/FarmTile.h"
#include "Inventory/VestigeInventoryComponent.h"
#include "Inventory/VestigeInventorySaveData.h"
#include "Player/VestigePlayerCharacter.h"
#include "Player/VestigePlayerState.h"

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

		/**
		 * A default grid and the farmer Blueprint the game uses, standing in front of tile (3, 0), with a player state
		 * whose inventory knows the base items. The Blueprint is used because it holds the real toolbar and seed item,
		 * which load from the base content at BeginPlay.
		 */
		BEFORE_EACH()
		{
			Grid = &Spawner.SpawnActor<AFarmGrid>();
			Grid->DispatchBeginPlay();

			AVestigePlayerState& PlayerState = Spawner.SpawnActor<AVestigePlayerState>();
			PlayerState.DispatchBeginPlay();
			Inventory = PlayerState.GetInventory();
			ASSERT_THAT(IsNotNull(Inventory));

			UClass* FarmerClass = LoadClass<AVestigePlayerCharacter>(nullptr, TEXT("/Game/Vestige/Blueprints/BP_Farmer.BP_Farmer_C"));
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

		int32 GetSeeds() const { return Inventory->GetItemCount(FarmingComponent->GetSeedItemId()); }

		AFarmTile* TargetTile() const { return Grid->GetTileAt(FIntPoint(3, 0)); }

		/** Plants a seed in the target tile and grows it until it is ripe. */
		const UFarmCropData* GrowRipeCrop()
		{
			TargetTile()->Till();
			Hold(TEXT("Base.Seeds"), 1);
			FarmingComponent->UseTool();
			const UFarmCropData* Crop = TargetTile()->GetCrop();
			if (Crop)
			{
				for (int32 Day = 0; Day < Crop->GetDaysToMature(); ++Day)
				{
					TargetTile()->Water();
					TargetTile()->AdvanceDay();
				}
			}
			return Crop;
		}

		TEST_METHOD(PlantingUsesASeed)
		{
			TargetTile()->Till();
			Hold(TEXT("Base.Seeds"), 5);

			FarmingComponent->UseTool();

			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Planted));
			ASSERT_THAT(IsTrue(GetSeeds() == 4));
		}

		TEST_METHOD(TheSeedItemPlantsTheCropItsFragmentNames)
		{
			TargetTile()->Till();
			Hold(TEXT("Base.Seeds"), 1);

			FarmingComponent->UseTool();

			ASSERT_THAT(IsNotNull(TargetTile()->GetCrop()));
			ASSERT_THAT(IsTrue(TargetTile()->GetCrop()->GetContentId() == TEXT("Base.Turnip")));
		}

		TEST_METHOD(WithoutSeedsNothingIsPlanted)
		{
			TargetTile()->Till();
			Hold(TEXT("Base.Seeds"), 0);

			FarmingComponent->UseTool();

			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Tilled));
			ASSERT_THAT(IsTrue(GetSeeds() == 0));
		}

		TEST_METHOD(WithoutAPlayerStateNothingIsPlanted)
		{
			TargetTile()->Till();
			Hold(TEXT("Base.Seeds"), 5);
			Farmer->SetPlayerState(nullptr);

			FarmingComponent->UseTool();

			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Tilled));
		}

		TEST_METHOD(TheHoeTillsTheTileInFront)
		{
			Hold(TEXT("Base.Hoe"), 0);

			FarmingComponent->UseTool();

			ASSERT_THAT(IsTrue(TargetTile()->GetState() == EFarmTileState::Tilled));
		}

		TEST_METHOD(HarvestingAddsTheCropsYieldOfItsItem)
		{
			const UFarmCropData* Crop = GrowRipeCrop();
			ASSERT_THAT(IsNotNull(Crop));
			Hold(TEXT("Base.Hand"), 0);

			FarmingComponent->UseTool();

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

			FarmingComponent->UseTool();

			ASSERT_THAT(IsTrue(Inventory->GetItemCount(Crop->GetHarvestItemId()) == 0));
			ASSERT_THAT(IsTrue(TargetTile()->IsRipe()));
		}

		TEST_METHOD(UsingAToolAwayFromTheFieldChangesNothing)
		{
			Hold(TEXT("Base.Seeds"), 3);
			Farmer->SetActorLocation(FVector(-500.f, -500.f, 0.f));

			FarmingComponent->UseTool();

			ASSERT_THAT(IsTrue(GetSeeds() == 3));
		}

		TEST_METHOD(AnUnknownSavedToolPutsTheFirstSlotInHand)
		{
			FFarmFarmingSaveData Data;
			Data.CurrentToolId = TEXT("SomeMod.Scythe");

			ASSERT_THAT(IsFalse(FarmingComponent->RestoreFromSaveData(Data)));
			ASSERT_THAT(IsTrue(FarmingComponent->GetToolbar().GetCurrentSlot() == 0));
		}
	};
}

#endif
