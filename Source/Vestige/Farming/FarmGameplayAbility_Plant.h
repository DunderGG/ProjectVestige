// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Farming/VestigeGameplayAbility_UseTool.h"
#include "FarmGameplayAbility_Plant.generated.h"

class UFarmCropData;

/**
 * The seeds' behaviour: plants the seed item in hand (the selected hotbar slot of the inventory on the player state) in empty
 * tilled soil, growing the crop its seed fragment names and taking one seed from that slot. Named by DA_Tool_Seeds, which
 * every seed item's tool fragment names.
 */
UCLASS()
class UFarmGameplayAbility_Plant : public UVestigeGameplayAbility_UseTool
{
	GENERATED_BODY()

protected:
	virtual bool CanUseOn(const FIntPoint& Tile, FGameplayTag* OutFailureReason) const override;
	virtual bool UseOn(const FIntPoint& Tile) override;

private:
	/** The crop the seed item in hand grows, or nullptr if the item in hand isn't a seed or its crop isn't loaded. */
	const UFarmCropData* FindCropInHand() const;
};
