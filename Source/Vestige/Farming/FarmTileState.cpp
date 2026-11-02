// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Farming/FarmTileState.h"

#include "Content/FarmCropData.h"

bool FFarmTileState::Till()
{
	if (!CanTill())
	{
		return false;
	}
	State = EFarmTileState::Tilled;
	return true;
}

bool FFarmTileState::Plant(const UFarmCropData* InCrop)
{
	if (!CanPlant() || !InCrop)
	{
		return false;
	}
	State = EFarmTileState::Planted;
	Crop = InCrop;
	GrowthDays = 0;
	return true;
}

bool FFarmTileState::Water()
{
	if (!CanWater())
	{
		return false;
	}
	bWatered = true;
	return true;
}

bool FFarmTileState::Harvest()
{
	if (!IsRipe())
	{
		return false;
	}
	State = EFarmTileState::Tilled;
	Crop = nullptr;
	GrowthDays = 0;
	return true;
}

bool FFarmTileState::IsRipe() const
{
	return State == EFarmTileState::Planted && Crop && GrowthDays >= Crop->GetDaysToMature();
}

void FFarmTileState::AdvanceDay()
{
	if (State == EFarmTileState::Planted && bWatered && !IsRipe())
	{
		++GrowthDays;
	}
	bWatered = false;
}

void FFarmTileState::Restore(EFarmTileState SavedState, bool bSavedWatered, int32 SavedGrowthDays, const UFarmCropData* SavedCrop)
{
	// A planted tile can't exist without its crop, so a missing crop leaves empty tilled soil.
	const bool bPlanted = SavedState == EFarmTileState::Planted && SavedCrop;
	State = SavedState == EFarmTileState::Planted && !bPlanted ? EFarmTileState::Tilled : SavedState;
	Crop = bPlanted ? SavedCrop : nullptr;
	bWatered = bSavedWatered && State != EFarmTileState::Grass;
	GrowthDays = Crop ? FMath::Clamp(SavedGrowthDays, 0, Crop->GetDaysToMature()) : 0;
}
