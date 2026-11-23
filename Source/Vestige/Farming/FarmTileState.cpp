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
	WateredNights = 0;
	bMutated = false;
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
	Contamination = FMath::Max(Contamination - Crop->GetContaminationCleanedPerHarvest(), 0);
	State = EFarmTileState::Tilled;
	Crop = nullptr;
	GrowthDays = 0;
	WateredNights = 0;
	bMutated = false;
	return true;
}

bool FFarmTileState::Wither()
{
	if (!CanWither())
	{
		return false;
	}
	State = EFarmTileState::Withered;
	WateredNights = 0;
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
	WateredNights = 0;
	bMutated = false;
	return true;
}

bool FFarmTileState::IsRipe() const
{
	return State == EFarmTileState::Planted && Crop && Crop->IsRipeAt(GrowthDays);
}

bool FFarmTileState::AdvanceDay(int32 NightlyFall)
{
	bool bRipenedTonight = false;
	if (State == EFarmTileState::Planted && IsWatered() && !IsRipe())
	{
		// A dry night counts nothing, so a slowed crop grows on every Nth watered night, not every Nth night.
		const int32 NightsPerGrowthDay = Crop->GetWateredNightsPerGrowthDay(Contamination);
		if (NightsPerGrowthDay > 0 && ++WateredNights >= NightsPerGrowthDay)
		{
			++GrowthDays;
			WateredNights = 0;
			bRipenedTonight = IsRipe();
		}
	}
	WaterLevel = FMath::Max(WaterLevel - NightlyFall, 0);
	return bRipenedTonight;
}

bool FFarmTileState::SetContamination(int32 Level, int32 MaxLevel)
{
	const int32 NewLevel = FMath::Clamp(Level, 0, FMath::Max(MaxLevel, 0));
	if (NewLevel == Contamination)
	{
		return false;
	}
	Contamination = NewLevel;
	return true;
}

bool FFarmTileState::Mutate()
{
	if (!IsRipe() || bMutated || !Crop->CanMutate())
	{
		return false;
	}
	bMutated = true;
	return true;
}

FPrimaryAssetId FFarmTileState::GetHarvestItemId() const
{
	if (!Crop)
	{
		return FPrimaryAssetId();
	}
	return bMutated ? Crop->GetMutatedHarvestItemId() : Crop->GetHarvestItemId();
}

void FFarmTileState::Restore(
	const FFarmTileSaveData& Saved, const UFarmCropData* SavedCrop, int32 MaxWaterLevel, int32 MaxContaminationLevel)
{
	// A planted or withered tile can't exist without its crop, so a missing crop leaves empty tilled soil.
	const bool bHasCropState = Saved.State == EFarmTileState::Planted || Saved.State == EFarmTileState::Withered;
	const bool bKeepsCrop = bHasCropState && SavedCrop;
	State = bHasCropState && !bKeepsCrop ? EFarmTileState::Tilled : Saved.State;
	Crop = bKeepsCrop ? SavedCrop : nullptr;
	WaterLevel = State == EFarmTileState::Grass ? 0 : FMath::Clamp(Saved.WaterLevel, 0, FMath::Max(MaxWaterLevel, 0));
	GrowthDays = Crop ? FMath::Clamp(Saved.GrowthDays, 0, Crop->GetDaysToMature()) : 0;

	// The level first: the watered nights a growing crop may have counted depend on it.
	Contamination = FMath::Clamp(Saved.Contamination, 0, FMath::Max(MaxContaminationLevel, 0));
	const bool bGrowing = State == EFarmTileState::Planted && !IsRipe();
	const int32 MaxWateredNights = bGrowing ? FMath::Max(Crop->GetWateredNightsPerGrowthDay(Contamination) - 1, 0) : 0;
	WateredNights = FMath::Clamp(Saved.WateredNights, 0, MaxWateredNights);
	bMutated = Saved.bMutated && IsRipe() && Crop->CanMutate();
}
