// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Farming/VestigeGameplayAbility_UseTool.h"
#include "FarmGameplayAbility_Harvest.generated.h"

class UFarmCropData;

/**
 * The hand's behaviour: clears a withered crop (nothing is yielded), or harvests a ripe crop into the inventory on the player
 * state, leaving tilled soil, if the bag has room for its yield; a full bag leaves the crop in the ground. A mutated crop gives
 * its mutated item, and a harvest that cleaned its tile says so (Systems.md, section 27). Reports On Crop Harvested. Named by
 * DA_Tool_Hand.
 */
UCLASS()
class UFarmGameplayAbility_Harvest : public UVestigeGameplayAbility_UseTool
{
	GENERATED_BODY()

protected:
	virtual bool CanUseOn(const FIntPoint& Tile, FVestigeFeedbackReport* OutFailure) const override;
	virtual bool UseOn(const FIntPoint& Tile) override;

private:
	/** Whether the inventory has room for Crop's yield of HarvestItemId, the tile's harvest item (the mutated one when mutated). */
	bool HasRoomForHarvest(const FPrimaryAssetId& HarvestItemId, const UFarmCropData& Crop) const;

	/**
	 * When a harvest cleaned its tile, the level fell: reaches the first-cleaning milestone and tells the player, Farm.Cleaned, or
	 * Farm.Cleaned.Clean at 0.
	 */
	void ReportCleaning(int32 LevelBefore, int32 LevelAfter) const;
};
