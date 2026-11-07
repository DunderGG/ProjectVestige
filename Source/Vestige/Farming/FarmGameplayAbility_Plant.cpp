// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Farming/FarmGameplayAbility_Plant.h"

#include "Farming/FarmField.h"
#include "Farming/FarmingComponent.h"
#include "Farming/FarmTileState.h"
#include "Inventory/VestigeInventoryComponent.h"

#define LOCTEXT_NAMESPACE "FarmGameplayAbility_Plant"

bool UFarmGameplayAbility_Plant::CanUseOn(const FIntPoint& Tile) const
{
	const UFarmField* Field = GetField();
	const FFarmTileState* State = Field ? Field->GetTile(Tile) : nullptr;
	return State && State->CanPlant() && HasSeedToPlant();
}

bool UFarmGameplayAbility_Plant::UseOn(const FIntPoint& Tile)
{
	if (!HasSeedToPlant())
	{
		Notify(LOCTEXT("OutOfSeeds", "Out of seeds."));
		return false;
	}

	// HasSeedToPlant found the farming component and the inventory.
	const UFarmingComponent& FarmingComponent = *GetFarming();
	UFarmField* Field = GetField();
	if (!Field || !Field->Plant(Tile, FarmingComponent.GetSeedCrop()))
	{
		Notify(LOCTEXT("CantPlant", "Seeds need empty tilled soil."));
		return false;
	}
	GetInventory()->TryRemoveItem(FarmingComponent.GetSeedItemId(), 1);
	Notify(LOCTEXT("Planted", "Planted a seed."));
	return true;
}

bool UFarmGameplayAbility_Plant::HasSeedToPlant() const
{
	const UFarmingComponent* FarmingComponent = GetFarming();
	const UVestigeInventoryComponent* Inventory = GetInventory();
	return FarmingComponent && Inventory && FarmingComponent->GetSeedCrop()
		&& Inventory->GetItemCount(FarmingComponent->GetSeedItemId()) > 0;
}

#undef LOCTEXT_NAMESPACE
