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
 *
 * Seeds go in only in their crop's seasons (UFarmCropData::GetSeasonFit, with today's date from the clock). A planting too late
 * for the crop to ripen before the season ends is refused once a day per crop with a warning, which the farming component
 * remembers; a later press that day plants it (Systems.md, section 22). Seeds are refused on a tile whose contamination their
 * crop can't be planted or grow on, and the too-late check counts the tile's slower growth (Systems.md, section 27).
 */
UCLASS()
class UFarmGameplayAbility_Plant : public UVestigeGameplayAbility_UseTool
{
	GENERATED_BODY()

protected:
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/**
	 * Reports, the first that applies: no seed in hand, out of season, no tile, a withered crop, a tile that can't take seeds,
	 * a tile below the crop's MinPlantingContamination, a tile whose contamination the crop doesn't grow on, then too late at
	 * the tile's growth rate unless the player was warned on an earlier press today.
	 */
	virtual bool CanUseOn(const FIntPoint& Tile, FVestigeFeedbackReport* OutFailure) const override;

	virtual bool UseOn(const FIntPoint& Tile) override;

	/** A too-late warning: the farming component remembers it for the crop and the day, and the rest of this press plants nothing. */
	virtual void OnPressRefused(const FIntPoint& Tile, const FVestigeFeedbackReport& Refusal) override;

private:
	/** The crop the seed item in hand grows, or nullptr if the item in hand isn't a seed or its crop isn't loaded. */
	const UFarmCropData* FindCropInHand() const;

	/**
	 * Whether the player chose to plant Crop too late on day count Day: warned about it on an earlier press that day, not on
	 * this one, which they may not have read yet.
	 */
	bool IsTooLatePlantingConfirmed(const UFarmCropData& Crop, int32 Day) const;

	/** Set when this press warned that its planting is too late, so a held key can't plant past the warning. Cleared on end. */
	bool bWarnedThisPress = false;
};
