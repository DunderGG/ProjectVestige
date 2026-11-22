// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "FarmShippingSaveData.generated.h"

/** One item in the shipping bin in a save. */
USTRUCT()
struct FFarmShippedStackSaveData
{
	GENERATED_BODY()

	/** ContentId of the item (Base.Turnip). */
	UPROPERTY()
	FName ItemId;

	UPROPERTY()
	int32 Count = 0;
};

/**
 * UFarmShippingSubsystem's part of a save, under Base.Shipping: what is in the bin until the night, one entry per item. No
 * record is an empty bin.
 */
USTRUCT()
struct FFarmShippingSaveData
{
	GENERATED_BODY()

	/** This layout's version (Systems.md, section 1). Raise it when a change would load older parts wrong. */
	static constexpr int32 Version = 1;

	UPROPERTY()
	TArray<FFarmShippedStackSaveData> Stacks;
};
