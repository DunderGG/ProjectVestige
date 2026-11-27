// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Content/VestigeDialogueCondition.h"
#include "GameplayTagContainer.h"
#include "VestigeDialogueCondition_Date.generated.h"

/**
 * Holds from a day count on and, optionally, only in one season and between two days of it: Day >= FromDay, the season matches
 * when one is named, and FromDayOfSeason <= DayOfSeason, and DayOfSeason <= ToDayOfSeason unless that is 0 (the season's last).
 */
UCLASS(meta = (DisplayName = "Date"))
class UVestigeDialogueCondition_Date : public UVestigeDialogueCondition
{
	GENERATED_BODY()

public:
	virtual bool IsMet_Implementation(const FVestigeDialogueContext& Context) const override;

#if WITH_EDITOR
	/** Reports a day below 1 and a range that ends before it starts. */
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

#if WITH_DEV_AUTOMATION_TESTS
	/** For tests only. */
	void SetForTests(int32 InFromDay, const FGameplayTag& InSeason = FGameplayTag(), int32 InFromDayOfSeason = 1, int32 InToDayOfSeason = 0)
	{
		FromDay = InFromDay;
		Season = InSeason;
		FromDayOfSeason = InFromDayOfSeason;
		ToDayOfSeason = InToDayOfSeason;
	}
#endif

private:
	/** The first day count it holds on, from 1. */
	UPROPERTY(EditAnywhere, Category = "Dialogue", meta = (AllowPrivateAccess = "true", ClampMin = 1))
	int32 FromDay = 1;

	/** The season it holds in, a tag under Vestige.Season; empty for any. */
	UPROPERTY(EditAnywhere, Category = "Dialogue", meta = (AllowPrivateAccess = "true", Categories = "Vestige.Season"))
	FGameplayTag Season;

	/** The first day of the season it holds on, from 1. */
	UPROPERTY(EditAnywhere, Category = "Dialogue", meta = (AllowPrivateAccess = "true", ClampMin = 1))
	int32 FromDayOfSeason = 1;

	/** The last day of the season it holds on; 0 for the season's last day. */
	UPROPERTY(EditAnywhere, Category = "Dialogue", meta = (AllowPrivateAccess = "true", ClampMin = 0))
	int32 ToDayOfSeason = 0;
};
