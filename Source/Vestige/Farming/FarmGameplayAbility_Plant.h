// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Farming/VestigeGameplayAbility_UseTool.h"
#include "FarmGameplayAbility_Plant.generated.h"

/**
 * The seeds' behaviour: plants the farming component's seed item (UFarmingComponent::GetSeedItemId) in empty tilled soil,
 * taking one seed from the inventory on the player state. Named by DA_Tool_Seeds.
 */
UCLASS()
class UFarmGameplayAbility_Plant : public UVestigeGameplayAbility_UseTool
{
	GENERATED_BODY()

protected:
	virtual bool CanUseOn(const FIntPoint& Tile) const override;
	virtual bool UseOn(const FIntPoint& Tile) override;

private:
	/** Whether the inventory holds a seed of the farming component's seed item, and the item names a crop. */
	bool HasSeedToPlant() const;
};
