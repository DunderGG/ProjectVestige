// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Farming/VestigeGameplayAbility_UseTool.h"
#include "FarmGameplayAbility_Harvest.generated.h"

class UFarmCropData;

/**
 * The hand's behaviour: harvests a ripe crop into the inventory on the player state, leaving tilled soil, if the bag has
 * room for its yield; a full bag leaves the crop in the ground. Reports On Crop Harvested. Named by DA_Tool_Hand.
 */
UCLASS()
class UFarmGameplayAbility_Harvest : public UVestigeGameplayAbility_UseTool
{
	GENERATED_BODY()

protected:
	virtual bool CanUseOn(const FIntPoint& Tile, FGameplayTag* OutFailureReason) const override;
	virtual bool UseOn(const FIntPoint& Tile) override;

private:
	/** Whether the inventory has room for what Crop yields. */
	bool HasRoomForHarvest(const UFarmCropData& Crop) const;
};
