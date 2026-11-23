// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Content/VestigeContentData.h"
#include "GameplayTagContainer.h"
#include "FarmCropData.generated.h"

class UStaticMesh;
struct FVestigeCalendar;

/** Whether planting a crop on a day fits its seasons. A fixed set of rule outcomes (Systems.md, section 22). */
enum class EFarmSeasonFit : uint8
{
	/** The crop grows in the day's season and can ripen before a season it doesn't grow in begins. */
	Fits,

	/** The crop doesn't grow in the day's season. */
	OutOfSeason,

	/** The crop grows in the day's season, but needs more days to ripen than are left before a season it doesn't grow in. */
	TooLate
};

/**
 * The answer of UFarmCropData::GetSeasonFit for planting a crop on one day, with the numbers a report about it names. Never
 * saved: it is worked out when asked.
 */
struct FFarmSeasonFit
{
	EFarmSeasonFit Fit = EFarmSeasonFit::Fits;

	/** The watered nights the crop needs to ripen: its DaysToMature times the tile's watered nights per growth day. */
	int32 DaysToRipe = 0;

	/**
	 * The days the crop has before it would wither: the day's DaysLeft plus the days of each following season in a row that the
	 * crop grows in. MAX_int32 when no coming season ends its growth, and while it is out of season.
	 */
	int32 DaysToDeadline = MAX_int32;

	/** Out of season: the index in the calendar's seasons of the next season the crop grows in. Else INDEX_NONE. */
	int32 PlantSeasonIndex = INDEX_NONE;

	/** The index in the calendar's seasons of the first coming season the crop doesn't grow in, or INDEX_NONE without one. */
	int32 DeadlineSeasonIndex = INDEX_NONE;

	/**
	 * The values a feedback report about this fit on day count Day gives its text: the calendar's GetReportValues(Day),
	 * DaysToRipe, and out of season PlantSeason and PlantSeasonInSentence, too late DaysToDeadline, DeadlineSeason and
	 * DeadlineSeasonInSentence (the seasons' DisplayName and NameInSentence). A season whose index is INDEX_NONE gives no value.
	 */
	FFormatNamedArguments GetReportValues(const FVestigeCalendar& Calendar, int32 Day) const;
};

/** How a crop looks on one growth day: one mesh at a uniform scale, standing on its origin at the base. */
struct FFarmCropGrowthLook
{
	/** The mesh, or nullptr when the crop has none for that day or it isn't loaded; the tile then draws its placeholder. */
	UStaticMesh* Mesh = nullptr;

	/** Uniform scale of Mesh, from the crop's StageStartScale on a stage's first day to 1 on its last. */
	float Scale = 1.f;
};

/**
 * One growth stage of a crop: the growth day it starts on and the mesh it shows until the next stage starts. BlueprintType only so
 * editor scripts can make and set it; Blueprints don't read it.
 */
USTRUCT(BlueprintType)
struct FFarmCropGrowthStage
{
	GENERATED_BODY()

	/** The watered growth day the stage starts on: 0 for the seedling on the day planted, then strictly later for each stage. */
	UPROPERTY(EditAnywhere, Category = "Farming", meta = (ClampMin = 0))
	int32 StartDay = 0;

	/**
	 * The mesh the crop shows from StartDay until the next stage starts. Origin at the center of the base on the soil surface; at
	 * least 1 / StageStartScale times the previous stage's mesh in height and spread.
	 */
	UPROPERTY(EditAnywhere, Category = "Farming")
	TSoftObjectPtr<UStaticMesh> Mesh;
};

/**
 * The definition of one crop: how it grows, how it looks and what its harvest gives, plus the ID and name every content asset has.
 * One asset per crop, found through the Asset Manager by its ContentId. Holds no runtime state; tiles do.
 */
UCLASS()
class UFarmCropData : public UVestigeContentData
{
	GENERATED_BODY()

public:
	/** The primary asset type crops are registered under in Config/DefaultGame.ini. */
	static const FPrimaryAssetType AssetType;

	int32 GetDaysToMature() const { return DaysToMature; }
	int32 GetYield() const { return Yield; }

	/** Scale of a growth stage mesh on its stage's first day, from above 0 to 1. */
	float GetStageStartScale() const { return StageStartScale; }

	/** The item a harvest gives (VestigeItem:Base.Turnip). */
	const FPrimaryAssetId& GetHarvestItemId() const { return HarvestItemId; }

	/**
	 * How many watered nights make one growth day on a tile at contamination Level, 0 when the crop doesn't grow there. A level
	 * past the end of WateredNightsPerGrowthDay uses its last entry; an empty list gives 1, so broken data grows as on clean soil.
	 */
	int32 GetWateredNightsPerGrowthDay(int32 Level) const;

	/** The lowest contamination level its seeds may be planted on. */
	int32 GetMinPlantingContamination() const { return MinPlantingContamination; }

	/** The contamination levels one harvest of the crop takes off its tile, not below clean. */
	int32 GetContaminationCleanedPerHarvest() const { return ContaminationCleanedPerHarvest; }

	/** The item a mutated harvest gives, Yield times, or an invalid ID for a crop that never mutates. */
	const FPrimaryAssetId& GetMutatedHarvestItemId() const { return MutatedHarvestItemId; }

	/** Whether the crop can ripen mutated: it has a mutated harvest item. */
	bool CanMutate() const { return MutatedHarvestItemId.IsValid(); }

	/** The mesh of the mutated ripe crop: MutatedRipeMesh, or the ripe mesh without one. Nullptr while not loaded; never loads. */
	UStaticMesh* GetMutatedRipeMesh() const;

	/** Whether a crop that has grown for GrowthDays watered days can be harvested. A DaysToMature below 1 is always ripe. */
	bool IsRipeAt(int32 GrowthDays) const { return GrowthDays >= DaysToMature; }

	/**
	 * Which of GrowthStages a crop that has grown for GrowthDays days shows: the last one whose StartDay is at most GrowthDays,
	 * or the first if none is. INDEX_NONE once it is ripe, or when the crop has no growth stages.
	 */
	int32 GetGrowthStageIndex(int32 GrowthDays) const;

	/**
	 * How a crop that has grown for GrowthDays days looks: its ripe mesh at full size once ripe, before that its growth stage
	 * mesh, scaled up evenly within the stage. The mesh is nullptr when it is missing or not loaded. Never loads an asset.
	 */
	FFarmCropGrowthLook GetGrowthLook(int32 GrowthDays) const;

	/**
	 * Whether the crop grows in Season, a season of Calendar: when its Seasons lists that tag exactly, or lists none of the
	 * calendar's seasons (a crop of every season). A listed child of a season tag doesn't count.
	 */
	bool GrowsIn(const FGameplayTag& Season, const FVestigeCalendar& Calendar) const;

	/**
	 * Whether planting the crop on day count Day fits its seasons in Calendar: out of season when it doesn't grow in the day's
	 * season, too late when it needs more days to ripen than it has before a coming season it doesn't grow in. Every crop fits
	 * in a calendar without seasons. Assumes a watering every night, and that NightsPerGrowthDay of them make one growth
	 * day (the tile's contamination, GetWateredNightsPerGrowthDay; at least 1). Pure: reads no clock or world.
	 */
	FFarmSeasonFit GetSeasonFit(const FVestigeCalendar& Calendar, int32 Day, int32 NightsPerGrowthDay = 1) const;

#if WITH_EDITOR
	/**
	 * Reports a HarvestItemId that is empty, of another content type, or names no item; no Seasons, and a season the game
	 * data's calendar doesn't have (skipped when that calendar has no seasons); a DaysToMature below 1, a StageStartScale outside
	 * (0, 1], a missing ripe mesh, no growth stages, a stage without a mesh, a first StartDay other than 0, start days that don't
	 * strictly increase or reach DaysToMature, and the ripe mesh used by a stage. For contamination: an empty or negative
	 * WateredNightsPerGrowthDay, a crop that grows on no level from MinPlantingContamination up, a MutatedHarvestItemId of another
	 * type or naming no item, and a MutatedRipeMesh used by a growth stage.
	 */
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

#if WITH_DEV_AUTOMATION_TESTS
	/** For tests only. Crops get their harvest item in the editor. */
	void SetHarvestItemIdForTests(const FPrimaryAssetId& InHarvestItemId) { HarvestItemId = InHarvestItemId; }

	/** For tests only. Crops get their growth time in the editor. */
	void SetDaysToMatureForTests(int32 InDaysToMature) { DaysToMature = InDaysToMature; }

	/** For tests only. Crops get their growth stages and meshes in the editor. */
	void SetGrowthStagesForTests(const TArray<FFarmCropGrowthStage>& InGrowthStages, const TSoftObjectPtr<UStaticMesh>& InRipeMesh)
	{
		GrowthStages = InGrowthStages;
		RipeMesh = InRipeMesh;
	}

	/** For tests only. Crops get their stage start scale in the editor. */
	void SetStageStartScaleForTests(float InStageStartScale) { StageStartScale = InStageStartScale; }

	/** For tests only. Crops get their seasons in the editor. */
	void SetSeasonsForTests(const FGameplayTagContainer& InSeasons) { Seasons = InSeasons; }

	/** For tests only. Crops get their response to contamination in the editor. */
	void SetContaminationResponseForTests(
		const TArray<int32>& InWateredNightsPerGrowthDay, int32 InMinPlantingContamination, int32 InContaminationCleanedPerHarvest)
	{
		WateredNightsPerGrowthDay = InWateredNightsPerGrowthDay;
		MinPlantingContamination = InMinPlantingContamination;
		ContaminationCleanedPerHarvest = InContaminationCleanedPerHarvest;
	}

	/** For tests only. Crops get their mutated form in the editor. */
	void SetMutationForTests(const FPrimaryAssetId& InMutatedHarvestItemId, const TSoftObjectPtr<UStaticMesh>& InMutatedRipeMesh)
	{
		MutatedHarvestItemId = InMutatedHarvestItemId;
		MutatedRipeMesh = InMutatedRipeMesh;
	}
#endif

protected:
	virtual FPrimaryAssetType GetContentType() const override { return AssetType; }

private:
	/** Whether Seasons lists any of Calendar's seasons exactly. A crop that lists none grows in every season. */
	bool ListsAnySeasonOf(const FVestigeCalendar& Calendar) const;

#if WITH_EDITOR
	/** IsDataValid's checks of the contamination response and the mutated form, each error passed to ReportError. */
	void ReportContaminationErrors(TFunctionRef<void(const FText&)> ReportError) const;
#endif

	/**
	 * Seasons the crop grows in, tags of the calendar's seasons (DA_GameData), matched exactly. Seeds are planted only in these,
	 * and a crop listing none of the calendar's seasons grows in every season (data validation reports an empty list).
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Farming", meta = (AllowPrivateAccess = "true", Categories = "Vestige.Season"))
	FGameplayTagContainer Seasons;

	/** Watered days the crop needs before it can be harvested. The last growth stage lasts until then. */
	UPROPERTY(EditDefaultsOnly, Category = "Farming|Growth", meta = (AllowPrivateAccess = "true", ClampMin = 1))
	int32 DaysToMature = 3;

	/**
	 * The stages the crop shows while it grows, seedling first, without the ripe mesh: each from its StartDay until the next one
	 * starts, the last until DaysToMature. The first starts on day 0, each later one on a later day before DaysToMature, chosen so
	 * each look lasts as long as it should. A stage without a mesh draws the tile's placeholder shape.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Farming|Visuals",
		meta = (AllowPrivateAccess = "true", AssetBundles = "Visuals", TitleProperty = "StartDay"))
	TArray<FFarmCropGrowthStage> GrowthStages;

	/** The mesh the crop shows once it can be harvested, at full size, and never before. Origin as for the growth stage meshes. */
	UPROPERTY(EditDefaultsOnly, Category = "Farming|Visuals", meta = (AllowPrivateAccess = "true", AssetBundles = "Visuals"))
	TSoftObjectPtr<UStaticMesh> RipeMesh;

	/**
	 * Scale of a growth stage mesh on the stage's first day. The crop grows evenly to full size on the stage's last day, so it
	 * changes every night it grew; a one-day stage shows at full size.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Farming|Visuals", meta = (AllowPrivateAccess = "true", ClampMin = 0.1, ClampMax = 1))
	float StageStartScale = 0.8f;

	/** The item one harvest gives, Yield times. Prices belong to that item. */
	UPROPERTY(EditDefaultsOnly, Category = "Farming|Harvest", meta = (AllowPrivateAccess = "true", AllowedTypes = "VestigeItem"))
	FPrimaryAssetId HarvestItemId;

	/** How many of the harvest item one harvest gives. */
	UPROPERTY(EditDefaultsOnly, Category = "Farming|Harvest", meta = (AllowPrivateAccess = "true", ClampMin = 1))
	int32 Yield = 1;

	/**
	 * How many watered nights make one growth day on each contamination level (index = level: clean, light, heavy), 0 where the
	 * crop doesn't grow (it doesn't die either). A level past the end uses the last entry (Systems.md, section 27).
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Farming|Contamination", meta = (AllowPrivateAccess = "true", ClampMin = 0))
	TArray<int32> WateredNightsPerGrowthDay = {1, 2, 0};

	/** The lowest contamination level the crop's seeds may be planted on; below it they are refused. */
	UPROPERTY(EditDefaultsOnly, Category = "Farming|Contamination", meta = (AllowPrivateAccess = "true", ClampMin = 0))
	int32 MinPlantingContamination = 0;

	/** The contamination levels one harvest takes off the crop's tile, not below clean. 0 for a crop that doesn't clean. */
	UPROPERTY(EditDefaultsOnly, Category = "Farming|Contamination", meta = (AllowPrivateAccess = "true", ClampMin = 0))
	int32 ContaminationCleanedPerHarvest = 0;

	/**
	 * The item a mutated harvest gives, Yield times. Empty means the crop never mutates. Whether a crop ripens mutated is the
	 * field settings' chance on its tile's level.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Farming|Contamination", meta = (AllowPrivateAccess = "true", AllowedTypes = "VestigeItem"))
	FPrimaryAssetId MutatedHarvestItemId;

	/** The mesh a mutated ripe crop shows, with the mutated material. Empty uses the ripe mesh. Origin as for the ripe mesh. */
	UPROPERTY(EditDefaultsOnly, Category = "Farming|Contamination", meta = (AllowPrivateAccess = "true", AssetBundles = "Visuals"))
	TSoftObjectPtr<UStaticMesh> MutatedRipeMesh;
};
