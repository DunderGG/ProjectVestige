// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Core/VestigeFeedbackSubsystem.h"

#include "Core/VestigeLog.h"
#include "Engine/World.h"

namespace VestigeFeedback
{
	/** The report's values for the log, as "Day=2", separated by spaces. */
	FString DescribeArguments(const FFormatNamedArguments& Arguments)
	{
		TArray<FString> Parts;
		Parts.Reserve(Arguments.Num());
		for (const TPair<FString, FFormatArgumentValue>& Argument : Arguments)
		{
			constexpr bool bRebuildText = false;
			constexpr bool bRebuildAsSource = false;
			Parts.Add(Argument.Key + TEXT("=") + Argument.Value.ToFormattedString(bRebuildText, bRebuildAsSource));
		}
		return FString::Join(Parts, TEXT(" "));
	}
}

void UVestigeFeedbackSubsystem::Report(const UWorld* World, const FGameplayTag& Tag, FFormatNamedArguments Arguments)
{
	if (!Tag.IsValid())
	{
		VESTIGE_LOG(World, LogVestigeFeedback, Warning, "A report without a valid tag was ignored.");
		return;
	}

	// The HUD lets messages fade and shows nothing for some reports, so the log keeps the full history (Output Log, filtered
	// to LogVestigeFeedback).
	VESTIGE_LOG(World, LogVestigeFeedback, Log, "{Tag} {Arguments}", Tag.ToString(), VestigeFeedback::DescribeArguments(Arguments));

	if (UVestigeFeedbackSubsystem* Feedback = UWorld::GetSubsystem<UVestigeFeedbackSubsystem>(World))
	{
		Feedback->OnFeedback.Broadcast(FVestigeFeedbackReport{Tag, MoveTemp(Arguments)});
	}
}
