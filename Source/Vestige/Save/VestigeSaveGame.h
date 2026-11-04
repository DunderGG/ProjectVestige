// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/VestigeStaminaSaveData.h"
#include "Farming/VestigeRestSaveData.h"
#include "Farming/FarmFarmingSaveData.h"
#include "Farming/FarmTileState.h"
#include "GameFramework/SaveGame.h"
#include "Inventory/VestigeInventorySaveData.h"
#include "Time/VestigeClockSaveData.h"
#include "VestigeSaveGame.generated.h"

/**
 * The saved data layout: everything needed to pick the farm back up, as plain values and content IDs.
 * A data container with no logic, so its members are public like a struct's. Each owner writes and restores its own
 * part: the clock (AVestigeDayCycle), the tool in hand (UFarmingComponent), the inventory (UVestigeInventoryComponent), the
 * stamina (UVestigeStaminaSet), the rest (UVestigeRestComponent) and the tiles (AFarmGrid, AFarmTile).
 * Until the first public build there are no upgrades: raise CurrentVersion when a change makes older saves load wrong, and
 * older saves are then refused (Architecture.md, section 6). A new field whose default suits older saves needs no new version.
 */
UCLASS()
class UVestigeSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** The layout this build writes. Restarted at 1 on 2026-11-04, when the development saves were deleted. */
	static constexpr int32 CurrentVersion = 1;

	/** The two slots saves alternate between, so writing a save never touches the newest good one. */
	static constexpr const TCHAR* SlotNames[] = {TEXT("Farm"), TEXT("Farm2")};
	static constexpr int32 UserIndex = 0;

	/** The layout version this save was written with. 0 in a file that has none. */
	UPROPERTY()
	int32 SaveVersion = 0;

	/** Counts up with every save. Of the two slots, the one with the higher number holds the newer save. */
	UPROPERTY()
	int32 SaveSequence = 0;

	UPROPERTY()
	FVestigeClockSaveData Clock;

	UPROPERTY()
	FFarmFarmingSaveData Farming;

	UPROPERTY()
	FVestigeInventorySaveData Inventory;

	UPROPERTY()
	FVestigeStaminaSaveData Stamina;

	UPROPERTY()
	FVestigeRestSaveData Rest;

	/** Every tile, found again by its coordinates. */
	UPROPERTY()
	TArray<FFarmTileSaveData> Tiles;
};
