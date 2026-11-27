// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Content/VestigeDialogueCondition_Milestone.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "VestigeDialogueConditionMilestone"

bool UVestigeDialogueCondition_Milestone::IsMet_Implementation(const FVestigeDialogueContext& Context) const
{
	const FGameplayTagContainer& Counted = bToday ? Context.MilestonesToday : Context.Milestones;
	return Counted.HasTagExact(Milestone) == bReached;
}

#if WITH_EDITOR
EDataValidationResult UVestigeDialogueCondition_Milestone::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	if (!Milestone.IsValid())
	{
		Context.AddError(LOCTEXT("NoMilestone", "The milestone condition has no valid Milestone tag."));
		Result = EDataValidationResult::Invalid;
	}
	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
