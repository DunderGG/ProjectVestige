// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "FarmWaterSaveData.generated.h"

/**
 * UFarmWaterSubsystem's part of a save, under Base.FarmWater: which day the farm's clean water was last drawn from and how
 * much was drawn that day. The daily supply is not saved, so a supply tuned since applies at once. No record is a full store.
 */
USTRUCT()
struct FFarmWaterSaveData
{
	GENERATED_BODY()

	/** This layout's version (Systems.md, section 1). Raise it when a change would load older parts wrong. */
	static constexpr int32 Version = 1;

	/** The day water was last drawn on, or INDEX_NONE before anything was drawn. */
	UPROPERTY()
	int32 Day = INDEX_NONE;

	/** How much clean water was drawn on that day. */
	UPROPERTY()
	int32 CleanWaterDrawn = 0;
};
