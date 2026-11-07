// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Farming/FarmGameplayAbility_Till.h"

#include "Farming/FarmField.h"
#include "Farming/FarmTileState.h"

#define LOCTEXT_NAMESPACE "FarmGameplayAbility_Till"

bool UFarmGameplayAbility_Till::CanUseOn(const FIntPoint& Tile) const
{
	const UFarmField* Field = GetField();
	const FFarmTileState* State = Field ? Field->GetTile(Tile) : nullptr;
	return State && State->CanTill();
}

bool UFarmGameplayAbility_Till::UseOn(const FIntPoint& Tile)
{
	UFarmField* Field = GetField();
	const bool bTilled = Field && Field->Till(Tile);
	Notify(bTilled ? LOCTEXT("Tilled", "Tilled the soil.") : LOCTEXT("CantTill", "Only grass can be tilled."));
	return bTilled;
}

#undef LOCTEXT_NAMESPACE
