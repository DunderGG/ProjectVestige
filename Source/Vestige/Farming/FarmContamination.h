// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "FarmContamination.generated.h"

/**
 * The shared numbers of tile contamination (Systems.md, section 27), set in the Vestige Farm Fields developer settings and
 * copied into every UFarmField: the highest level a tile can hold and how likely a crop ripening on each level is to mutate.
 * How a crop responds to a level is the crop's own data.
 */
USTRUCT()
struct FFarmContaminationRules
{
	GENERATED_BODY()

	/** The highest contamination level: 0 is clean, 1 lightly and 2 heavily contaminated. */
	UPROPERTY(EditAnywhere, Category = "Contamination", meta = (ClampMin = 1))
	int32 MaxLevel = 2;

	/**
	 * The chance, from 0 to 1, that a crop that can mutate ripens mutated, by the level it ripened on (index = level). A level
	 * past the end uses the last entry; an empty list means no mutation.
	 */
	UPROPERTY(EditAnywhere, Category = "Contamination", meta = (ClampMin = 0, ClampMax = 1))
	TArray<float> MutationChanceByLevel = {0.f, 0.25f, 0.f};

	/** The mutation chance on Level, from 0 to 1. */
	float GetMutationChance(int32 Level) const;

	/** Rules with MaxLevel lifted to at least 1 and each chance clamped to 0..1. A pure function, so tests don't change the settings. */
	static FFarmContaminationRules GetClamped(const FFarmContaminationRules& Rules);
};

/**
 * One rectangle of a field's starting contamination: the tiles from Min to Max, both corners included, start at Level. Set on a
 * placed AFarmGrid. BlueprintType only so editor scripts can make and set it; Blueprints don't read it.
 */
USTRUCT(BlueprintType)
struct FFarmContaminationArea
{
	GENERATED_BODY()

	/** The corner with the lowest column (X) and row (Y). */
	UPROPERTY(EditAnywhere, Category = "Farming")
	FIntPoint Min = FIntPoint::ZeroValue;

	/** The corner with the highest column (X) and row (Y), included in the area. */
	UPROPERTY(EditAnywhere, Category = "Farming")
	FIntPoint Max = FIntPoint::ZeroValue;

	/** The level the area's tiles start a new game with, clamped to the field's rules. */
	UPROPERTY(EditAnywhere, Category = "Farming", meta = (ClampMin = 0))
	int32 Level = 1;
};

namespace FarmContamination
{
	/**
	 * Whether a crop ripening on day count Day at Coordinates of field FieldId ripens mutated, for a game whose mutation seed is
	 * Seed. Repeatable on every run and every machine: the same inputs always give the same answer, so a reloaded night rolls the
	 * same. False for a Chance at or below 0, true at or above 1, without a roll. Pure and stateless.
	 */
	bool RollsMutation(int32 Seed, FName FieldId, const FIntPoint& Coordinates, int32 Day, float Chance);
}
