// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

// Only in builds with tests, like UVestigeTestEventRecorder.
#if WITH_TESTS

#include "CoreMinimal.h"
#include "Content/VestigeDialogueOutcome.h"
#include "VestigeTestDialogueOutcome.generated.h"

/** A dialogue outcome for tests: writes its name into a list the test owns each time it is applied, so a test sees when. */
UCLASS(NotBlueprintable, HideDropdown)
class UVestigeTestDialogueOutcome : public UVestigeDialogueOutcome
{
	GENERATED_BODY()

public:
	/** Makes Apply add Name to AppliedLog. The log must outlive the outcome's use. */
	void Setup(const FString& InName, TArray<FString>& AppliedLog);

	virtual void Apply_Implementation(const FVestigeDialogueContext& Context) const override;

private:
	FString Name;
	TArray<FString>* Log = nullptr;
};

#endif
