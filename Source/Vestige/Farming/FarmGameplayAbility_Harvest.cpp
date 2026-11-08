// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Farming/FarmGameplayAbility_Harvest.h"

#include "AbilitySystem/VestigeGameplayTags.h"
#include "Content/FarmCropData.h"
#include "Farming/FarmField.h"
#include "Farming/FarmingComponent.h"
#include "Farming/FarmTileState.h"
#include "Inventory/VestigeInventoryComponent.h"

bool UFarmGameplayAbility_Harvest::CanUseOn(const FIntPoint& Tile, FGameplayTag* OutFailureReason) const
{
	const UFarmField* Field = GetField();
	const FFarmTileState* State = Field ? Field->GetTile(Tile) : nullptr;
	if (!State)
	{
		return FailBecause(OutFailureReason, VestigeGameplayTags::Feedback_Tool_NoTarget);
	}
	if (!State->IsRipe() || !State->GetCrop())
	{
		return FailBecause(OutFailureReason, VestigeGameplayTags::Feedback_Farm_NothingRipe);
	}
	return HasRoomForHarvest(*State->GetCrop()) || FailBecause(OutFailureReason, VestigeGameplayTags::Feedback_Inventory_Full);
}

bool UFarmGameplayAbility_Harvest::UseOn(const FIntPoint& Tile)
{
	UFarmField* Field = GetField();
	const FFarmTileState* State = Field ? Field->GetTile(Tile) : nullptr;
	const UFarmCropData* Crop = State ? State->GetCrop() : nullptr;

	// Room is asked before harvesting, so a full bag leaves the crop in the ground instead of destroying it.
	if (!Crop || !State->IsRipe() || !HasRoomForHarvest(*Crop) || !Field->Harvest(Tile))
	{
		return false;
	}
	GetInventory()->TryAddItem(Crop->GetHarvestItemId(), Crop->GetYield());
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
