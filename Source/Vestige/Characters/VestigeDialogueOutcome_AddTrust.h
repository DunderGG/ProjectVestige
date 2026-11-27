// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Content/VestigeDialogueOutcome.h"
#include "VestigeDialogueOutcome_AddTrust.generated.h"

/**
 * Gives the speaker's trust Amount points: the relationship component's AddTrust. Lives in Characters/, since Content/ can't include
 * the component. Does nothing without a context player or a relationship component.
 */
UCLASS(meta = (DisplayName = "Add Trust"))
class UVestigeDialogueOutcome_AddTrust : public UVestigeDialogueOutcome
{
	GENERATED_BODY()

public:
	virtual void Apply_Implementation(const FVestigeDialogueContext& Context) const override;

#if WITH_EDITOR
	/** Reports an Amount below 1. */
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

#if WITH_DEV_AUTOMATION_TESTS
	/** For tests only. */
	void SetAmountForTests(int32 InAmount) { Amount = InAmount; }
#endif

private:
	/** The trust points given, at least 1. */
	UPROPERTY(EditAnywhere, Category = "Dialogue", meta = (AllowPrivateAccess = "true", ClampMin = 1))
	int32 Amount = 1;
};
