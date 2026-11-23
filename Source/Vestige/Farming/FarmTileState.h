// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "UObject/PrimaryAssetId.h"
#include "FarmTileState.generated.h"

class UFarmCropData;
struct FFarmTileSaveData;

/** What is on a farm tile. Water is separate, because tilled and planted soil can both be wet. */
UENUM(BlueprintType)
enum class EFarmTileState : uint8
{
	Grass,
	Tilled,
	Planted,
	/** Appended last: saves hold the value, so the order never changes. A dead plant that keeps its crop, growth days and water. */
	Withered
};

/**
 * The numbers of the tile water rules, set in the Vestige Farm Fields developer settings and copied into every UFarmField.
 * Each is at least 1 (GetClamped), so a field can always be watered and always dries.
 */
USTRUCT()
struct FFarmWaterRules
{
	GENERATED_BODY()

	/** The most water steps a tile holds. A tile at the cap is soaked and refuses the watering can. */
	UPROPERTY(EditAnywhere, Category = "Water", meta = (ClampMin = 1))
	int32 MaxLevel = 2;

	/** The steps one use of the watering can adds. */
	UPROPERTY(EditAnywhere, Category = "Water", meta = (ClampMin = 1))
	int32 StepsPerWatering = 1;

	/** The steps every tile loses each night, after its crop has grown. */
	UPROPERTY(EditAnywhere, Category = "Water", meta = (ClampMin = 1))
	int32 NightlyFall = 1;

	/** Rules with every number lifted to at least 1. A pure function, so tests don't change the settings. */
	static FFarmWaterRules GetClamped(const FFarmWaterRules& Rules);
};

/**
 * One tile's farming state and the rules that change it: tilling, planting, watering, growing and harvesting.
 * A plain struct, so the rules can be tested without spawning a tile. UFarmField holds one per tile; the grid's UFarmFieldMeshesComponent draws it.
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

	/**
	 * Adds Steps water steps to tilled or planted soil, up to MaxLevel. Returns false, changing nothing, if Steps is less
	 * than 1, the tile is grass or it already holds MaxLevel.
	 */
	bool AddWater(int32 Steps, int32 MaxLevel);

	/**
	 * Removes a ripe crop, leaving tilled soil, and lowers the contamination by the crop's ContaminationCleanedPerHarvest, not
	 * below 0. Returns false, changing nothing, if nothing ripe grows here.
	 */
	bool Harvest();

	/** Turns an unripe planted crop into a withered one. Returns false, changing nothing, if CanWither is false. */
	bool Wither();

	/** Clears a withered crop, leaving empty tilled soil with its water. Returns false, changing nothing, if the tile isn't withered. */
	bool ClearWithered();

	/**
	 * Called once per in-game day. An unripe crop on a tile with any water counts a watered night, and grows one growth day
	 * when it has counted its crop's watered nights per growth day on the tile's contamination (none where that is 0); then the
	 * tile loses NightlyFall steps. Returns true when the crop became ripe tonight.
	 */
	bool AdvanceDay(int32 NightlyFall);

	/**
	 * Sets the contamination to Level, clamped to 0..MaxLevel, whatever is on the tile. Returns false, changing nothing, when the
	 * clamped level is the tile's already. Only UFarmField calls it: a starting layout and developer tools.
	 */
	bool SetContamination(int32 Level, int32 MaxLevel);

	/** Makes a ripe crop that can mutate a mutated one. Returns false, changing nothing, on any other tile or one already mutated. */
	bool Mutate();

	/**
	 * Puts the tile straight into Saved's state, without replaying any action. A planted or withered state without SavedCrop
	 * restores as empty tilled soil, watered grass as dry grass; the water level is clamped to 0..MaxWaterLevel, contamination
	 * to 0..MaxContaminationLevel (grass included), growth to what the crop can reach, the watered nights to below the crop's
	 * interval on the restored level (0 without a growing crop), and the mutation is kept only on a ripe crop that can mutate.
	 */
	void Restore(const FFarmTileSaveData& Saved, const UFarmCropData* SavedCrop, int32 MaxWaterLevel, int32 MaxContaminationLevel);

	EFarmTileState GetState() const { return State; }

	/** The water steps the soil holds, from 0 (dry). */
	int32 GetWaterLevel() const { return WaterLevel; }
	bool IsWatered() const { return WaterLevel > 0; }
	bool IsRipe() const;
	bool IsWithered() const { return State == EFarmTileState::Withered; }

	/** Whether Wither would change the tile: a planted crop that isn't ripe (ripe crops don't wither). */
	bool CanWither() const { return State == EFarmTileState::Planted && !IsRipe(); }

	/** Whether ClearWithered would change the tile now. */
	bool CanClearWithered() const { return IsWithered(); }

	// Whether the action would change the tile now, without doing it. Harvesting can when IsRipe.
	bool CanTill() const { return State == EFarmTileState::Grass; }
	bool CanPlant() const { return State == EFarmTileState::Tilled; }
	bool CanAddWater(int32 MaxLevel) const { return State != EFarmTileState::Grass && WaterLevel < MaxLevel; }
	int32 GetGrowthDays() const { return GrowthDays; }

	/** The crop growing here, or nullptr if nothing is planted. */
	const UFarmCropData* GetCrop() const { return Crop; }

	/** The contamination level, from 0 (clean) to the field's MaxLevel. Kept whatever is on the tile. */
	int32 GetContamination() const { return Contamination; }
	bool IsContaminated() const { return Contamination > 0; }

	/** The watered nights counted toward the crop's next growth day, 0 on a tile without a growing crop. */
	int32 GetWateredNights() const { return WateredNights; }

	/** Whether the ripe crop here ripened mutated. Only ever true on a ripe planted tile. */
	bool IsMutated() const { return bMutated; }

	/** The item a harvest here gives: the crop's mutated item when mutated, else its harvest item. Invalid without a crop. */
	FPrimaryAssetId GetHarvestItemId() const;

private:
	UPROPERTY(VisibleInstanceOnly, Category = "Farming")
	EFarmTileState State = EFarmTileState::Grass;

	UPROPERTY(VisibleInstanceOnly, Category = "Farming")
	int32 WaterLevel = 0;

	UPROPERTY(VisibleInstanceOnly, Category = "Farming")
	int32 GrowthDays = 0;

	/** Set exactly while the tile is planted or withered. Whoever loads the crop also loads its meshes. */
	UPROPERTY(VisibleInstanceOnly, Category = "Farming")
	TObjectPtr<const UFarmCropData> Crop;

	UPROPERTY(VisibleInstanceOnly, Category = "Farming")
	int32 Contamination = 0;

	UPROPERTY(VisibleInstanceOnly, Category = "Farming")
	int32 WateredNights = 0;

	UPROPERTY(VisibleInstanceOnly, Category = "Farming")
	bool bMutated = false;
};

/**
 * One tile's part of a save: plain values and content IDs only. UFarmField fills one per tile, with where the tile is,
 * and its FFarmFieldSaveData holds the list.
 */
USTRUCT()
struct FFarmTileSaveData
{
	GENERATED_BODY()

	/** The tile's column (X) and row (Y) in the grid. */
	UPROPERTY()
	FIntPoint Coordinates = FIntPoint::ZeroValue;

	UPROPERTY()
	EFarmTileState State = EFarmTileState::Grass;

	/** Absent in saves from before tiles held a water level, which load dry. */
	UPROPERTY()
	int32 WaterLevel = 0;

	UPROPERTY()
	int32 GrowthDays = 0;

	/** ContentId of the crop growing here (Base.Turnip), or None. */
	UPROPERTY()
	FName CropId;

	/** Absent in saves from before tiles held contamination, which load clean. */
	UPROPERTY()
	int32 Contamination = 0;

	/** The watered nights toward the next growth day. Absent in older saves, which load with none. */
	UPROPERTY()
	int32 WateredNights = 0;

	/** Whether the ripe crop ripened mutated. Absent in older saves, which load with no mutation. */
	UPROPERTY()
	bool bMutated = false;
};
