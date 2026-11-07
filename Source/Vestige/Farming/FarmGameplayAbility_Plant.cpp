// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Farming/FarmGameplayAbility_Plant.h"

#include "Content/FarmSeedFragment.h"
#include "Content/VestigeItemData.h"
#include "Farming/FarmField.h"
#include "Farming/FarmingComponent.h"
#include "Farming/FarmTileState.h"
#include "Inventory/VestigeInventoryComponent.h"

#define LOCTEXT_NAMESPACE "FarmGameplayAbility_Plant"

bool UFarmGameplayAbility_Plant::CanUseOn(const FIntPoint& Tile) const
{
	const UFarmField* Field = GetField();
	const FFarmTileState* State = Field ? Field->GetTile(Tile) : nullptr;
	return State && State->CanPlant() && FindCropInHand();
}

bool UFarmGameplayAbility_Plant::UseOn(const FIntPoint& Tile)
{
	const UFarmCropData* Crop = FindCropInHand();
	if (!Crop)
	{
		Notify(LOCTEXT("NothingToPlant", "Nothing to plant in hand."));
		return false;
	}

	UFarmField* Field = GetField();
	if (!Field || !Field->Plant(Tile, Crop))
	{
		Notify(LOCTEXT("CantPlant", "Seeds need empty tilled soil."));
		return false;
	}

	// From the stack in hand, so another stack of the same seed stays as it is. This can empty the hand, which takes this
	// ability back once its use has finished.
	UVestigeInventoryComponent* Inventory = GetInventory();
	Inventory->TryRemoveItemFromSlot(Inventory->GetSelectedHotbarSlot(), 1);
	Notify(LOCTEXT("Planted", "Planted a seed."));
	return true;
}

const UFarmCropData* UFarmGameplayAbility_Plant::FindCropInHand() const
{
	const UFarmingComponent* FarmingComponent = GetFarming();
	const UVestigeInventoryComponent* Inventory = GetInventory();
	const UVestigeItemData* Item = Inventory ? Inventory->GetItemInHand() : nullptr;
	const UFarmSeedFragment* Seed = Item ? Item->FindFragment<UFarmSeedFragment>() : nullptr;
	return FarmingComponent && Seed ? FarmingComponent->FindCrop(Seed->GetCropId()) : nullptr;
}

#undef LOCTEXT_NAMESPACE
