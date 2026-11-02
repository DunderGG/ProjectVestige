// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Inventory/VestigeInventoryComponent.h"

#include "Content/VestigeItemData.h"
#include "Engine/AssetManager.h"
#include "Inventory/VestigeInventoryLog.h"
#include "Inventory/VestigeInventorySaveData.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

UVestigeInventoryComponent::UVestigeInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UVestigeInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	Inventory.Reset(SlotCount);
	LoadItems();
	AddStartingItems();
	OnInventoryChanged.Broadcast();
}

void UVestigeInventoryComponent::LoadItems()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UVestigeInventoryComponent::LoadItems);

	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	if (!AssetManager)
	{
		return;
	}

	TArray<FPrimaryAssetId> ItemIds;
	AssetManager->GetPrimaryAssetIdList(UVestigeItemData::AssetType, ItemIds);

	// Definitions have no meshes or textures, so all of them load quickly, and waiting is fine at play start.
	if (const TSharedPtr<FStreamableHandle> Handle = AssetManager->LoadPrimaryAssets(ItemIds))
	{
		Handle->WaitUntilComplete();
	}
	for (const FPrimaryAssetId& ItemId : ItemIds)
	{
		if (const UVestigeItemData* Item = AssetManager->GetPrimaryAssetObject<UVestigeItemData>(ItemId))
		{
			Items.Add(ItemId, Item);
		}
	}
	VESTIGE_LOG(this, LogVestigeInventory, Log, "Loaded {Count} item definitions.", Items.Num());
}

void UVestigeInventoryComponent::AddStartingItems()
{
	for (const FVestigeItemStack& Stack : StartingItems)
	{
		const UVestigeItemData* Item = FindItem(Stack.ItemId);
		if (Item && !Inventory.TryAdd(*Item, Stack.Count))
		{
			VESTIGE_LOG(
				this, LogVestigeInventory, Warning, "No room for the {Count} starting {ItemId}.", Stack.Count, Stack.ItemId.ToString());
		}
	}
}

const UVestigeItemData* UVestigeInventoryComponent::FindItem(const FPrimaryAssetId& ItemId) const
{
	const TObjectPtr<const UVestigeItemData>* Item = Items.Find(ItemId);
	if (!Item)
	{
		VESTIGE_LOG(this, LogVestigeInventory, Warning, "No item has the ID {ItemId}.", ItemId.ToString());
		return nullptr;
	}
	return *Item;
}

bool UVestigeInventoryComponent::CanAddItem(const FPrimaryAssetId& ItemId, int32 Count) const
{
	const UVestigeItemData* Item = FindItem(ItemId);
	return Item && Inventory.CanAdd(*Item, Count);
}

bool UVestigeInventoryComponent::TryAddItem(const FPrimaryAssetId& ItemId, int32 Count)
{
	const UVestigeItemData* Item = FindItem(ItemId);
	if (!Item || !Inventory.TryAdd(*Item, Count))
	{
		return false;
	}
	OnInventoryChanged.Broadcast();
	return true;
}

bool UVestigeInventoryComponent::TryRemoveItem(const FPrimaryAssetId& ItemId, int32 Count)
{
	const UVestigeItemData* Item = FindItem(ItemId);
	if (!Item || !Inventory.TryRemove(*Item, Count))
	{
		return false;
	}
	OnInventoryChanged.Broadcast();
	return true;
}

int32 UVestigeInventoryComponent::GetItemCount(const FPrimaryAssetId& ItemId) const
{
	const UVestigeItemData* Item = FindItem(ItemId);
	return Item ? Inventory.GetCount(*Item) : 0;
}

FVestigeInventorySaveData UVestigeInventoryComponent::GetSaveData() const
{
	FVestigeInventorySaveData Data;
	const TArray<FVestigeInventorySlot>& Slots = Inventory.GetSlots();
	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		if (!Slots[Index].IsEmpty())
		{
			Data.Slots.Add({Index, Slots[Index].Item->GetContentId(), Slots[Index].Count});
		}
	}
	return Data;
}

int32 UVestigeInventoryComponent::RestoreFromSaveData(const FVestigeInventorySaveData& Data)
{
	Inventory.Reset(SlotCount);
	int32 SkippedCount = 0;

	// Every stack goes back in its own slot first, so the leftovers can't take a slot another stack was saved in.
	TArray<TPair<const UVestigeItemData*, int32>> Leftovers;
	for (const FVestigeInventorySlotSaveData& Saved : Data.Slots)
	{
		const UVestigeItemData* Item = FindItem(FPrimaryAssetId(UVestigeItemData::AssetType, Saved.ItemId));
		if (!Item)
		{
			++SkippedCount;
			continue;
		}
		const int32 Left = Saved.Count - Inventory.PlaceInSlot(Saved.Slot, *Item, Saved.Count);
		if (Left > 0)
		{
			Leftovers.Emplace(Item, Left);
		}
	}

	for (const TPair<const UVestigeItemData*, int32>& Leftover : Leftovers)
	{
		const int32 Added = FMath::Min(Leftover.Value, Inventory.GetRoomFor(*Leftover.Key));
		if (Added > 0)
		{
			Inventory.TryAdd(*Leftover.Key, Added);
		}
		if (Added < Leftover.Value)
		{
			VESTIGE_LOG(this, LogVestigeInventory, Warning, "No room for {Count} saved {ItemId}, so they were left out.",
				Leftover.Value - Added, Leftover.Key->GetContentId());
			++SkippedCount;
		}
	}

	OnInventoryChanged.Broadcast();
	return SkippedCount;
}

#if WITH_DEV_AUTOMATION_TESTS
void UVestigeInventoryComponent::ResetForTests(int32 InSlotCount, TConstArrayView<const UVestigeItemData*> InItems)
{
	SlotCount = InSlotCount;
	Inventory.Reset(SlotCount);
	Items.Reset();
	for (const UVestigeItemData* Item : InItems)
	{
		Items.Add(Item->GetPrimaryAssetId(), Item);
	}
}
#endif
