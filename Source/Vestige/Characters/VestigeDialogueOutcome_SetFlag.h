// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Content/VestigeDialogueOutcome.h"
#include "GameplayTagContainer.h"
#include "VestigeDialogueOutcome_SetFlag.generated.h"

/**
 * Sets a relationship flag of the speaker on today's day, or moves its day to today if it is set: the relationship component's
 * SetFlag. Lives in Characters/, since Content/ can't include the component. Does nothing without a context player or a
 * relationship component.
 */
UCLASS(meta = (DisplayName = "Set Flag"))
class UVestigeDialogueOutcome_SetFlag : public UVestigeDialogueOutcome
{
	GENERATED_BODY()

public:
	virtual void Apply_Implementation(const FVestigeDialogueContext& Context) const override;

#if WITH_EDITOR
	/** Reports an invalid flag tag. */
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

#if WITH_DEV_AUTOMATION_TESTS
	/** For tests only. */
	void SetFlagForTests(const FGameplayTag& InFlag) { Flag = InFlag; }
#endif

private:
	/** The flag, a tag under Vestige.Relationship (a mod's own root works too). */
	UPROPERTY(EditAnywhere, Category = "Dialogue", meta = (AllowPrivateAccess = "true"))
	FGameplayTag Flag;
};
