// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Content/VestigeDialogueCondition_TimeOfDay.h"

bool UVestigeDialogueCondition_TimeOfDay::IsMet_Implementation(const FVestigeDialogueContext& Context) const
{
	return Context.TimeOfDay >= FromHour && Context.TimeOfDay < ToHour;
}
