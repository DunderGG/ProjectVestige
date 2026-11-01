// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "VestigeStaminaCostEffect.generated.h"

/**
 * Spends stamina once: adds the set-by-caller magnitude Vestige.Data.StaminaCost to Stamina, so the caller sets the cost
 * as a negative amount from its data (a tool's StaminaCost). UVestigeStaminaSet keeps the result at 0 or more.
 * A C++ class rather than a Blueprint asset: its shape is fixed and its magnitude always comes from the caller.
 */
UCLASS()
class UVestigeStaminaCostEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UVestigeStaminaCostEffect();
};
