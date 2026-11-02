// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "VestigeStaminaRefillEffect.generated.h"

/**
 * Sets stamina once, to the amount set by the caller as Vestige.Data.StaminaRefill, kept within 0 to MaxStamina by the
 * stamina set. Applied when the player sleeps, before the new day starts and is saved: MaxStamina after a good night,
 * less while exhausted. A C++ class rather than a Blueprint asset: its shape is fixed and its magnitude comes from the caller.
 */
UCLASS()
class UVestigeStaminaRefillEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UVestigeStaminaRefillEffect();
};
