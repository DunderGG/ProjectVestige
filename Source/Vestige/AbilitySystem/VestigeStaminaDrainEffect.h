// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "VestigeStaminaDrainEffect.generated.h"

/**
 * Drains stamina steadily until it is removed: every PeriodSeconds of real time it takes the amount set by the caller as
 * Vestige.Data.StaminaDrain, kept at 0 or more by the stamina set. The rest component applies it at midnight and
 * removes it when the next day starts. A C++ class rather than a Blueprint asset: its shape is fixed and its magnitude
 * comes from the caller, who turns a rate per game hour into an amount per period.
 */
UCLASS()
class UVestigeStaminaDrainEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	/** Real seconds between two drains. Short enough for the HUD's number to count down steadily. */
	static constexpr float PeriodSeconds = 1.f;

	UVestigeStaminaDrainEffect();
};
