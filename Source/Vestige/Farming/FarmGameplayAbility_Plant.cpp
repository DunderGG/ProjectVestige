// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Farming/FarmGameplayAbility_Plant.h"

#include "AbilitySystem/VestigeGameplayTags.h"
#include "Content/FarmCropData.h"
#include "Content/FarmSeedFragment.h"
#include "Content/VestigeContentCatalog.h"
#include "Content/VestigeItemData.h"
#include "Farming/FarmField.h"
#include "Farming/FarmTileState.h"
#include "Inventory/VestigeInventoryComponent.h"

bool UFarmGameplayAbility_Plant::CanUseOn(const FIntPoint& Tile, FGameplayTag* OutFailureReason) const
{
	// The seed first: without one, the tile doesn't matter.
	if (!FindCropInHand())
	{
		return FailBecause(OutFailureReason, VestigeGameplayTags::Feedback_Farm_NothingToPlant);
	}
	const UFarmField* Field = GetField();
	const FFarmTileState* State = Field ? Field->GetTile(Tile) : nullptr;
	if (!State)
	{
		return FailBecause(OutFailureReason, VestigeGameplayTags::Feedback_Tool_NoTarget);
	}
	return State->CanPlant() || FailBecause(OutFailureReason, VestigeGameplayTags::Feedback_Farm_CantPlant);
}

bool UFarmGameplayAbility_Plant::UseOn(const FIntPoint& Tile)
{
	const UFarmCropData* Crop = FindCropInHand();
	UFarmField* Field = GetField();
	if (!Crop || !Field || !Field->Plant(Tile, Crop))
	{
		return false;
	}

	// From the stack in hand, so another stack of the same seed stays as it is. This can empty the hand, which takes this
	// ability back once its use has finished.
	UVestigeInventoryComponent* Inventory = GetInventory();
	Inventory->TryRemoveItemFromSlot(Inventory->GetSelectedHotbarSlot(), 1);
	return true;
}

const UFarmCropData* UFarmGameplayAbility_Plant::FindCropInHand() const
{
	const UVestigeInventoryComponent* Inventory = GetInventory();
	const UVestigeItemData* Item = Inventory ? Inventory->GetItemInHand() : nullptr;
	const UFarmSeedFragment* Seed = Item ? Item->FindFragment<UFarmSeedFragment>() : nullptr;
	const UVestigeContentCatalog* Catalog = Seed ? UVestigeContentCatalog::Get(GetWorld()) : nullptr;
	return Catalog ? Catalog->Find<UFarmCropData>(Seed->GetCropId()) : nullptr;
}
