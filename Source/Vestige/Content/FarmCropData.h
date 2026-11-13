// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Content/VestigeContentData.h"
#include "GameplayTagContainer.h"
#include "FarmCropData.generated.h"

class UStaticMesh;

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

#if WITH_EDITOR
	/**
	 * Reports a HarvestItemId that is empty, of another content type, or names no item; a DaysToMature below 1, a StageStartScale
	 * outside (0, 1], a missing ripe mesh, no growth stages, a stage without a mesh, a first StartDay other than 0, start days
	 * that don't strictly increase or reach DaysToMature, and the ripe mesh used by a stage.
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
#endif

protected:
	virtual FPrimaryAssetType GetContentType() const override { return AssetType; }

private:
	/** Seasons the crop grows in. Not used yet: crops grow in any season until seasons exist. */
	UPROPERTY(EditDefaultsOnly, Category = "Farming", meta = (AllowPrivateAccess = "true", Categories = "Vestige.Season"))
	FGameplayTagContainer Seasons;

	/** Watered days the crop needs before it can be harvested. The last growth stage lasts until then. */
	UPROPERTY(EditDefaultsOnly, Category = "Farming|Growth", meta = (AllowPrivateAccess = "true", ClampMin = 1))
	int32 DaysToMature = 3;

	/** Tile water level, from 0 (dry) to 1 (soaked), the crop needs to grow that day. Not used yet: any watered tile grows. */
	UPROPERTY(EditDefaultsOnly, Category = "Farming|Growth", meta = (AllowPrivateAccess = "true", ClampMin = 0, ClampMax = 1))
	float WaterNeed = 0.f;

	/** Highest tile contamination, from 0 (clean) to 1, the crop still grows in. Not used yet: tiles have no contamination. */
	UPROPERTY(EditDefaultsOnly, Category = "Farming|Growth", meta = (AllowPrivateAccess = "true", ClampMin = 0, ClampMax = 1))
	float ContaminationTolerance = 1.f;

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
};
