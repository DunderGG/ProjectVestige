// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Tests/VestigeTestToolAbility.h"

#if WITH_TESTS

#include "Farming/FarmField.h"
#include "Farming/FarmTileState.h"

bool UVestigeTestToolAbility::CanUseOn(const FIntPoint& Tile) const
{
	const UFarmField* Field = GetField();
	const FFarmTileState* State = Field ? Field->GetTile(Tile) : nullptr;
	return State && State->CanTill();
}

bool UVestigeTestToolAbility::UseOn(const FIntPoint& Tile)
{
	UFarmField* Field = GetField();
	return Field && Field->Till(Tile) && Field->Water(Tile);
}

#endif
