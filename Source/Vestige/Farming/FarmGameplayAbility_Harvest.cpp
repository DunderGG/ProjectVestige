// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Farming/FarmGameplayAbility_Harvest.h"

#include "Content/FarmCropData.h"
#include "Farming/FarmField.h"
#include "Farming/FarmingComponent.h"
#include "Farming/FarmTileState.h"
#include "Inventory/VestigeInventoryComponent.h"

#define LOCTEXT_NAMESPACE "FarmGameplayAbility_Harvest"

bool UFarmGameplayAbility_Harvest::CanUseOn(const FIntPoint& Tile) const
{
	const UFarmField* Field = GetField();
	const FFarmTileState* State = Field ? Field->GetTile(Tile) : nullptr;
	return State && State->IsRipe() && State->GetCrop() && HasRoomForHarvest(*State->GetCrop());
}

bool UFarmGameplayAbility_Harvest::UseOn(const FIntPoint& Tile)
{
	UFarmField* Field = GetField();
	const FFarmTileState* State = Field ? Field->GetTile(Tile) : nullptr;
	const UFarmCropData* Crop = State ? State->GetCrop() : nullptr;
	if (!Crop || !State->IsRipe())
	{
		Notify(LOCTEXT("NothingRipe", "Nothing ripe here."));
		return false;
	}

	// Asked before harvesting, so a full bag leaves the crop in the ground instead of destroying it.
	if (!HasRoomForHarvest(*Crop))
	{
		Notify(LOCTEXT("BagFull", "No room in your bag."));
		return false;
	}
	if (!Field->Harvest(Tile))
	{
		return false;
	}
	GetInventory()->TryAddItem(Crop->GetHarvestItemId(), Crop->GetYield());
	Notify(LOCTEXT("Harvested", "Harvested a crop!"));
	if (UFarmingComponent* FarmingComponent = GetFarming())
	{
		FarmingComponent->ReportCropHarvested(*Crop, Tile);
	}
	return true;
}

bool UFarmGameplayAbility_Harvest::HasRoomForHarvest(const UFarmCropData& Crop) const
{
	const UVestigeInventoryComponent* Inventory = GetInventory();
	return Inventory && Inventory->CanAddItem(Crop.GetHarvestItemId(), Crop.GetYield());
}

#undef LOCTEXT_NAMESPACE
