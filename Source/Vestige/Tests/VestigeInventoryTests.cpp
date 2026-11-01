// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/ActorTestSpawner.h"
#include "Content/VestigeItemData.h"
#include "CQTest.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerState.h"
#include "Inventory/VestigeInventory.h"
#include "Inventory/VestigeInventoryComponent.h"
#include "Inventory/VestigeInventorySaveData.h"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace VestigeInventoryTests
{
	TEST_CLASS(Inventory, "Vestige")
	{
		FActorTestSpawner Spawner;

		/** An item asset with this ContentId and stack size. */
		const UVestigeItemData& MakeItem(FName ContentId, int32 StackSize)
		{
			UVestigeItemData& Item = Spawner.SpawnObject<UVestigeItemData>();
			Item.SetContentIdForTests(ContentId);
			Item.SetMaxStackSizeForTests(StackSize);
			return Item;
		}

		/** An inventory component with SlotCount slots that knows these items, without loading any. */
		UVestigeInventoryComponent& MakeComponent(int32 SlotCount, TConstArrayView<const UVestigeItemData*> Items)
		{
			AActor& Owner = Spawner.SpawnActor<AActor>();
			UVestigeInventoryComponent* Component = NewObject<UVestigeInventoryComponent>(&Owner);
			Component->ResetForTests(SlotCount, Items);
			return *Component;
		}

		static FVestigeInventorySaveData MakeSave(TArray<FVestigeInventorySlotSaveData> Slots)
		{
			FVestigeInventorySaveData Data;
			Data.Slots = MoveTemp(Slots);
			return Data;
		}

		TEST_METHOD(AddingTopsUpStacksBeforeFillingEmptySlots)
		{
			const UVestigeItemData& Seeds = MakeItem(TEXT("Base.Seeds"), 10);
			const UVestigeItemData& Stone = MakeItem(TEXT("Base.Stone"), 10);
			FVestigeInventory Bag;
			Bag.Reset(3);
			ASSERT_THAT(IsTrue(Bag.TryAdd(Seeds, 4)));
			ASSERT_THAT(IsTrue(Bag.TryAdd(Stone, 1)));

			ASSERT_THAT(IsTrue(Bag.TryAdd(Seeds, 9)));

			ASSERT_THAT(IsTrue(Bag.GetSlots()[0].Count == 10));
			ASSERT_THAT(IsTrue(Bag.GetSlots()[1].Item == &Stone));
			ASSERT_THAT(IsTrue(Bag.GetSlots()[2].Item == &Seeds));
			ASSERT_THAT(IsTrue(Bag.GetSlots()[2].Count == 3));
			ASSERT_THAT(IsTrue(Bag.GetCount(Seeds) == 13));
		}

		TEST_METHOD(ItemsThatDontStackTakeASlotEach)
		{
			const UVestigeItemData& Can = MakeItem(TEXT("Base.Can"), 1);
			FVestigeInventory Bag;
			Bag.Reset(3);

			ASSERT_THAT(IsTrue(Bag.TryAdd(Can, 2)));

			ASSERT_THAT(IsTrue(Bag.GetSlots()[0].Count == 1));
			ASSERT_THAT(IsTrue(Bag.GetSlots()[1].Count == 1));
			ASSERT_THAT(IsTrue(Bag.GetSlots()[2].IsEmpty()));
		}

		TEST_METHOD(AnAddThatDoesntFitChangesNothing)
		{
			const UVestigeItemData& Seeds = MakeItem(TEXT("Base.Seeds"), 10);
			FVestigeInventory Bag;
			Bag.Reset(2);
			ASSERT_THAT(IsTrue(Bag.TryAdd(Seeds, 15)));

			ASSERT_THAT(IsFalse(Bag.CanAdd(Seeds, 6)));
			ASSERT_THAT(IsFalse(Bag.TryAdd(Seeds, 6)));
			ASSERT_THAT(IsTrue(Bag.GetCount(Seeds) == 15));
			ASSERT_THAT(IsTrue(Bag.GetRoomFor(Seeds) == 5));
		}

		TEST_METHOD(RemovingTakesFromTheLastStacksFirst)
		{
			const UVestigeItemData& Seeds = MakeItem(TEXT("Base.Seeds"), 10);
			FVestigeInventory Bag;
			Bag.Reset(3);
			ASSERT_THAT(IsTrue(Bag.TryAdd(Seeds, 14)));

			ASSERT_THAT(IsTrue(Bag.TryRemove(Seeds, 6)));

			ASSERT_THAT(IsTrue(Bag.GetSlots()[0].Count == 8));
			ASSERT_THAT(IsTrue(Bag.GetSlots()[1].IsEmpty()));
			ASSERT_THAT(IsTrue(Bag.GetCount(Seeds) == 8));
		}

		TEST_METHOD(RemovingMoreThanThereIsChangesNothing)
		{
			const UVestigeItemData& Seeds = MakeItem(TEXT("Base.Seeds"), 10);
			FVestigeInventory Bag;
			Bag.Reset(2);
			ASSERT_THAT(IsTrue(Bag.TryAdd(Seeds, 3)));

			ASSERT_THAT(IsFalse(Bag.TryRemove(Seeds, 4)));
			ASSERT_THAT(IsTrue(Bag.GetCount(Seeds) == 3));
		}

		TEST_METHOD(CountsBelowOneAreRefused)
		{
			const UVestigeItemData& Seeds = MakeItem(TEXT("Base.Seeds"), 10);
			FVestigeInventory Bag;
			Bag.Reset(2);

			ASSERT_THAT(IsFalse(Bag.TryAdd(Seeds, 0)));
			ASSERT_THAT(IsFalse(Bag.TryAdd(Seeds, -3)));
			ASSERT_THAT(IsFalse(Bag.TryRemove(Seeds, 0)));
			ASSERT_THAT(IsTrue(Bag.GetSlots()[0].IsEmpty()));
		}

		TEST_METHOD(AnUnknownItemIdIsRefused)
		{
			UVestigeInventoryComponent& Component = MakeComponent(4, {});
			const FPrimaryAssetId Unknown(UVestigeItemData::AssetType, TEXT("SomeMod.Pumpkin"));

			ASSERT_THAT(IsFalse(Component.CanAddItem(Unknown, 1)));
			ASSERT_THAT(IsFalse(Component.TryAddItem(Unknown, 1)));
			ASSERT_THAT(IsTrue(Component.GetItemCount(Unknown) == 0));
		}

		TEST_METHOD(EveryChangeIsBroadcast)
		{
			const UVestigeItemData& Seeds = MakeItem(TEXT("Base.Seeds"), 10);
			UVestigeInventoryComponent& Component = MakeComponent(4, {&Seeds});
			int32 Broadcasts = 0;
			Component.OnInventoryChanged.AddLambda(
				[&Broadcasts]()
				{
					++Broadcasts;
				});

			ASSERT_THAT(IsTrue(Component.TryAddItem(Seeds.GetPrimaryAssetId(), 3)));
			ASSERT_THAT(IsTrue(Component.TryRemoveItem(Seeds.GetPrimaryAssetId(), 1)));
			ASSERT_THAT(IsFalse(Component.TryRemoveItem(Seeds.GetPrimaryAssetId(), 9)));
			Component.RestoreFromSaveData(FVestigeInventorySaveData());

			ASSERT_THAT(IsTrue(Broadcasts == 3));
		}

		TEST_METHOD(StacksRoundTripInTheirSlots)
		{
			const UVestigeItemData& Seeds = MakeItem(TEXT("Base.Seeds"), 10);
			const UVestigeItemData& Stone = MakeItem(TEXT("Base.Stone"), 10);
			UVestigeInventoryComponent& Saved = MakeComponent(4, {&Seeds, &Stone});
			Saved.RestoreFromSaveData(MakeSave({{3, TEXT("Base.Seeds"), 7}, {1, TEXT("Base.Stone"), 2}}));
			UVestigeInventoryComponent& Restored = MakeComponent(4, {&Seeds, &Stone});

			const int32 Skipped = Restored.RestoreFromSaveData(Saved.GetSaveData());

			ASSERT_THAT(IsTrue(Skipped == 0));
			ASSERT_THAT(IsTrue(Restored.GetSlots()[0].IsEmpty()));
			ASSERT_THAT(IsTrue(Restored.GetSlots()[1].Item == &Stone));
			ASSERT_THAT(IsTrue(Restored.GetSlots()[1].Count == 2));
			ASSERT_THAT(IsTrue(Restored.GetSlots()[3].Item == &Seeds));
			ASSERT_THAT(IsTrue(Restored.GetSlots()[3].Count == 7));
		}

		TEST_METHOD(UnknownSavedItemsAreSkipped)
		{
			const UVestigeItemData& Seeds = MakeItem(TEXT("Base.Seeds"), 10);
			UVestigeInventoryComponent& Component = MakeComponent(4, {&Seeds});

			const int32 Skipped = Component.RestoreFromSaveData(MakeSave({{0, TEXT("SomeMod.Pumpkin"), 3}, {1, TEXT("Base.Seeds"), 2}}));

			ASSERT_THAT(IsTrue(Skipped == 1));
			ASSERT_THAT(IsTrue(Component.GetSlots()[0].IsEmpty()));
			ASSERT_THAT(IsTrue(Component.GetItemCount(Seeds.GetPrimaryAssetId()) == 2));
		}

		TEST_METHOD(AStackAboveTheStackSizeSpillsIntoFreeSlots)
		{
			const UVestigeItemData& Seeds = MakeItem(TEXT("Base.Seeds"), 10);
			const UVestigeItemData& Stone = MakeItem(TEXT("Base.Stone"), 10);
			UVestigeInventoryComponent& Component = MakeComponent(3, {&Seeds, &Stone});

			// Slot 1 is saved with a stone, so the spilled seeds must skip it.
			const int32 Skipped = Component.RestoreFromSaveData(MakeSave({{0, TEXT("Base.Seeds"), 15}, {1, TEXT("Base.Stone"), 1}}));

			ASSERT_THAT(IsTrue(Skipped == 0));
			ASSERT_THAT(IsTrue(Component.GetSlots()[0].Count == 10));
			ASSERT_THAT(IsTrue(Component.GetSlots()[1].Item == &Stone));
			ASSERT_THAT(IsTrue(Component.GetSlots()[2].Item == &Seeds));
			ASSERT_THAT(IsTrue(Component.GetSlots()[2].Count == 5));
		}

		TEST_METHOD(ABadSavedSlotGoesInAsANormalAdd)
		{
			const UVestigeItemData& Seeds = MakeItem(TEXT("Base.Seeds"), 10);
			UVestigeInventoryComponent& Component = MakeComponent(2, {&Seeds});

			const int32 Skipped = Component.RestoreFromSaveData(MakeSave({{7, TEXT("Base.Seeds"), 4}}));

			ASSERT_THAT(IsTrue(Skipped == 0));
			ASSERT_THAT(IsTrue(Component.GetSlots()[0].Count == 4));
		}

		TEST_METHOD(SavedItemsWithNoRoomLeftAreSkipped)
		{
			const UVestigeItemData& Seeds = MakeItem(TEXT("Base.Seeds"), 10);
			UVestigeInventoryComponent& Component = MakeComponent(1, {&Seeds});

			const int32 Skipped = Component.RestoreFromSaveData(MakeSave({{0, TEXT("Base.Seeds"), 12}}));

			ASSERT_THAT(IsTrue(Skipped == 1));
			ASSERT_THAT(IsTrue(Component.GetItemCount(Seeds.GetPrimaryAssetId()) == 10));
		}

		/** Catches a missing editor step: the game mode must name a player state whose inventory starts with seeds. */
		TEST_METHOD(ThePlayerStartsWithTurnipSeeds)
		{
			UClass* GameModeClass =
				LoadClass<AGameModeBase>(nullptr, TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonGameMode.BP_ThirdPersonGameMode_C"));
			ASSERT_THAT(IsNotNull(GameModeClass));
			const TSubclassOf<APlayerState> PlayerStateClass = GameModeClass->GetDefaultObject<AGameModeBase>()->PlayerStateClass;
			ASSERT_THAT(IsNotNull(PlayerStateClass.Get()));

			APlayerState& PlayerState = Spawner.SpawnActor<APlayerState>(FActorSpawnParameters(), PlayerStateClass);
			PlayerState.DispatchBeginPlay();
			const UVestigeInventoryComponent* Component = PlayerState.FindComponentByClass<UVestigeInventoryComponent>();

			ASSERT_THAT(IsNotNull(Component));
			ASSERT_THAT(IsTrue(Component->GetItemCount(FPrimaryAssetId(UVestigeItemData::AssetType, TEXT("Base.TurnipSeeds"))) > 0));
		}
	};
}

#endif
