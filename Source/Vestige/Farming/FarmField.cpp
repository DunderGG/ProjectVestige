// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Farming/FarmField.h"

#include "Algo/Count.h"
#include "Content/FarmCropData.h"
#include "Content/VestigeCalendar.h"
#include "Farming/FarmLog.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

void UFarmField::Initialize(
	FName InFieldId, const FIntPoint& InSize, const FFarmWaterRules& InWaterRules, const FFarmContaminationRules& InContaminationRules)
{
	FieldId = InFieldId;
	WaterRules = FFarmWaterRules::GetClamped(InWaterRules);
	ContaminationRules = FFarmContaminationRules::GetClamped(InContaminationRules);
	Size = FIntPoint(FMath::Max(InSize.X, 1), FMath::Max(InSize.Y, 1));
	Tiles.Reset();
	Tiles.SetNum(Size.X * Size.Y);
}

bool UFarmField::Contains(const FIntPoint& Coordinates) const
{
	return Coordinates.X >= 0 && Coordinates.X < Size.X && Coordinates.Y >= 0 && Coordinates.Y < Size.Y;
}

int32 UFarmField::ToIndex(const FIntPoint& Coordinates) const
{
	return Contains(Coordinates) ? Coordinates.Y * Size.X + Coordinates.X : INDEX_NONE;
}

const FFarmTileState* UFarmField::GetTile(const FIntPoint& Coordinates) const
{
	const int32 Index = ToIndex(Coordinates);
	return Index == INDEX_NONE ? nullptr : &Tiles[Index];
}

bool UFarmField::Till(const FIntPoint& Coordinates)
{
	return ChangeTile(Coordinates,
		[](FFarmTileState& Tile)
		{
			return Tile.Till();
		});
}

bool UFarmField::Plant(const FIntPoint& Coordinates, const UFarmCropData* Crop)
{
	return ChangeTile(Coordinates,
		[Crop](FFarmTileState& Tile)
		{
			return Tile.Plant(Crop);
		});
}

bool UFarmField::Water(const FIntPoint& Coordinates)
{
	return ChangeTile(Coordinates,
		[this](FFarmTileState& Tile)
		{
			return Tile.AddWater(WaterRules.StepsPerWatering, WaterRules.MaxLevel);
		});
}

bool UFarmField::Harvest(const FIntPoint& Coordinates)
{
	return ChangeTile(Coordinates,
		[](FFarmTileState& Tile)
		{
			return Tile.Harvest();
		});
}

bool UFarmField::ClearWithered(const FIntPoint& Coordinates)
{
	return ChangeTile(Coordinates,
		[](FFarmTileState& Tile)
		{
			return Tile.ClearWithered();
		});
}

int32 UFarmField::WitherCropsOutOfSeason(const FGameplayTag& Season, const FVestigeCalendar& Calendar)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UFarmField::WitherCropsOutOfSeason);

	int32 WitheredCount = 0;
	for (FFarmTileState& Tile : Tiles)
	{
		if (Tile.CanWither() && Tile.GetCrop() && !Tile.GetCrop()->GrowsIn(Season, Calendar) && Tile.Wither())
		{
			++WitheredCount;
		}
	}
	if (WitheredCount > 0)
	{
		VESTIGE_LOG(
			this, LogFarm, Log, "{Count} crops withered on field {FieldId} as {Season} began.", WitheredCount, FieldId, Season.ToString());
		OnTilesChanged.Broadcast();
	}
	return WitheredCount;
}

bool UFarmField::ChangeTile(const FIntPoint& Coordinates, TFunctionRef<bool(FFarmTileState&)> Action)
{
	const int32 Index = ToIndex(Coordinates);
	if (Index == INDEX_NONE || !Action(Tiles[Index]))
	{
		return false;
	}
	OnTileChanged.Broadcast(Coordinates);
	return true;
}

void UFarmField::AdvanceDay(int32 Day, int32 MutationSeed)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UFarmField::AdvanceDay);

	for (int32 Index = 0; Index < Tiles.Num(); ++Index)
	{
		FFarmTileState& Tile = Tiles[Index];
		if (Tile.AdvanceDay(WaterRules.NightlyFall))
		{
			RollMutation(Tile, FIntPoint(Index % Size.X, Index / Size.X), Day, MutationSeed);
		}
	}
	OnTilesChanged.Broadcast();
}

void UFarmField::RollMutation(FFarmTileState& Tile, const FIntPoint& Coordinates, int32 Day, int32 MutationSeed)
{
	// A crop rolls once in its life, on the night it ripens, so a ripe crop left in the ground gets no new chances.
	const float Chance = ContaminationRules.GetMutationChance(Tile.GetContamination());
	if (!Tile.GetCrop() || !Tile.GetCrop()->CanMutate() || Chance <= 0.f
		|| !FarmContamination::RollsMutation(MutationSeed, FieldId, Coordinates, Day, Chance) || !Tile.Mutate())
	{
		return;
	}
	VESTIGE_LOG(this, LogFarm, Verbose, "{CropId} at {Tile} on field {FieldId} ripened mutated.", Tile.GetCrop()->GetContentId(),
		Coordinates.ToString(), FieldId);
}

bool UFarmField::SetContamination(const FIntPoint& Coordinates, int32 Level)
{
	return ChangeTile(Coordinates,
		[this, Level](FFarmTileState& Tile)
		{
			return Tile.SetContamination(Level, ContaminationRules.MaxLevel);
		});
}

void UFarmField::ApplyContaminationAreas(TConstArrayView<FFarmContaminationArea> Areas)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UFarmField::ApplyContaminationAreas);

	if (Areas.IsEmpty())
	{
		return;
	}
	for (const FFarmContaminationArea& Area : Areas)
	{
		// Cut to the field; an area with nothing left inside it was set up wrong.
		const FIntPoint First(FMath::Max(Area.Min.X, 0), FMath::Max(Area.Min.Y, 0));
		const FIntPoint Last(FMath::Min(Area.Max.X, Size.X - 1), FMath::Min(Area.Max.Y, Size.Y - 1));
		if (First.X > Last.X || First.Y > Last.Y)
		{
			VESTIGE_LOG(this, LogFarm, Warning,
				"A starting contamination area from {Min} to {Max} has no tile in field {FieldId} of {Size} tiles, so it was skipped.",
				Area.Min.ToString(), Area.Max.ToString(), FieldId, Size.ToString());
			continue;
		}
		for (int32 Y = First.Y; Y <= Last.Y; ++Y)
		{
			for (int32 X = First.X; X <= Last.X; ++X)
			{
				Tiles[ToIndex(FIntPoint(X, Y))].SetContamination(Area.Level, ContaminationRules.MaxLevel);
			}
		}
	}

	const int32 ContaminatedCount = Algo::CountIf(Tiles,
		[](const FFarmTileState& Tile)
		{
			return Tile.IsContaminated();
		});
	VESTIGE_LOG(this, LogFarm, Log, "{Count} tiles of field {FieldId} start contaminated.", ContaminatedCount, FieldId);
	OnTilesChanged.Broadcast();
}

void UFarmField::Resize(const FIntPoint& NewSize)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UFarmField::Resize);

	const FIntPoint OldSize = Size;
	TArray<FFarmTileState> OldTiles = MoveTemp(Tiles);
	Initialize(FieldId, NewSize, WaterRules, ContaminationRules);
	for (int32 Y = 0; Y < FMath::Min(OldSize.Y, Size.Y); ++Y)
	{
		for (int32 X = 0; X < FMath::Min(OldSize.X, Size.X); ++X)
		{
			Tiles[ToIndex(FIntPoint(X, Y))] = OldTiles[Y * OldSize.X + X];
		}
	}
	OnTilesChanged.Broadcast();
}

TArray<FFarmTileSaveData> UFarmField::GetTilesSaveData() const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UFarmField::GetTilesSaveData);

	TArray<FFarmTileSaveData> SavedTiles;
	SavedTiles.Reserve(Tiles.Num());
	for (int32 Index = 0; Index < Tiles.Num(); ++Index)
	{
		const FFarmTileState& Tile = Tiles[Index];
		FFarmTileSaveData& Data = SavedTiles.AddDefaulted_GetRef();
		Data.Coordinates = FIntPoint(Index % Size.X, Index / Size.X);
		Data.State = Tile.GetState();
		Data.WaterLevel = Tile.GetWaterLevel();
		Data.GrowthDays = Tile.GetGrowthDays();
		Data.CropId = Tile.GetCrop() ? Tile.GetCrop()->GetContentId() : NAME_None;
		Data.Contamination = Tile.GetContamination();
		Data.WateredNights = Tile.GetWateredNights();
		Data.bMutated = Tile.IsMutated();
	}
	return SavedTiles;
}

int32 UFarmField::RestoreTilesFromSaveData(const TArray<FFarmTileSaveData>& SavedTiles, const TMap<FName, const UFarmCropData*>& Crops)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UFarmField::RestoreTilesFromSaveData);

	int32 SkippedCount = 0;
	for (const FFarmTileSaveData& Data : SavedTiles)
	{
		const int32 Index = ToIndex(Data.Coordinates);
		if (Index == INDEX_NONE)
		{
			VESTIGE_LOG(this, LogFarm, Warning, "The save has a tile at {Tile} outside field {FieldId}, so it was skipped.",
				Data.Coordinates.ToString(), FieldId);
			++SkippedCount;
			continue;
		}

		const UFarmCropData* SavedCrop = Crops.FindRef(Data.CropId);
		if (!Data.CropId.IsNone() && !SavedCrop)
		{
			VESTIGE_LOG(this, LogFarm, Warning,
				"The save has a crop {CropId} that no longer exists, so tile {Tile} of field {FieldId} is empty.", Data.CropId,
				Data.Coordinates.ToString(), FieldId);
			++SkippedCount;
		}
		Tiles[Index].Restore(Data, SavedCrop, WaterRules.MaxLevel, ContaminationRules.MaxLevel);
	}
	OnTilesChanged.Broadcast();
	return SkippedCount;
}
