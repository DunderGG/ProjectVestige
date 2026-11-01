// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "FarmFarmingSaveData.generated.h"

/**
 * UFarmingComponent's part of a save: plain values and content IDs only. UVestigeSaveGame holds it; the farming component
 * fills it and restores from it.
 */
USTRUCT()
struct FFarmFarmingSaveData
{
	GENERATED_BODY()

	/** ContentId of the tool in hand (Base.Hoe). */
	UPROPERTY()
	FName CurrentToolId;

	// The counts version 1 saves kept here, before seeds and harvests became inventory items. Only the upgrade to version 2
	// reads them; nothing writes them any more.

	UPROPERTY()
	int32 Seeds = 0;

	UPROPERTY()
	int32 HarvestedCrops = 0;
};
