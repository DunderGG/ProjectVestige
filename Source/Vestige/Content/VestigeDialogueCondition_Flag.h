// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Content/VestigeDialogueCondition.h"
#include "GameplayTagContainer.h"
#include "VestigeDialogueCondition_Flag.generated.h"

/**
 * Holds when a relationship flag of the speaker is set (or, with bSet off, isn't), counting a flag as set only when it was last set
 * within MinDaysSince to MaxDaysSince days ago. So "set, at least a day ago" is bSet with MinDaysSince 1; "set today" is bSet
 * with MaxDaysSince 0; "not set in the last two days" is bSet off with MaxDaysSince 1.
 */
UCLASS(meta = (DisplayName = "Flag"))
class UVestigeDialogueCondition_Flag : public UVestigeDialogueCondition
{
	GENERATED_BODY()

public:
	virtual bool IsMet_Implementation(const FVestigeDialogueContext& Context) const override;

#if WITH_EDITOR
	/** Reports an invalid flag tag and day limits that don't make a range. */
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

#if WITH_DEV_AUTOMATION_TESTS
	/** For tests only. */
	void SetForTests(const FGameplayTag& InFlag, bool bInSet, int32 InMinDaysSince = 0, int32 InMaxDaysSince = -1)
	{
		Flag = InFlag;
		bSet = bInSet;
		MinDaysSince = InMinDaysSince;
		MaxDaysSince = InMaxDaysSince;
	}
#endif

private:
	/** The flag, a tag under Vestige.Relationship (a mod's own root works too). */
	UPROPERTY(EditAnywhere, Category = "Dialogue", meta = (AllowPrivateAccess = "true"))
	FGameplayTag Flag;

	/** Whether the flag must be set (true) or not set (false) within the days below. */
	UPROPERTY(EditAnywhere, Category = "Dialogue", meta = (AllowPrivateAccess = "true"))
	bool bSet = true;

	/** The fewest days since the flag was last set for it to count as set: 0 is today. */
	UPROPERTY(EditAnywhere, Category = "Dialogue", meta = (AllowPrivateAccess = "true", ClampMin = 0))
	int32 MinDaysSince = 0;

	/** The most days since it was last set for it to count as set: -1 for no limit. */
	UPROPERTY(EditAnywhere, Category = "Dialogue", meta = (AllowPrivateAccess = "true", ClampMin = -1))
	int32 MaxDaysSince = -1;
};
