// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "UI/VestigeFeedbackResponses.h"

#include "Core/VestigeFeedbackSubsystem.h"
#include "GameplayTagsManager.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "VestigeFeedbackResponses"

FText UVestigeFeedbackResponses::FormatMessage(const FVestigeFeedbackResponse& Response, const FVestigeFeedbackReport& Report)
{
	return FText::Format(Response.Message, Report.Arguments);
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
	}
	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
