// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Tests/VestigeTestTimedDrainEffect.h"

#if WITH_TESTS

UVestigeTestTimedDrainEffect::UVestigeTestTimedDrainEffect()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(10.f));
	Period = FScalableFloat(1.f);
}

#endif
