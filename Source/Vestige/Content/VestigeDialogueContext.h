// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "VestigeDialogueContext.generated.h"

class APlayerState;
class UVestigeCharacterData;

/**
 * What a dialogue condition or outcome knows about the talk, filled by the conversation component before it chooses a
 * conversation: who speaks, who listens, today and the state of the player's relationship with the speaker. Our conditions
 * read only these values, so they are pure and need no world (Systems.md, section 11).
 */
USTRUCT(BlueprintType)
struct FVestigeDialogueContext
{
	GENERATED_BODY()

	/** The character the player talks to. */
	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	TObjectPtr<const UVestigeCharacterData> Speaker = nullptr;

	/** The player's state, for a mod's condition that needs more than the values below. */
	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	TObjectPtr<APlayerState> Player = nullptr;

	/** The day count, from 1. */
	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	int32 Day = 0;

	/** Hours since midnight at the start of the day, so after midnight is 24 to 30 (the clock's GetTimeOfDay). */
	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	float TimeOfDay = 0.f;

	/** Whether the player has met the speaker. */
	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	bool bHasMet = false;

	/**
	 * How many talks with the speaker started today. The context that chooses the conversation is made before the talk is recorded,
	 * so it does not count this talk and bHasMet is the state before it; the contexts of the lines, replies and outcomes of the
	 * talk are made after it, so they count it and bHasMet is true.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	int32 TalksToday = 0;

	/** How much the player's trust with the speaker is: earned points plus the speaker's points for the milestones reached. */
	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	int32 Trust = 0;

	/** A copy of the speaker's relationship flags, each with the day it was last set. */
	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	TMap<FGameplayTag, int32> FlagDays;

	/** A copy of the milestones reached. */
	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	FGameplayTagContainer Milestones;

	/** Today's season tag, empty when the calendar has no seasons. */
	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	FGameplayTag Season;

	/** The day in the season, from 1. */
	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	int32 DayOfSeason = 1;
};
