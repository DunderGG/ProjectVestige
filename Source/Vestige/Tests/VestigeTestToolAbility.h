// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

// Only in builds with tests, like UVestigeTestEventRecorder.
#if WITH_TESTS

#include "CoreMinimal.h"
#include "Farming/VestigeGameplayAbility_UseTool.h"
#include "VestigeTestToolAbility.generated.h"

/**
 * A tool behaviour no base tool has, for tests: tills grass and waters it in the same use. Shows that a new tool is a new
 * ability, with no change to the farming component.
 */
UCLASS(NotBlueprintable, HideDropdown)
class UVestigeTestToolAbility : public UVestigeGameplayAbility_UseTool
{
	GENERATED_BODY()

protected:
	virtual bool CanUseOn(const FIntPoint& Tile, FGameplayTag* OutFailureReason) const override;
	virtual bool UseOn(const FIntPoint& Tile) override;
};

#endif
