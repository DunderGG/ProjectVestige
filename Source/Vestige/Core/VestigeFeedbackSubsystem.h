// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/WorldSubsystem.h"
#include "VestigeFeedbackSubsystem.generated.h"

/**
 * A part of a report's message that another row words (Systems.md, section 15, clauses): the text argument it fills (such as
 * Paid), the feedback tag whose row words it, and that row's values. An invalid tag is an empty clause: the argument is empty.
 */
struct FVestigeFeedbackClause
{
	FName Argument;
	FGameplayTag Tag;
	FFormatNamedArguments Arguments;
};

/**
 * Something the player may need to know about, as gameplay reports it: what happened (a Vestige.Feedback tag) and the values
 * a text about it may use (such as Day), and the clauses that fill the text's other arguments. Presentation picks the text,
 * sound or effect for the tag; gameplay never writes it.
 */
struct FVestigeFeedbackReport
{
	FGameplayTag Tag;
	FFormatNamedArguments Arguments;
	TArray<FVestigeFeedbackClause> Clauses;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnVestigeFeedback, const FVestigeFeedbackReport& /* Report */);

/**
 * Carries feedback reports from game code to whatever presents them (Systems.md, section 15, feedback reports).
 * Game code reports what happened and never knows who listens; AVestigeHUD listens and shows the text its responses asset
 * gives the tag. The game is single-player, so the world's player is the one the reports are for.
 */
UCLASS()
class UVestigeFeedbackSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * Reports Tag, with the values a text about it may use, for the player in World. Does nothing without a world or before the
	 * subsystem exists. An invalid tag logs a warning and isn't broadcast.
	 */
	static void Report(const UWorld* World, const FGameplayTag& Tag, FFormatNamedArguments Arguments = {});

	/** Reports FeedbackReport, a report that may carry clauses, as the other overload does. */
	static void Report(const UWorld* World, FVestigeFeedbackReport FeedbackReport);

	/** Broadcast with each report as it is made. Presentation listens; game code doesn't. */
	FOnVestigeFeedback OnFeedback;
};
