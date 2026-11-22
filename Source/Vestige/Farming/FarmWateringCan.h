// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"

class UVestigeInventoryComponent;
class UVestigeItemData;

/**
 * The watering can's rules over an inventory slot, in one place for the watering ability, the well and the HUD. An item is a
 * can when it has a UFarmWaterContainerFragment; its water is the stat Vestige.ItemStat.Water.Clean of its slot, where an
 * absent stat means a full can (a new can, or one from a save made before the stat existed). Systems.md, section 24.
 */
namespace FarmWateringCan
{
	/** What one watering pours: a watering is one step on one tile, and the step's size is the tile rules'. */
	constexpr int32 WaterPerWatering = 1;

	/** The capacity of Item's water container fragment, or INDEX_NONE for no item or an item without one. */
	int32 GetCapacity(const UVestigeItemData* Item);

	/** The water in the can in Slot, from 0 to its capacity, or INDEX_NONE if the slot holds no can. */
	int32 GetWater(const UVestigeInventoryComponent& Inventory, int32 Slot);

	/** Takes one watering's water from the can in Slot. Returns false, changing nothing, if it holds less or isn't a can. */
	bool Pour(UVestigeInventoryComponent& Inventory, int32 Slot);

	/**
	 * The slot of the can to fill: the first, in slot order, that isn't full; else the first can there is (a full one); else
	 * INDEX_NONE. So the well finds the can wherever it is in the bag, and with two cans never skips one that needs water.
	 */
	int32 FindCanToFill(const UVestigeInventoryComponent& Inventory);

	/** Adds up to Amount to the can in Slot, never above its capacity, and returns how much it added (0 if the slot holds no can). */
	int32 Fill(UVestigeInventoryComponent& Inventory, int32 Slot, int32 Amount);
}
