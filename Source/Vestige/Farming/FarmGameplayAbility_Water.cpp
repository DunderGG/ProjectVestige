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
	return State->CanWater() || FailBecause(OutFailure, VestigeGameplayTags::Feedback_Farm_CantWater);
}

bool UFarmGameplayAbility_Water::UseOn(const FIntPoint& Tile)
{
	UFarmField* Field = GetField();
	return Field && Field->Water(Tile);
}
