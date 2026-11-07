// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/VestigeSaveable.h"
#include "Inventory/VestigeInventory.h"
#include "VestigeInventoryComponent.generated.h"

class UVestigeItemData;
struct FVestigeInventorySaveData;

DECLARE_MULTICAST_DELEGATE(FOnVestigeInventoryChanged);
DECLARE_MULTICAST_DELEGATE(FOnVestigeItemInHandChanged);

/**
 * An item and how many of it, for setting up an inventory's contents. BlueprintType so that editor Python scripts can set
 * starting items on a Blueprint.
 */
USTRUCT(BlueprintType)
struct FVestigeItemStack
{
	GENERATED_BODY()

	/** The item (VestigeItem:Base.TurnipSeeds). */
	UPROPERTY(EditAnywhere, Category = "Inventory", meta = (AllowedTypes = "VestigeItem"))
	FPrimaryAssetId ItemId;

	UPROPERTY(EditAnywhere, Category = "Inventory", meta = (ClampMin = 1))
	int32 Count = 1;
};

/**
 * The items the player carries: a fixed number of slots, each empty or holding a stack of one item. Lives on the player
 * state, so it outlives the pawn. Loads every item definition at BeginPlay, so adding an item during play never waits on
 * a load, and broadcasts OnInventoryChanged after every change. The stacking rules are in FVestigeInventory.
 * The first slots are the hotbar (HotbarSlotCount): one of them is selected, and its item is the item in hand, which the
 * farming component uses. Saves its slots and the selected slot under Base.Inventory through UVestigeSaveRegistry while it
 * is in play, so the player's bag crosses a map change; it is the player's inventory, and a second inventory (a chest)
 * will need a key of its own and a HotbarSlotCount of 0. Systems.md, section 8.
 */
UCLASS(ClassGroup = (Inventory), meta = (BlueprintSpawnableComponent))
class UVestigeInventoryComponent : public UActorComponent, public IVestigeSaveable
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

	/** Removes Count from the slot at Slot only. Returns false, changing nothing, if that slot holds fewer. */
	bool TryRemoveItemFromSlot(int32 Slot, int32 Count);

	/** How many of the item the inventory holds, across all its stacks. */
	int32 GetItemCount(const FPrimaryAssetId& ItemId) const;

	const TArray<FVestigeInventorySlot>& GetSlots() const { return Inventory.GetSlots(); }

	/** How many of the first slots are the hotbar: HotbarSlotCount, never more than there are slots. */
	int32 GetHotbarSlotCount() const;

	/**
	 * Selects the hotbar slot Slot (0 is the first), whose item is then in hand. Returns false, changing nothing, if Slot is
	 * outside the hotbar.
	 */
	bool SelectHotbarSlot(int32 Slot);

	/** The selected hotbar slot, or INDEX_NONE if the inventory has no hotbar. */
	int32 GetSelectedHotbarSlot() const { return SelectedHotbarSlot; }

	/** The item in the selected hotbar slot, or nullptr when that slot is empty or there is no hotbar. */
	const UVestigeItemData* GetItemInHand() const;

	/** The non-empty slots, for saving. */
	FVestigeInventorySaveData GetSaveData() const;

	/**
	 * Replaces the contents with the saved slots and selected hotbar slot, without replaying any action, and broadcasts
	 * OnInventoryChanged and, if the hand changed, OnItemInHandChanged. Each stack goes back in its slot; what doesn't fit
	 * there (a bad slot, a stack size lowered since) is added like any item. A selected slot outside the hotbar selects the
	 * first, with a warning. Returns how many saved stacks were skipped, wholly or partly: unknown items, and items with no
	 * room left.
	 */
	int32 RestoreFromSaveData(const FVestigeInventorySaveData& Data);

	/** Empties the slots, selects the first hotbar slot and adds the starting items, then broadcasts the changes. */
	virtual void StartNewGame() override;

	virtual FName GetSaveKey() const override;
	virtual int32 GetSaveVersion() const override;
	virtual FVestigeSaveRecord WriteSaveRecord() const override;
	virtual bool ReadSaveRecord(const FVestigeSaveRecord& Record, int32& OutLeftOutCount) override;

	/** Broadcast after every change, including when a save is loaded. */
	FOnVestigeInventoryChanged OnInventoryChanged;

	/**
	 * Broadcast after OnInventoryChanged when the selected hotbar slot or the item in it changed (not its count alone),
	 * including when a save is loaded. It carries no value: a listener called during a nested broadcast may be called for
	 * the newer change first, so listeners read GetSelectedHotbarSlot and GetItemInHand when called.
	 */
	FOnVestigeItemInHandChanged OnItemInHandChanged;

#if WITH_DEV_AUTOMATION_TESTS
	/**
	 * For tests only: empties the inventory to this many slots, selects the first hotbar slot and makes these item
	 * definitions known, without loading. Broadcasts the hand if it changed.
	 */
	void ResetForTests(int32 InSlotCount, TConstArrayView<const UVestigeItemData*> InItems);
#endif

protected:
	/** Loads the item definitions, then takes its saved slots, or the starting items when there are none. */
	virtual void BeginPlay() override;

	/** Keeps its slots for the next map before it goes. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Loads every item definition the Asset Manager knows into Items. */
	void LoadItems();

	void AddStartingItems();

	/** The loaded definition of the item with this ID, or nullptr, with a warning, if there is none. */
	const UVestigeItemData* FindItem(const FPrimaryAssetId& ItemId) const;

	/** Empties the slots to SlotCount and selects the first hotbar slot, without broadcasting. */
	void ResetSlots();

	/** Selects the hotbar slot Slot without broadcasting. Returns false, changing nothing, if Slot is outside the hotbar. */
	bool SelectHotbarSlotSilently(int32 Slot);

	/** Broadcasts OnInventoryChanged, then BroadcastItemInHandIfChanged. */
	void BroadcastChanges();

	/**
	 * Broadcasts OnItemInHandChanged if the selected slot or its item differs from the last broadcast. What it remembers is
	 * updated before broadcasting, so a listener that changes the inventory again is compared against the latest state.
	 */
	void BroadcastItemInHandIfChanged();

	/** How many slots the inventory has. */
	UPROPERTY(EditDefaultsOnly, Category = "Inventory", meta = (AllowPrivateAccess = "true", ClampMin = 1))
	int32 SlotCount = 24;

	/** How many of the first slots are the hotbar, selected with keys 1 to 9. 0 for an inventory without a hand (a chest). */
	UPROPERTY(EditDefaultsOnly, Category = "Inventory", meta = (AllowPrivateAccess = "true", ClampMin = 0))
	int32 HotbarSlotCount = 9;

	/** The selected hotbar slot, or INDEX_NONE without a hotbar. */
	int32 SelectedHotbarSlot = 0;

	/** The selected slot and its item at the last OnItemInHandChanged, to tell whether the hand changed. */
	int32 BroadcastHotbarSlot = INDEX_NONE;
	TWeakObjectPtr<const UVestigeItemData> BroadcastItemInHand;

	/** What the inventory holds when a game starts without a save. */
	UPROPERTY(EditDefaultsOnly, Category = "Inventory", meta = (AllowPrivateAccess = "true"))
	TArray<FVestigeItemStack> StartingItems;

	/** Every item definition, by its primary asset ID, loaded at BeginPlay. */
	UPROPERTY()
	TMap<FPrimaryAssetId, TObjectPtr<const UVestigeItemData>> Items;

	UPROPERTY()
	FVestigeInventory Inventory;
};
