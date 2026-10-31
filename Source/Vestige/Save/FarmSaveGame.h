// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Farming/FarmFarmingSaveData.h"
#include "Farming/FarmTileState.h"
#include "GameFramework/SaveGame.h"
#include "Inventory/VestigeInventorySaveData.h"
#include "Time/FarmClockSaveData.h"
#include "FarmSaveGame.generated.h"

/**
 * The hard-coded tools from before tools were data assets. Only read when upgrading a version 0 save.
 * The names must stay as they are, because those saves store the enum by name.
 */
UENUM()
enum class EFarmTool : uint8
{
	Hoe,
	Seeds,
	WateringCan,
	Hand
};

/**
 * The saved data layout: everything needed to pick the farm back up, as plain values and content IDs.
 * A data container with no logic, so its members are public like a struct's. Each owner writes and restores its own
 * part: the clock (AFarmDayCycle), the tool in hand (UFarmingComponent), the inventory (UVestigeInventoryComponent) and
 * the tiles (AFarmGrid, AFarmTile).
 * Raise CurrentVersion with every layout change and add an upgrade step to UFarmSaveSubsystem::UpgradeToCurrentVersion.
 */
UCLASS()
class UFarmSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/**
	 * The layout this build writes.
	 * 0: before save versions. Flat fields, tiles in grid order without coordinates or crops, one slot.
	 * 1: each owner's part in its own struct, tiles by coordinates with their crop's ContentId, two alternating slots.
	 * 2: the inventory's slots, holding the seeds and harvest that version 1 kept as two counts in Farming.
	 */
	static constexpr int32 CurrentVersion = 2;

	/**
	 * The two slots saves alternate between, so writing a save never touches the newest good one.
	 * The first is also the single slot version 0 saves used.
	 */
	static constexpr const TCHAR* SlotNames[] = {TEXT("Farm"), TEXT("Farm2")};
	static constexpr int32 UserIndex = 0;

	/** The layout version this save was written with. Saves from before versions existed read as 0. */
	UPROPERTY()
	int32 SaveVersion = 0;

	/** Counts up with every save. Of the two slots, the one with the higher number holds the newer save. */
	UPROPERTY()
	int32 SaveSequence = 0;

	UPROPERTY()
	FFarmClockSaveData Clock;

	UPROPERTY()
	FFarmFarmingSaveData Farming;

	/** Empty in saves before version 2. */
	UPROPERTY()
	FVestigeInventorySaveData Inventory;

	/** Every tile, found again by its coordinates. In version 0 saves, in grid order without coordinates or crops. */
	UPROPERTY()
	TArray<FFarmTileSaveData> Tiles;

	// The rest is the version 0 layout, kept under its old names so old saves still read into it. Only the upgrade
	// reads these fields.

	UPROPERTY()
	int32 Day = 1;

	UPROPERTY()
	float TimeOfDay = 0.f;

	UPROPERTY()
	int32 Seeds = 0;

	UPROPERTY()
	int32 HarvestedCrops = 0;

	/** ContentId of the tool in hand. Empty in version 0 saves made before tools were data assets. */
	UPROPERTY()
	FName CurrentToolId;

	/** The tool in hand in saves made before tools were data assets. Only read when CurrentToolId is empty. */
	UPROPERTY()
	EFarmTool CurrentTool = EFarmTool::Hoe;
};
