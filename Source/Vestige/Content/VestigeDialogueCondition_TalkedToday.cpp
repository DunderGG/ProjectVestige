// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Content/VestigeDialogueCondition_TalkedToday.h"

bool UVestigeDialogueCondition_TalkedToday::IsMet_Implementation(const FVestigeDialogueContext& Context) const
{
	return (Context.TalksToday > 0) == bTalked;
}
