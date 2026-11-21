// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Farming/VestigeGameplayAbility_UseTool.h"
#include "FarmGameplayAbility_Water.generated.h"

/**
 * The watering can's behaviour: adds a water step to tilled soil, planted or not, that isn't soaked yet. A held press waters
 * each tile once; a new press adds the next step. Named by DA_Tool_WateringCan.
 */
UCLASS()
class UFarmGameplayAbility_Water : public UVestigeGameplayAbility_UseTool
{
	GENERATED_BODY()

protected:
	virtual bool CanUseOn(const FIntPoint& Tile, FVestigeFeedbackReport* OutFailure) const override;
	virtual bool UseOn(const FIntPoint& Tile) override;

	/** Forgets the tiles this press watered, before the base ends the activation. */
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	/**
	 * The tiles this press has watered. The base repeats a use while the key is held, which would water the faced tile again
	 * after each use; a tile in here is refused (Farm.Soaked, reported only on the press) until a new press.
	 */
	TSet<FIntPoint> TilesWateredThisPress;
};
