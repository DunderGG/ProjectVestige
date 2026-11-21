// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Farming/VestigeGameplayAbility_UseTool.h"
#include "FarmGameplayAbility_Water.generated.h"

/**
 * The watering can's behaviour: adds a water step to tilled soil, planted or not, that isn't soaked yet. A held press waters
 * each tile once; a new press adds the next step. Each watering pours one water from the can in hand, and an empty can waters
 * nothing and costs nothing. Named by DA_Tool_WateringCan.
 */
UCLASS()
class UFarmGameplayAbility_Water : public UVestigeGameplayAbility_UseTool
{
	GENERATED_BODY()

protected:
	/** The can in the selected hotbar slot must hold water: otherwise Water.CanEmpty, which the base reports once per press. */
	virtual bool CanUseToolNow(FVestigeFeedbackReport* OutFailure) const override;
	virtual bool CanUseOn(const FIntPoint& Tile, FVestigeFeedbackReport* OutFailure) const override;
	virtual bool UseOn(const FIntPoint& Tile) override;

	/** Forgets the tiles this press watered, before the base ends the activation. */
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	/** Whether the missing-container warning was logged, so a held key warns once rather than on every try. */
	mutable bool bWarnedAboutMissingContainer = false;

	/**
	 * The tiles this press has watered. The base repeats a use while the key is held, which would water the faced tile again
	 * after each use; a tile in here is refused (Farm.Soaked, reported only on the press) until a new press.
	 */
	TSet<FIntPoint> TilesWateredThisPress;
};
