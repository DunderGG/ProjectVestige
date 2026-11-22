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

	/**
	 * The report's values and clauses for the log: its values, then each clause as "Paid=[Vestige.Feedback.Shipping.Paid
	 * Amount=250]", or "Paid=[]" for an empty one.
	 */
	FString DescribeReport(const FVestigeFeedbackReport& FeedbackReport)
	{
		TArray<FString> Parts;
		Parts.Reserve(1 + FeedbackReport.Clauses.Num());
		Parts.Add(DescribeArguments(FeedbackReport.Arguments));
		for (const FVestigeFeedbackClause& Clause : FeedbackReport.Clauses)
		{
			FString Content;
			if (Clause.Tag.IsValid())
			{
				Content = (Clause.Tag.ToString() + TEXT(" ") + DescribeArguments(Clause.Arguments)).TrimEnd();
			}
			Parts.Add(Clause.Argument.ToString() + TEXT("=[") + Content + TEXT("]"));
		}
		return FString::Join(Parts, TEXT(" ")).TrimStart();
	}
}

void UVestigeFeedbackSubsystem::Report(const UWorld* World, const FGameplayTag& Tag, FFormatNamedArguments Arguments)
{
	Report(World, FVestigeFeedbackReport{Tag, MoveTemp(Arguments), {}});
}

void UVestigeFeedbackSubsystem::Report(const UWorld* World, FVestigeFeedbackReport FeedbackReport)
{
	if (!FeedbackReport.Tag.IsValid())
	{
		VESTIGE_LOG(World, LogVestigeFeedback, Warning, "A report without a valid tag was ignored.");
		return;
	}

	// The HUD lets messages fade and shows nothing for some reports, so the log keeps the full history (Output Log, filtered
	// to LogVestigeFeedback).
	VESTIGE_LOG(World, LogVestigeFeedback, Log, "{Tag} {Arguments}", FeedbackReport.Tag.ToString(),
		VestigeFeedback::DescribeReport(FeedbackReport));

	if (UVestigeFeedbackSubsystem* Feedback = UWorld::GetSubsystem<UVestigeFeedbackSubsystem>(World))
	{
		Feedback->OnFeedback.Broadcast(FeedbackReport);
	}
}
