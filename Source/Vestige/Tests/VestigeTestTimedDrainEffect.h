// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

// Only in builds with tests, like UVestigeTestEventRecorder.
#if WITH_TESTS

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "VestigeTestTimedDrainEffect.generated.h"

/** A periodic effect with a duration, for tests: a late-night drain that isn't instant. */
UCLASS(NotBlueprintable, HideDropdown)
class UVestigeTestTimedDrainEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UVestigeTestTimedDrainEffect();
};

#endif
