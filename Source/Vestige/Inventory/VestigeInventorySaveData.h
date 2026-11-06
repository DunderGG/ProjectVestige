// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "VestigeInventorySaveData.generated.h"

/** One non-empty inventory slot in a save. */
USTRUCT()
struct FVestigeInventorySlotSaveData
{
	GENERATED_BODY()

	/** The slot's position, from 0. */
	UPROPERTY()
	int32 Slot = INDEX_NONE;

	/** ContentId of the item stacked there (Base.Turnip). */
	UPROPERTY()
	FName ItemId;

	UPROPERTY()
	int32 Count = 0;
};

/**
 * UVestigeInventoryComponent's part of a save, under Base.Inventory: plain values and content IDs only. The inventory fills
 * it and restores from it.
 */
USTRUCT()
struct FVestigeInventorySaveData
{
	GENERATED_BODY()

	/** This layout's version (Systems.md, section 1). Raise it when a change would load older parts wrong. */
	static constexpr int32 Version = 1;

	/** The non-empty slots, in slot order. */
	UPROPERTY()
	TArray<FVestigeInventorySlotSaveData> Slots;
};
