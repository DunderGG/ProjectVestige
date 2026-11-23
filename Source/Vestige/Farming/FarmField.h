// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Farming/FarmContamination.h"
#include "Farming/FarmTileState.h"
#include "UObject/Object.h"
#include "FarmField.generated.h"

class UFarmCropData;
struct FVestigeCalendar;

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
	/**
	 * Sets the field up as InSize clean grass tiles that follow InWaterRules (each number lifted to at least 1) and
	 * InContaminationRules (clamped). Called once, by UFarmFieldSubsystem when it makes the field.
	 */
	void Initialize(
		FName InFieldId, const FIntPoint& InSize, const FFarmWaterRules& InWaterRules, const FFarmContaminationRules& InContaminationRules);

	/** The water rules every tile of this field follows. */
	const FFarmWaterRules& GetWaterRules() const { return WaterRules; }

	/** The contamination rules every tile of this field follows: the highest level and the mutation chances. */
	const FFarmContaminationRules& GetContaminationRules() const { return ContaminationRules; }

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
	/** Adds the rules' StepsPerWatering to the tile's water, up to the rules' MaxLevel. */
	bool Water(const FIntPoint& Coordinates);
	bool Harvest(const FIntPoint& Coordinates);
	/** Clears a withered crop, leaving tilled soil with its water. */
	bool ClearWithered(const FIntPoint& Coordinates);

	/**
	 * Withers every unripe planted crop that doesn't grow in Season (UFarmCropData::GrowsIn). Returns how many, logs it when
	 * it isn't 0 and broadcasts OnTilesChanged once when anything withered.
	 */
	int32 WitherCropsOutOfSeason(const FGameplayTag& Season, const FVestigeCalendar& Calendar);

	/**
	 * Starts day count Day on every tile: crops on watered tiles grow by their tile's contamination, then every tile loses the
	 * nightly fall, then each crop that ripened tonight and can mutate rolls FarmContamination::RollsMutation with MutationSeed
	 * and the chance on its tile's level. Broadcasts OnTilesChanged once.
	 */
	void AdvanceDay(int32 Day, int32 MutationSeed);

	/**
	 * Sets the tile's contamination to Level, clamped to the rules' MaxLevel, for developer tools. Returns false, changing
	 * nothing, outside the field or when the level is already that; broadcasts OnTileChanged when it changed.
	 */
	bool SetContamination(const FIntPoint& Coordinates, int32 Level);

	/**
	 * Sets the starting contamination of a new game's field: each area's tiles to its level, clamped to the rules, later areas
	 * over earlier ones. An area partly outside the field is cut to it, one wholly outside is skipped with a warning. Logs how
	 * many tiles start contaminated and broadcasts OnTilesChanged once. Only UFarmFieldSubsystem calls it, when it makes a field.
	 */
	void ApplyContaminationAreas(TConstArrayView<FFarmContaminationArea> Areas);

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

	/** Mutates Tile, whose crop ripened tonight, when its crop can mutate and the roll for its level says so. */
	void RollMutation(FFarmTileState& Tile, const FIntPoint& Coordinates, int32 Day, int32 MutationSeed);

	/** Applies Action to the tile at Coordinates and broadcasts OnTileChanged if it returned true. */
	bool ChangeTile(const FIntPoint& Coordinates, TFunctionRef<bool(FFarmTileState&)> Action);

	/** Which field this is: the ID its grid has, unique across every map (Base.Farm). Saves find it again by it. */
	UPROPERTY(VisibleInstanceOnly, Category = "Farming")
	FName FieldId;

	UPROPERTY(VisibleInstanceOnly, Category = "Farming")
	FIntPoint Size = FIntPoint::ZeroValue;

	/** The numbers of the water rules, copied from the settings when the field was made, so tests can use their own. */
	UPROPERTY(VisibleInstanceOnly, Category = "Farming")
	FFarmWaterRules WaterRules;

	/** The numbers of the contamination rules, copied from the settings when the field was made, so tests can use their own. */
	UPROPERTY(VisibleInstanceOnly, Category = "Farming")
	FFarmContaminationRules ContaminationRules;

	/** Size.X * Size.Y tiles, row by row. A property, so the crops planted in them stay loaded with the field. */
	UPROPERTY(VisibleInstanceOnly, Category = "Farming")
	TArray<FFarmTileState> Tiles;
};
