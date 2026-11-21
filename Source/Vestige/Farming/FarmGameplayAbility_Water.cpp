// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Farming/FarmGameplayAbility_Water.h"

#include "AbilitySystem/VestigeGameplayTags.h"
#include "Farming/FarmField.h"
#include "Farming/FarmLog.h"
#include "Farming/FarmTileState.h"
#include "Farming/FarmWateringCan.h"
#include "Inventory/VestigeInventoryComponent.h"

bool UFarmGameplayAbility_Water::CanUseToolNow(FVestigeFeedbackReport* OutFailure) const
{
	const UVestigeInventoryComponent* Inventory = GetInventory();
	const int32 Slot = Inventory ? Inventory->GetSelectedHotbarSlot() : INDEX_NONE;
	const int32 Water = Inventory ? FarmWateringCan::GetWater(*Inventory, Slot) : INDEX_NONE;
	if (Inventory && Water == INDEX_NONE && !bWarnedAboutMissingContainer)
	{
		bWarnedAboutMissingContainer = true;
		// The item names the watering can's tool but isn't a water container: a content mistake, refused as an empty can.
		VESTIGE_LOG(this, LogFarm, Warning, "The item in hand has no water container fragment, so it waters nothing.");
	}
	return Water >= FarmWateringCan::WaterPerWatering || FailBecause(OutFailure, VestigeGameplayTags::Feedback_Water_CanEmpty);
}

bool UFarmGameplayAbility_Water::CanUseOn(const FIntPoint& Tile, FVestigeFeedbackReport* OutFailure) const
{
	const UFarmField* Field = GetField();
	const FFarmTileState* State = Field ? Field->GetTile(Tile) : nullptr;
	if (!State)
	{
		return FailBecause(OutFailure, VestigeGameplayTags::Feedback_Tool_NoTarget);
	}
	if (State->GetState() == EFarmTileState::Grass)
	{
		return FailBecause(OutFailure, VestigeGameplayTags::Feedback_Farm_CantWater);
	}
	const bool bCanAddWater = State->CanAddWater(Field->GetWaterRules().MaxLevel) && !TilesWateredThisPress.Contains(Tile);
	return bCanAddWater || FailBecause(OutFailure, VestigeGameplayTags::Feedback_Farm_Soaked);
}

bool UFarmGameplayAbility_Water::UseOn(const FIntPoint& Tile)
{
	UFarmField* Field = GetField();
	if (!Field || !Field->Water(Tile))
	{
		return false;
	}
	TilesWateredThisPress.Add(Tile);

	// Nothing runs between CanUseToolNow and here that should change the bag, so a failed pour is a bug; the watering stands.
	UVestigeInventoryComponent* Inventory = GetInventory();
	if (!Inventory || !FarmWateringCan::TryPour(*Inventory, Inventory->GetSelectedHotbarSlot()))
	{
		VESTIGE_LOG(this, LogFarm, Warning, "Watered tile {Tile} but the can had no water to pour.", Tile.ToString());
	}
	return true;
}

void UFarmGameplayAbility_Water::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	TilesWateredThisPress.Reset();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
