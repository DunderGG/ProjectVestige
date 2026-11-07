// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Farming/VestigeGameplayAbility_UseTool.h"
#include "FarmGameplayAbility_Water.generated.h"

/**
 * The watering can's behaviour: waters tilled soil, planted or not, that isn't watered yet. Named by DA_Tool_WateringCan.
 */
UCLASS()
class UFarmGameplayAbility_Water : public UVestigeGameplayAbility_UseTool
{
	GENERATED_BODY()

protected:
	virtual bool CanUseOn(const FIntPoint& Tile) const override;
	virtual bool UseOn(const FIntPoint& Tile) override;
};
