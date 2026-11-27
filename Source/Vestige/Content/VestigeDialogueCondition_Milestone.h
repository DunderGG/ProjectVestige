// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Content/VestigeDialogueCondition.h"
#include "GameplayTagContainer.h"
#include "VestigeDialogueCondition_Milestone.generated.h"

/** Holds when a milestone (Vestige.Milestone.Farm.Harvested) has been reached, or, with bReached off, hasn't. */
UCLASS(meta = (DisplayName = "Milestone"))
class UVestigeDialogueCondition_Milestone : public UVestigeDialogueCondition
{
	GENERATED_BODY()

public:
	virtual bool IsMet_Implementation(const FVestigeDialogueContext& Context) const override;

#if WITH_EDITOR
	/** Reports an invalid milestone tag. */
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

#if WITH_DEV_AUTOMATION_TESTS
	/** For tests only. */
	void SetForTests(const FGameplayTag& InMilestone, bool bInReached)
	{
		Milestone = InMilestone;
		bReached = bInReached;
	}
#endif

private:
	/** The milestone, a tag under Vestige.Milestone. */
	UPROPERTY(EditAnywhere, Category = "Dialogue", meta = (AllowPrivateAccess = "true", Categories = "Vestige.Milestone"))
	FGameplayTag Milestone;

	/** Whether it must have been reached (true) or not (false). */
	UPROPERTY(EditAnywhere, Category = "Dialogue", meta = (AllowPrivateAccess = "true"))
	bool bReached = true;
};
