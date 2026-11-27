// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Content/VestigeDialogueCondition_Flag.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "VestigeDialogueConditionFlag"

bool UVestigeDialogueCondition_Flag::IsMet_Implementation(const FVestigeDialogueContext& Context) const
{
	const int32* DaySet = Context.FlagDays.Find(Flag);
	bool bCountsAsSet = false;
	if (DaySet)
	{
		const int32 DaysSince = Context.Day - *DaySet;
		bCountsAsSet = DaysSince >= MinDaysSince && (MaxDaysSince < 0 || DaysSince <= MaxDaysSince);
	}
	return bCountsAsSet == bSet;
}

#if WITH_EDITOR
EDataValidationResult UVestigeDialogueCondition_Flag::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	if (!Flag.IsValid())
	{
		Context.AddError(LOCTEXT("NoFlag", "The flag condition has no valid Flag tag."));
		Result = EDataValidationResult::Invalid;
	}
	if (MinDaysSince < 0 || (MaxDaysSince >= 0 && MaxDaysSince < MinDaysSince) || MaxDaysSince < -1)
	{
		Context.AddError(
			LOCTEXT("BadDays", "The flag condition's Min Days Since must be at least 0, and Max Days Since -1 or not below it."));
		Result = EDataValidationResult::Invalid;
	}
	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
