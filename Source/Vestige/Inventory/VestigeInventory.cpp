// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Inventory/VestigeInventory.h"

#include "Content/VestigeItemData.h"

namespace VestigeInventoryRules
{
	/** The item's stack size, never below 1, so a bad asset can't make room for nothing or divide anything by zero. */
	int32 GetStackSize(const UVestigeItemData& Item)
	{
		return FMath::Max(1, Item.GetMaxStackSize());
	}
}

void FVestigeInventory::Reset(int32 SlotCount)
{
	Slots.Reset();
	Slots.SetNum(FMath::Max(0, SlotCount));
}

bool FVestigeInventory::CanAdd(const UVestigeItemData& Item, int32 Count) const
{
	return Count > 0 && GetRoomFor(Item) >= Count;
}

bool FVestigeInventory::TryAdd(const UVestigeItemData& Item, int32 Count)
{
	if (!CanAdd(Item, Count))
	{
		return false;
	}

	const int32 StackSize = VestigeInventoryRules::GetStackSize(Item);
	int32 Left = Count;
	for (FVestigeInventorySlot& Slot : Slots)
	{
		if (Slot.Item == &Item)
		{
			const int32 Added = FMath::Min(Left, StackSize - Slot.Count);
			Slot.Count += Added;
			Left -= Added;
		}
	}
	for (FVestigeInventorySlot& Slot : Slots)
	{
		if (Left > 0 && Slot.IsEmpty())
		{
			Slot.Item = &Item;
			Slot.Count = FMath::Min(Left, StackSize);
			Left -= Slot.Count;
		}
	}
	return true;
}

bool FVestigeInventory::TryRemove(const UVestigeItemData& Item, int32 Count)
{
	if (Count <= 0 || GetCount(Item) < Count)
	{
		return false;
	}

	int32 Left = Count;
	for (int32 Index = Slots.Num() - 1; Index >= 0 && Left > 0; --Index)
	{
		FVestigeInventorySlot& Slot = Slots[Index];
		if (Slot.Item != &Item)
		{
			continue;
		}
		const int32 Removed = FMath::Min(Left, Slot.Count);
		Slot.Count -= Removed;
		Left -= Removed;
		if (Slot.Count == 0)
		{
			Slot.Item = nullptr;
		}
	}
	return true;
}

int32 FVestigeInventory::PlaceInSlot(int32 SlotIndex, const UVestigeItemData& Item, int32 Count)
{
	if (!Slots.IsValidIndex(SlotIndex) || !Slots[SlotIndex].IsEmpty() || Count <= 0)
	{
		return 0;
	}
	FVestigeInventorySlot& Slot = Slots[SlotIndex];
	Slot.Item = &Item;
	Slot.Count = FMath::Min(Count, VestigeInventoryRules::GetStackSize(Item));
	return Slot.Count;
}

int32 FVestigeInventory::GetRoomFor(const UVestigeItemData& Item) const
{
	const int32 StackSize = VestigeInventoryRules::GetStackSize(Item);
	int32 Room = 0;
	for (const FVestigeInventorySlot& Slot : Slots)
	{
		if (Slot.IsEmpty())
		{
			Room += StackSize;
		}
		else if (Slot.Item == &Item)
		{
			Room += StackSize - Slot.Count;
		}
	}
	return Room;
}

int32 FVestigeInventory::GetCount(const UVestigeItemData& Item) const
{
	int32 Count = 0;
	for (const FVestigeInventorySlot& Slot : Slots)
	{
		if (Slot.Item == &Item)
		{
			Count += Slot.Count;
		}
	}
	return Count;
}
