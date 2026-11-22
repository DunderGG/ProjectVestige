// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Farming/FarmWateringCan.h"

#include "AbilitySystem/VestigeGameplayTags.h"
#include "Content/FarmWaterContainerFragment.h"
#include "Content/VestigeItemData.h"
#include "Inventory/VestigeInventoryComponent.h"

namespace FarmWateringCan
{
	int32 GetCapacity(const UVestigeItemData* Item)
	{
		const UFarmWaterContainerFragment* Container = Item ? Item->FindFragment<UFarmWaterContainerFragment>() : nullptr;
		return Container ? Container->GetCapacity() : INDEX_NONE;
	}

	int32 GetWater(const UVestigeInventoryComponent& Inventory, int32 Slot)
	{
		const TArray<FVestigeInventorySlot>& Slots = Inventory.GetSlots();
		const int32 Capacity = Slots.IsValidIndex(Slot) ? GetCapacity(Slots[Slot].Item) : INDEX_NONE;
		if (Capacity == INDEX_NONE)
		{
			return INDEX_NONE;
		}

		// An absent stat is a full can; a stat of 0 is an empty one.
		const TOptional<int32> Stat = Inventory.FindSlotStat(Slot, VestigeGameplayTags::ItemStat_Water_Clean);
		return Stat ? FMath::Clamp(*Stat, 0, Capacity) : Capacity;
	}

	bool Pour(UVestigeInventoryComponent& Inventory, int32 Slot)
	{
		const int32 Water = GetWater(Inventory, Slot);
		if (Water < WaterPerWatering)
		{
			return false;
		}
		return Inventory.SetSlotStat(Slot, VestigeGameplayTags::ItemStat_Water_Clean, Water - WaterPerWatering);
	}

	int32 FindCanToFill(const UVestigeInventoryComponent& Inventory)
	{
		int32 FirstCan = INDEX_NONE;
		const TArray<FVestigeInventorySlot>& Slots = Inventory.GetSlots();
		for (int32 Slot = 0; Slot < Slots.Num(); ++Slot)
		{
			const int32 Capacity = GetCapacity(Slots[Slot].Item);
			if (Capacity == INDEX_NONE)
			{
				continue;
			}
			if (GetWater(Inventory, Slot) < Capacity)
			{
				return Slot;
			}
			if (FirstCan == INDEX_NONE)
			{
				FirstCan = Slot;
			}
		}
		return FirstCan;
	}

	int32 Fill(UVestigeInventoryComponent& Inventory, int32 Slot, int32 Amount)
	{
		const int32 Water = GetWater(Inventory, Slot);
		if (Water == INDEX_NONE)
		{
			return 0;
		}
		const int32 Capacity = GetCapacity(Inventory.GetSlots()[Slot].Item);
		const int32 Added = FMath::Clamp(Amount, 0, Capacity - Water);
		if (Added > 0 && !Inventory.SetSlotStat(Slot, VestigeGameplayTags::ItemStat_Water_Clean, Water + Added))
		{
			return 0;
		}
		return Added;
	}
}
