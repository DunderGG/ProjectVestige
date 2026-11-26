// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Tests/VestigeTestDialogueOutcome.h"

#if WITH_TESTS

void UVestigeTestDialogueOutcome::Setup(const FString& InName, TArray<FString>& AppliedLog)
{
	Name = InName;
	Log = &AppliedLog;
}

void UVestigeTestDialogueOutcome::Apply_Implementation(const FVestigeDialogueContext& Context) const
{
	if (Log)
	{
		Log->Add(Name);
	}
}

#endif
