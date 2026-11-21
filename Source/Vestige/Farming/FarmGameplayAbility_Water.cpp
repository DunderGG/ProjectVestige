// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Farming/FarmGameplayAbility_Water.h"

#include "AbilitySystem/VestigeGameplayTags.h"
#include "Farming/FarmField.h"
#include "Farming/FarmTileState.h"

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
	return true;
}

void UFarmGameplayAbility_Water::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	TilesWateredThisPress.Reset();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
