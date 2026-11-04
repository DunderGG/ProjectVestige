// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Farming/FarmField.h"

#include "Content/FarmCropData.h"
#include "Farming/FarmLog.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

void UFarmField::Initialize(FName InFieldId, const FIntPoint& InSize)
{
	FieldId = InFieldId;
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
		[](FFarmTileState& Tile)
		{
			return Tile.Water();
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

void UFarmField::AdvanceDay()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UFarmField::AdvanceDay);

	for (FFarmTileState& Tile : Tiles)
	{
		Tile.AdvanceDay();
	}
	OnTilesChanged.Broadcast();
}

void UFarmField::Resize(const FIntPoint& NewSize)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UFarmField::Resize);

	const FIntPoint OldSize = Size;
	TArray<FFarmTileState> OldTiles = MoveTemp(Tiles);
	Initialize(FieldId, NewSize);
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
		Data.bWatered = Tile.IsWatered();
		Data.GrowthDays = Tile.GetGrowthDays();
		Data.CropId = Tile.GetCrop() ? Tile.GetCrop()->GetContentId() : NAME_None;
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
		Tiles[Index].Restore(Data.State, Data.bWatered, Data.GrowthDays, SavedCrop);
	}
	OnTilesChanged.Broadcast();
	return SkippedCount;
}
