// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "VestigeRelationshipSaveData.generated.h"

/** One relationship flag in a save: the tag's name and the day it was last set. */
USTRUCT()
struct FVestigeRelationshipFlagSaveData
{
	GENERATED_BODY()

	UPROPERTY()
	FName Flag;

	UPROPERTY()
	int32 Day = 0;
};

/**
 * What the player's relationship holds about one character in a save: their ContentId, whether the player has met them, the trust
 * points earned from talking and their flags. The trust the farm's firsts give isn't saved: it is worked out from the milestones.
 * The fields after bMet have defaults that suit a record written before them, so the version stays 1.
 */
USTRUCT()
struct FVestigeCharacterRelationshipSaveData
{
	GENERATED_BODY()

	/** ContentId of the character (Base.Hob). */
	UPROPERTY()
	FName Character;

	UPROPERTY()
	bool bMet = false;

	/** The trust earned from talking (the AddTrust outcome), never below 0. */
	UPROPERTY()
	int32 EarnedTrust = 0;

	/** The flags set, sorted by name so the same state writes the same bytes. */
	UPROPERTY()
	TArray<FVestigeRelationshipFlagSaveData> Flags;
};

/**
 * UVestigeRelationshipComponent's part of a save, under Base.Relationships: plain values and content IDs only. Today's talks
 * aren't saved.
 */
USTRUCT()
struct FVestigeRelationshipSaveData
{
	GENERATED_BODY()

	/** This layout's version (Systems.md, section 1). Raise it when a change would load older parts wrong. */
	static constexpr int32 Version = 1;

	UPROPERTY()
	TArray<FVestigeCharacterRelationshipSaveData> Characters;
};
