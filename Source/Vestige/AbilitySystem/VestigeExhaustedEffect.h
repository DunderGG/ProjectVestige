// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "VestigeExhaustedEffect.generated.h"

/**
 * Being exhausted after a late night: grants Vestige.State.Exhausted until it is removed, and has no modifiers of its
 * own. The rest component applies it when the player goes to bed late and removes it after a night in bed before
 * midnight. One level only: the rest component applies it only while the player isn't exhausted.
 * A C++ class rather than a Blueprint asset: its shape is fixed and it has no numbers to tune.
 */
UCLASS()
class UVestigeExhaustedEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UVestigeExhaustedEffect();
};
