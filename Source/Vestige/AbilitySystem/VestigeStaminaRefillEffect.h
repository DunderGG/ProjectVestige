// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "VestigeStaminaRefillEffect.generated.h"

/**
 * Refills stamina once, to MaxStamina. Applied when the player sleeps, before the new day starts and is saved.
 * A C++ class rather than a Blueprint asset: its shape is fixed and its magnitude is the target's own MaxStamina.
 */
UCLASS()
class UVestigeStaminaRefillEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UVestigeStaminaRefillEffect();
};
