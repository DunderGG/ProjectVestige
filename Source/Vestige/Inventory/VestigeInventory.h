// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "VestigeInventory.generated.h"

class UVestigeItemData;

/** One inventory slot: empty, or a stack of one item. */
USTRUCT()
struct FVestigeInventorySlot
{
	GENERATED_BODY()

	/** The item stacked here, or nullptr if the slot is empty. */
	UPROPERTY()
	TObjectPtr<const UVestigeItemData> Item;

	/** How many: from 1 to the item's stack size, or 0 when the slot is empty. */
	UPROPERTY()
	int32 Count = 0;

	bool IsEmpty() const { return Item == nullptr; }
};

/**
 * The rules of a fixed number of item slots: stacking, the stack size limit, and adding or removing all or nothing.
 * Holds loaded item definitions only; UVestigeInventoryComponent finds them by ID, fills it and broadcasts its changes.
 * Kept apart from the component so its rules can be tested without a level.
 */
USTRUCT()
struct FVestigeInventory
{
	GENERATED_BODY()

public:
	/** Empties every slot and makes the inventory SlotCount slots big. */
	void Reset(int32 SlotCount);

	/** True if all Count of Item fit, in stacks of it that aren't full and in empty slots. False for a Count below 1. */
	bool CanAdd(const UVestigeItemData& Item, int32 Count) const;

	/**
	 * Adds Count of Item, topping up its stacks in slot order before filling empty slots in order.
	 * Returns false, changing nothing, if they don't all fit or Count is below 1.
	 */
	bool TryAdd(const UVestigeItemData& Item, int32 Count);

	/** Removes Count of Item, from its last stacks first. Returns false, changing nothing, if there are fewer or Count is below 1. */
	bool TryRemove(const UVestigeItemData& Item, int32 Count);

	/**
	 * Removes Count from the slot at SlotIndex only, leaving other stacks of its item as they are. Returns false, changing
	 * nothing, if the slot doesn't exist or holds fewer, or Count is below 1.
	 */
	bool TryRemoveFromSlot(int32 SlotIndex, int32 Count);

	/**
	 * Puts up to Count of Item into the slot at SlotIndex, if that slot exists and is empty, never above the item's stack size.
	 * Returns how many it put there. For restoring a save, which keeps each stack in its slot.
	 */
	int32 PlaceInSlot(int32 SlotIndex, const UVestigeItemData& Item, int32 Count);

	/** How many of Item more would fit. */
	int32 GetRoomFor(const UVestigeItemData& Item) const;

	/** How many of Item there are, across all its stacks. */
	int32 GetCount(const UVestigeItemData& Item) const;

	const TArray<FVestigeInventorySlot>& GetSlots() const { return Slots; }

private:
	UPROPERTY()
	TArray<FVestigeInventorySlot> Slots;
};
