// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Content/VestigeDialogueCondition_Trust.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "VestigeDialogueConditionTrust"

bool UVestigeDialogueCondition_Trust::IsMet_Implementation(const FVestigeDialogueContext& Context) const
{
	return Context.Trust >= MinTrust;
}

#if WITH_EDITOR
EDataValidationResult UVestigeDialogueCondition_Trust::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	if (MinTrust < 0)
	{
		Context.AddError(LOCTEXT("NegativeTrust", "The trust condition's Min Trust is below 0."));
		Result = EDataValidationResult::Invalid;
	}
	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
