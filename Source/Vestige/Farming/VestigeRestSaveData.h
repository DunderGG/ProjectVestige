// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "VestigeRestSaveData.generated.h"

/** How the player rested, in a save under Base.Rest: written and restored by UVestigeRestComponent. */
USTRUCT()
struct FVestigeRestSaveData
{
	GENERATED_BODY()

	/** This layout's version (Systems.md, section 1). Raise it when a change would load older parts wrong. */
	static constexpr int32 Version = 1;

	/** Whether the day started exhausted after a late night. */
	UPROPERTY()
	bool bExhausted = false;
};
