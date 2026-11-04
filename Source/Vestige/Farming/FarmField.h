// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Farming/FarmTileState.h"
#include "UObject/Object.h"
#include "FarmField.generated.h"

class UFarmCropData;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnFarmFieldTileChanged, const FIntPoint& /* Coordinates */);
DECLARE_MULTICAST_DELEGATE(FOnFarmFieldTilesChanged);

/**
 * One field's tiles as data: a rectangle of FFarmTileState, row by row, and the farming actions on them by grid
 * coordinates (column X, row Y). Owned by UFarmFieldSubsystem, so a field and its crops outlive its map: it grows on every
 * new day whether or not its map is loaded. The level's AFarmGrid is a view of it, and redraws on its events.
 */
UCLASS()
class UFarmField : public UObject
{
	GENERATED_BODY()

public:
	/** Sets the field up as InSize grass tiles. Called once, by UFarmFieldSubsystem when it makes the field. */
	void Initialize(FName InFieldId, const FIntPoint& InSize);

	FName GetFieldId() const { return FieldId; }

	/** Columns (X) and rows (Y). */
	FIntPoint GetSize() const { return Size; }

	bool Contains(const FIntPoint& Coordinates) const;

	/** The tile at Coordinates, or nullptr if they are outside the field. */
	const FFarmTileState* GetTile(const FIntPoint& Coordinates) const;

	// The farming actions follow FFarmTileState's rules, documented there. Each returns false when it changes nothing (or the
	// coordinates are outside the field), and broadcasts OnTileChanged when it changes the tile.
	bool Till(const FIntPoint& Coordinates);
	bool Plant(const FIntPoint& Coordinates, const UFarmCropData* Crop);
	bool Water(const FIntPoint& Coordinates);
	bool Harvest(const FIntPoint& Coordinates);

	/** Starts a new day on every tile: watered crops grow one stage, then the soil dries out. Broadcasts OnTilesChanged. */
	void AdvanceDay();

	/**
	 * Makes the field NewSize (columns X, rows Y), each at least 1. Tiles inside both sizes keep their state, new ones are
	 * grass. Broadcasts OnTilesChanged.
	 */
	void Resize(const FIntPoint& NewSize);

	/** Every tile's saved state with its coordinates, for saving. */
	TArray<FFarmTileSaveData> GetTilesSaveData() const;

	/**
	 * Restores the tiles a save mentions, each found by its coordinates, with its crop looked up in Crops by ContentId.
	 * Tiles the save doesn't mention keep their state. Entries outside the field are skipped, and a crop missing from Crops
	 * leaves its tile as empty tilled soil, each with a warning. Broadcasts OnTilesChanged once. Returns how many entries it
	 * couldn't fully restore.
	 */
	int32 RestoreTilesFromSaveData(const TArray<FFarmTileSaveData>& SavedTiles, const TMap<FName, const UFarmCropData*>& Crops);

	/** Broadcast with a tile's coordinates when an action changed it. */
	FOnFarmFieldTileChanged OnTileChanged;

	/** Broadcast when many tiles may have changed at once: a new day, a restore, a resize. */
	FOnFarmFieldTilesChanged OnTilesChanged;

private:
	/** The index of Coordinates in Tiles, or INDEX_NONE outside the field. */
	int32 ToIndex(const FIntPoint& Coordinates) const;

	/** Applies Action to the tile at Coordinates and broadcasts OnTileChanged if it returned true. */
	bool ChangeTile(const FIntPoint& Coordinates, TFunctionRef<bool(FFarmTileState&)> Action);

	/** Which field this is: the ID its grid has, unique across every map (Base.Farm). Saves find it again by it. */
	UPROPERTY(VisibleInstanceOnly, Category = "Farming")
	FName FieldId;

	UPROPERTY(VisibleInstanceOnly, Category = "Farming")
	FIntPoint Size = FIntPoint::ZeroValue;

	/** Size.X * Size.Y tiles, row by row. A property, so the crops planted in them stay loaded with the field. */
	UPROPERTY(VisibleInstanceOnly, Category = "Farming")
	TArray<FFarmTileState> Tiles;
};
