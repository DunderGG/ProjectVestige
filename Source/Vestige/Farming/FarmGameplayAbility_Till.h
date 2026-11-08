// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Farming/VestigeGameplayAbility_UseTool.h"
#include "FarmGameplayAbility_Till.generated.h"

/**
 * The hoe's behaviour: tills grass into soil that can be planted. Named by DA_Tool_Hoe.
 */
UCLASS()
class UFarmGameplayAbility_Till : public UVestigeGameplayAbility_UseTool
{
	GENERATED_BODY()

protected:
	virtual bool CanUseOn(const FIntPoint& Tile, FGameplayTag* OutFailureReason) const override;
	virtual bool UseOn(const FIntPoint& Tile) override;
};
