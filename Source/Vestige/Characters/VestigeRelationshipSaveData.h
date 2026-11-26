// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "VestigeRelationshipSaveData.generated.h"

/** What the player's relationship holds about one character in a save: their ContentId and whether the player has met them. */
USTRUCT()
struct FVestigeCharacterRelationshipSaveData
{
	GENERATED_BODY()

	/** ContentId of the character (Base.Hob). */
	UPROPERTY()
	FName Character;

	UPROPERTY()
	bool bMet = false;
};

/**
 * UVestigeRelationshipComponent's part of a save, under Base.Relationships: plain values and content IDs only. Today's talks
 * aren't saved. The trust track adds trust and flags to the per-character struct.
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
