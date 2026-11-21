// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Farming/FarmTileState.h"

#include "Content/FarmCropData.h"

FFarmWaterRules FFarmWaterRules::GetClamped(const FFarmWaterRules& Rules)
{
	FFarmWaterRules Clamped;
	Clamped.MaxLevel = FMath::Max(Rules.MaxLevel, 1);
	Clamped.StepsPerWatering = FMath::Max(Rules.StepsPerWatering, 1);
	Clamped.NightlyFall = FMath::Max(Rules.NightlyFall, 1);
	return Clamped;
}

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

bool FFarmTileState::AddWater(int32 Steps, int32 MaxLevel)
{
	if (Steps < 1 || !CanAddWater(MaxLevel))
	{
		return false;
	}
	WaterLevel = FMath::Min(WaterLevel + Steps, MaxLevel);
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

bool FFarmTileState::Wither()
{
	if (!CanWither())
	{
		return false;
	}
	State = EFarmTileState::Withered;
	return true;
}

bool FFarmTileState::ClearWithered()
{
	if (!CanClearWithered())
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
	return State == EFarmTileState::Planted && Crop && Crop->IsRipeAt(GrowthDays);
}

void FFarmTileState::AdvanceDay(int32 NightlyFall)
{
	if (State == EFarmTileState::Planted && IsWatered() && !IsRipe())
	{
		++GrowthDays;
	}
	WaterLevel = FMath::Max(WaterLevel - NightlyFall, 0);
}

void FFarmTileState::Restore(
	EFarmTileState SavedState, int32 SavedWaterLevel, int32 SavedGrowthDays, const UFarmCropData* SavedCrop, int32 MaxLevel)
{
	// A planted or withered tile can't exist without its crop, so a missing crop leaves empty tilled soil.
	const bool bHasCropState = SavedState == EFarmTileState::Planted || SavedState == EFarmTileState::Withered;
	const bool bKeepsCrop = bHasCropState && SavedCrop;
	State = bHasCropState && !bKeepsCrop ? EFarmTileState::Tilled : SavedState;
	Crop = bKeepsCrop ? SavedCrop : nullptr;
	WaterLevel = State == EFarmTileState::Grass ? 0 : FMath::Clamp(SavedWaterLevel, 0, FMath::Max(MaxLevel, 0));
	GrowthDays = Crop ? FMath::Clamp(SavedGrowthDays, 0, Crop->GetDaysToMature()) : 0;
}
