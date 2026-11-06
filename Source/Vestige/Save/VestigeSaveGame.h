// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Core/VestigeSaveRecord.h"
#include "GameFramework/SaveGame.h"
#include "VestigeSaveGame.generated.h"

/**
 * The save file's layout: its format version, its place in the order of saves, and every owner's part as a record by
 * key (Systems.md, section 1). A data container with no logic, so its members are public like a struct's. It names no
 * owner: each owner writes and reads its own record through UVestigeSaveRegistry, with its own version.
 * Until the first public build there are no upgrades: raise CurrentVersion only when this file's own layout changes, and
 * older files are then refused (Architecture.md, section 6).
 */
UCLASS()
class UVestigeSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/**
	 * The file layout this build writes. Restarted at 1 on 2026-11-04, when the development saves were deleted; 2 when the
	 * tiles moved into one entry per field, and 3 since 2026-11-06, when every owner's part became a record by key.
	 */
	static constexpr int32 CurrentVersion = 3;

	/** The two slots saves alternate between, so writing a save never touches the newest good one. */
	static constexpr const TCHAR* SlotNames[] = {TEXT("Farm"), TEXT("Farm2")};
	static constexpr int32 UserIndex = 0;

	/** The file layout version this save was written with. 0 in a file that has none. */
	UPROPERTY()
	int32 SaveVersion = 0;

	/** Counts up with every save. Of the two slots, the one with the higher number holds the newer save. */
	UPROPERTY()
	int32 SaveSequence = 0;

	/** Every owner's part by its key (Base.Clock), including parts no owner in this build reads, which are kept. */
	UPROPERTY()
	TMap<FName, FVestigeSaveRecord> Records;
};
