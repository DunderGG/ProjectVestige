// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "FarmFarmingSaveData.generated.h"

/**
 * UFarmingComponent's part of a save, under Base.Farming: plain values and content IDs only. The farming component fills
 * it and restores from it.
 */
USTRUCT()
struct FFarmFarmingSaveData
{
	GENERATED_BODY()

	/** This layout's version (Systems.md, section 1). Raise it when a change would load older parts wrong. */
	static constexpr int32 Version = 1;

	/** ContentId of the tool in hand (Base.Hoe). */
	UPROPERTY()
	FName CurrentToolId;
};
