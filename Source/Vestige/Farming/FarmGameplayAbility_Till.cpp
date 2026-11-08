// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Farming/FarmGameplayAbility_Till.h"

#include "AbilitySystem/VestigeGameplayTags.h"
#include "Farming/FarmField.h"
#include "Farming/FarmTileState.h"

bool UFarmGameplayAbility_Till::CanUseOn(const FIntPoint& Tile, FGameplayTag* OutFailureReason) const
{
	const UFarmField* Field = GetField();
	const FFarmTileState* State = Field ? Field->GetTile(Tile) : nullptr;
	if (!State)
	{
		return FailBecause(OutFailureReason, VestigeGameplayTags::Feedback_Tool_NoTarget);
	}
	return State->CanTill() || FailBecause(OutFailureReason, VestigeGameplayTags::Feedback_Farm_CantTill);
}

bool UFarmGameplayAbility_Till::UseOn(const FIntPoint& Tile)
{
	UFarmField* Field = GetField();
	return Field && Field->Till(Tile);
}
