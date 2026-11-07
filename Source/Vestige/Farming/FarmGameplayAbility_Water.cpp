// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Farming/FarmGameplayAbility_Water.h"

#include "Farming/FarmField.h"
#include "Farming/FarmTileState.h"

#define LOCTEXT_NAMESPACE "FarmGameplayAbility_Water"

bool UFarmGameplayAbility_Water::CanUseOn(const FIntPoint& Tile) const
{
	const UFarmField* Field = GetField();
	const FFarmTileState* State = Field ? Field->GetTile(Tile) : nullptr;
	return State && State->CanWater();
}

bool UFarmGameplayAbility_Water::UseOn(const FIntPoint& Tile)
{
	UFarmField* Field = GetField();
	const bool bWatered = Field && Field->Water(Tile);
	Notify(bWatered ? LOCTEXT("Watered", "Watered.") : LOCTEXT("CantWater", "Nothing to water here."));
	return bWatered;
}

#undef LOCTEXT_NAMESPACE
