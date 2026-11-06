// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "VestigeStaminaSaveData.generated.h"

/** The player's stamina in a save, under Base.Stamina, written and restored by UVestigeStaminaSet. */
USTRUCT()
struct FVestigeStaminaSaveData
{
	GENERATED_BODY()

	/** This layout's version (Systems.md, section 1). Raise it when a change would load older parts wrong. */
	static constexpr int32 Version = 1;

	/** False if the player had no stamina set when saving. The player then keeps the stamina it starts with. */
	UPROPERTY()
	bool bHasStamina = false;

	UPROPERTY()
	float Stamina = 0.f;
};
