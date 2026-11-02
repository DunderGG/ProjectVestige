// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "VestigeRestSaveData.generated.h"

/** How the player rested, in a save: written and restored by UVestigeRestComponent. */
USTRUCT()
struct FVestigeRestSaveData
{
	GENERATED_BODY()

	/** Whether the day started exhausted after a late night. False in saves before version 4. */
	UPROPERTY()
	bool bExhausted = false;
};
