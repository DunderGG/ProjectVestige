// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "FarmTileState.generated.h"

class UFarmCropData;

/** What is on a farm tile. Watering is separate, because tilled and planted soil can both be wet. */
UENUM(BlueprintType)
enum class EFarmTileState : uint8
{
	Grass,
	Tilled,
	Planted
};

/**
 * One tile's farming state and the rules that change it: tilling, planting, watering, growing and harvesting.
 * A plain struct, so the rules can be tested without spawning a tile. AFarmTile owns one and shows it.
 * Every change goes through a function that checks it, so impossible states (a planted tile without a crop) can't happen.
 * Its ScriptName keeps it apart from EFarmTileState in Python, which drops both prefixes.
 */
USTRUCT(meta = (ScriptName = "FarmTileStateStruct"))
struct FFarmTileState
{
	GENERATED_BODY()

public:
	/** Turns grass into tilled soil. Returns false, changing nothing, if the tile isn't grass. */
	bool Till();

	/** Plants InCrop in tilled soil. Returns false, changing nothing, if the soil isn't empty and tilled or InCrop is null. */
	bool Plant(const UFarmCropData* InCrop);

	/** Waters tilled or planted soil until the next day. Returns false if the tile is grass or already watered. */
	bool Water();

	/** Removes a ripe crop, leaving tilled soil. Returns false, changing nothing, if nothing ripe grows here. */
	bool Harvest();

	/** Called once per in-game day. A watered crop grows one stage, then the soil dries out. */
	void AdvanceDay();

	/**
	 * Puts the tile straight into a saved state, without replaying any action. A planted state without a crop restores
	 * as empty tilled soil, watered grass as dry grass, and growth is clamped to what the crop can reach.
	 */
	void Restore(EFarmTileState SavedState, bool bSavedWatered, int32 SavedGrowthDays, const UFarmCropData* SavedCrop);

	EFarmTileState GetState() const { return State; }
	bool IsWatered() const { return bWatered; }
	bool IsRipe() const;

	// Whether the action would change the tile now, without doing it. Harvesting can when IsRipe.
	bool CanTill() const { return State == EFarmTileState::Grass; }
	bool CanPlant() const { return State == EFarmTileState::Tilled; }
	bool CanWater() const { return State != EFarmTileState::Grass && !bWatered; }
	int32 GetGrowthDays() const { return GrowthDays; }

	/** The crop growing here, or nullptr if nothing is planted. */
	const UFarmCropData* GetCrop() const { return Crop; }

private:
	UPROPERTY(VisibleInstanceOnly, Category = "Farming")
	EFarmTileState State = EFarmTileState::Grass;

	UPROPERTY(VisibleInstanceOnly, Category = "Farming")
	bool bWatered = false;

	UPROPERTY(VisibleInstanceOnly, Category = "Farming")
	int32 GrowthDays = 0;

	/** Set exactly while the tile is planted. Whoever loads the crop also loads its meshes. */
	UPROPERTY(VisibleInstanceOnly, Category = "Farming")
	TObjectPtr<const UFarmCropData> Crop;
};

/**
 * One tile's part of a save: plain values and content IDs only. AFarmTile fills the state and crop, AFarmGrid adds where
 * the tile is, and UVestigeSaveGame holds the list.
 */
USTRUCT()
struct FFarmTileSaveData
{
	GENERATED_BODY()

	/** The tile's column (X) and row (Y) in the grid. Version 0 saves have none; the upgrade adds them. */
	UPROPERTY()
	FIntPoint Coordinates = FIntPoint::ZeroValue;

	UPROPERTY()
	EFarmTileState State = EFarmTileState::Grass;

	UPROPERTY()
	bool bWatered = false;

	UPROPERTY()
	int32 GrowthDays = 0;

	/** ContentId of the crop growing here (Base.Turnip), or None. */
	UPROPERTY()
	FName CropId;
};
