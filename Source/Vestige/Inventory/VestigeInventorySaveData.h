// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "VestigeInventorySaveData.generated.h"

/** One stat of the item in a slot (Systems.md, section 8, slot stats): the stat tag's name and its value. */
USTRUCT()
struct FVestigeItemStatSaveData
{
	GENERATED_BODY()

	/** The stat tag's name (Vestige.ItemStat.Water.Clean), which a rename would break. */
	UPROPERTY()
	FName Stat;

	UPROPERTY()
	int32 Value = 0;
};

/** One non-empty inventory slot in a save. */
USTRUCT()
struct FVestigeInventorySlotSaveData
{
	GENERATED_BODY()

	/** The slot's position, from 0. */
	UPROPERTY()
	int32 Slot = INDEX_NONE;

	/** ContentId of the item stacked there (Base.Turnip). */
	UPROPERTY()
	FName ItemId;

	UPROPERTY()
	int32 Count = 0;

	/**
	 * The item's stats. A save from before stats has none, which the layout allows without a new version: the item is then fresh.
	 */
	UPROPERTY()
	TArray<FVestigeItemStatSaveData> Stats;
};

/**
 * UVestigeInventoryComponent's part of a save, under Base.Inventory: plain values and content IDs only. The inventory fills
 * it and restores from it.
 */
USTRUCT()
struct FVestigeInventorySaveData
{
	GENERATED_BODY()

	/**
	 * This layout's version (Systems.md, section 1). Raise it when a change would load older parts wrong. 2 since the tools
	 * became items: a version 1 bag holds no hoe or watering can, so reading it would leave the player unable to farm.
	 */
	static constexpr int32 Version = 2;

	/** The non-empty slots, in slot order. */
	UPROPERTY()
	TArray<FVestigeInventorySlotSaveData> Slots;

	/** The selected hotbar slot, whose item is in hand. */
	UPROPERTY()
	int32 SelectedHotbarSlot = 0;
};
