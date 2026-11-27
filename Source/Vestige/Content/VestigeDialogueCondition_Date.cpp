// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Content/VestigeDialogueCondition_Date.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "VestigeDialogueConditionDate"

bool UVestigeDialogueCondition_Date::IsMet_Implementation(const FVestigeDialogueContext& Context) const
{
	if (Context.Day < FromDay)
	{
		return false;
	}
	if (Season.IsValid() && Context.Season != Season)
	{
		return false;
	}
	return Context.DayOfSeason >= FromDayOfSeason && (ToDayOfSeason == 0 || Context.DayOfSeason <= ToDayOfSeason);
}

#if WITH_EDITOR
EDataValidationResult UVestigeDialogueCondition_Date::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	if (FromDay < 1 || FromDayOfSeason < 1 || ToDayOfSeason < 0 || (ToDayOfSeason > 0 && ToDayOfSeason < FromDayOfSeason))
	{
		Context.AddError(
			LOCTEXT("BadRange", "The date condition's days must start at 1 or later and end, unless 0, on or after the first."));
		Result = EDataValidationResult::Invalid;
	}
	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
