// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "UI/VestigeFeedbackResponses.h"

#include "Core/VestigeFeedbackSubsystem.h"
#include "Core/VestigeLog.h"
#include "GameplayTagsManager.h"
#include "Sound/SoundBase.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "VestigeFeedbackResponses"

FText UVestigeFeedbackResponses::FormatClause(const FVestigeFeedbackClause& Clause) const
{
	if (!Clause.Tag.IsValid())
	{
		return FText::GetEmpty();
	}

	const FVestigeFeedbackResponse* ClauseResponse = FindResponse(Clause.Tag);
	if (!ClauseResponse || ClauseResponse->Message.IsEmpty())
	{
		VESTIGE_LOG(this, LogVestigeFeedback, Warning, "{Responses} has no text for the clause {Argument} ({Tag}), so it is left empty.",
			GetNameSafe(this), Clause.Argument.ToString(), Clause.Tag.ToString());
		return FText::GetEmpty();
	}
	return FText::Format(ClauseResponse->Message, Clause.Arguments);
}

FText UVestigeFeedbackResponses::FormatMessage(const FVestigeFeedbackResponse& Response, const FVestigeFeedbackReport& FeedbackReport) const
{
	FFormatNamedArguments Arguments = FeedbackReport.Arguments;
	for (const FVestigeFeedbackClause& Clause : FeedbackReport.Clauses)
	{
		Arguments.Add(Clause.Argument.ToString(), FormatClause(Clause));
	}

	// A row ends with its clause's argument (" {Paid}"), so an empty clause leaves a space to trim.
	return FText::TrimTrailing(FText::Format(Response.Message, Arguments));
}

#if WITH_EDITOR
EDataValidationResult UVestigeFeedbackResponses::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	for (const TPair<FGameplayTag, FVestigeFeedbackResponse>& Row : Responses)
	{
		constexpr bool bErrorIfNotFound = false;
		const bool bKnown =
			Row.Key.IsValid() && UGameplayTagsManager::Get().RequestGameplayTag(Row.Key.GetTagName(), bErrorIfNotFound).IsValid();
		if (!bKnown)
		{
			Context.AddError(FText::Format(
				LOCTEXT("UnknownTag", "A row's tag '{0}' isn't a known gameplay tag. Pick a Vestige.Feedback tag or remove the row."),
				FText::FromName(Row.Key.GetTagName())));
			Result = EDataValidationResult::Invalid;
		}
		if (Row.Value.Sound && Row.Value.Sound->IsLooping())
		{
			Context.AddError(FText::Format(
				LOCTEXT("LoopingSound", "The row for '{0}' plays '{1}', which loops. A feedback sound can't be stopped, so it must not loop."),
				FText::FromName(Row.Key.GetTagName()), FText::FromString(GetNameSafe(Row.Value.Sound))));
			Result = EDataValidationResult::Invalid;
		}
	}
	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
