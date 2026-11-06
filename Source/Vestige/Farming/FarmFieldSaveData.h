// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Farming/FarmTileState.h"
#include "FarmFieldSaveData.generated.h"

/**
 * One field in a save: its ID, its size and every tile, as plain values and content IDs. UFarmFieldSubsystem fills one
 * per field and each UFarmField fills its tiles.
 */
USTRUCT()
struct FFarmFieldSaveData
{
	GENERATED_BODY()

	/** The field's ID, as its grid names it (Base.Farm). */
	UPROPERTY()
	FName FieldId;

	/** Columns (X) and rows (Y), so a field whose map isn't loaded is restored at its own size. */
	UPROPERTY()
	FIntPoint Size = FIntPoint::ZeroValue;

	/** Every tile, found again by its coordinates. */
	UPROPERTY()
	TArray<FFarmTileSaveData> Tiles;
};

/** UFarmFieldSubsystem's part of a save, under Base.Fields: every field of every map. */
USTRUCT()
struct FFarmFieldsSaveData
{
	GENERATED_BODY()

	/** This layout's version (Systems.md, section 1). Raise it when a change would load older parts wrong. */
	static constexpr int32 Version = 1;

	UPROPERTY()
	TArray<FFarmFieldSaveData> Fields;
};
