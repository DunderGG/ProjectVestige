// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/VestigeInventory.h"
#include "VestigeInventoryComponent.generated.h"

class UVestigeItemData;
struct FVestigeInventorySaveData;

DECLARE_MULTICAST_DELEGATE(FOnVestigeInventoryChanged);

/**
 * An item and how many of it, for setting up an inventory's contents. BlueprintType so that editor Python scripts can set
 * starting items on a Blueprint.
 */
USTRUCT(BlueprintType)
struct FVestigeItemStack
{
	GENERATED_BODY()

	/** The item (FarmItem:Base.TurnipSeeds). */
	UPROPERTY(EditAnywhere, Category = "Inventory", meta = (AllowedTypes = "FarmItem"))
	FPrimaryAssetId ItemId;

	UPROPERTY(EditAnywhere, Category = "Inventory", meta = (ClampMin = 1))
	int32 Count = 1;
};

/**
 * The items the player carries: a fixed number of slots, each empty or holding a stack of one item. Lives on the player
 * state, so it outlives the pawn. Loads every item definition at BeginPlay, so adding an item during play never waits on
 * a load, and broadcasts OnInventoryChanged after every change. The stacking rules are in FVestigeInventory.
 */
UCLASS(ClassGroup = (Inventory), meta = (BlueprintSpawnableComponent))
class UVestigeInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UVestigeInventoryComponent();

	/** True if all Count of the item fit. False for an ID that names no item. */
	bool CanAddItem(const FPrimaryAssetId& ItemId, int32 Count) const;

	/** Adds Count of the item. Returns false, changing nothing, if they don't all fit or the ID names no item. */
	bool TryAddItem(const FPrimaryAssetId& ItemId, int32 Count);

	/** Removes Count of the item. Returns false, changing nothing, if there are fewer. */
	bool TryRemoveItem(const FPrimaryAssetId& ItemId, int32 Count);

	/** How many of the item the inventory holds, across all its stacks. */
	int32 GetItemCount(const FPrimaryAssetId& ItemId) const;

	const TArray<FVestigeInventorySlot>& GetSlots() const { return Inventory.GetSlots(); }

	/** The non-empty slots, for saving. */
	FVestigeInventorySaveData GetSaveData() const;

	/**
	 * Replaces the contents with the saved slots, without replaying any action, and broadcasts OnInventoryChanged.
	 * Each stack goes back in its slot; what doesn't fit there (a bad slot, a stack size lowered since) is added like any
	 * item. Returns how many saved stacks were skipped, wholly or partly: unknown items, and items with no room left.
	 */
	int32 RestoreFromSaveData(const FVestigeInventorySaveData& Data);

	/** Broadcast after every change, including when a save is loaded. */
	FOnVestigeInventoryChanged OnInventoryChanged;

#if WITH_DEV_AUTOMATION_TESTS
	/** For tests only: empties the inventory to this many slots and makes these item definitions known, without loading. */
	void ResetForTests(int32 InSlotCount, TConstArrayView<const UVestigeItemData*> InItems);
#endif

protected:
	virtual void BeginPlay() override;

private:
	/** Loads every item definition the Asset Manager knows into Items. */
	void LoadItems();

	void AddStartingItems();

	/** The loaded definition of the item with this ID, or nullptr, with a warning, if there is none. */
	const UVestigeItemData* FindItem(const FPrimaryAssetId& ItemId) const;

	/** How many slots the inventory has. */
	UPROPERTY(EditDefaultsOnly, Category = "Inventory", meta = (AllowPrivateAccess = "true", ClampMin = 1))
	int32 SlotCount = 24;

	/** What the inventory holds when a game starts without a save. */
	UPROPERTY(EditDefaultsOnly, Category = "Inventory", meta = (AllowPrivateAccess = "true"))
	TArray<FVestigeItemStack> StartingItems;

	/** Every item definition, by its primary asset ID, loaded at BeginPlay. */
	UPROPERTY()
	TMap<FPrimaryAssetId, TObjectPtr<const UVestigeItemData>> Items;

	UPROPERTY()
	FVestigeInventory Inventory;
};
