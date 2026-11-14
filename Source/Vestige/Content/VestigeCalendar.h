// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "VestigeCalendar.generated.h"

#if WITH_EDITOR
class FDataValidationContext;
#endif

/**
 * One season of the calendar: the tag crops name it by and the name the player reads. BlueprintType only so editor scripts can
 * make and set it; Blueprints don't read it.
 */
USTRUCT(BlueprintType)
struct FVestigeSeason
{
	GENERATED_BODY()

	/** The season's tag, under Vestige.Season (Vestige.Season.Spring). Crops name the seasons they grow in by it. */
	UPROPERTY(EditDefaultsOnly, Category = "Calendar", meta = (Categories = "Vestige.Season"))
	FGameplayTag Tag;

	/** The name the HUD and the messages show ("Spring"). */
	UPROPERTY(EditDefaultsOnly, Category = "Calendar")
	FText DisplayName;
};

/**
 * One day of the calendar, worked out from the clock's day count by FVestigeCalendar::GetDate. Never saved: the day count is the
 * calendar's only state (Systems.md, section 4).
 */
struct FVestigeDate
{
	/** The day count, from 1. */
	int32 Day = 1;

	/** The year, from 1. */
	int32 Year = 1;

	/** The season's index in the calendar's seasons, or INDEX_NONE when the calendar has none. */
	int32 SeasonIndex = INDEX_NONE;

	/** The season's tag, empty when the calendar has no seasons. */
	FGameplayTag Season;

	/** The day in the season, from 1 to the calendar's days per season. */
	int32 DayOfSeason = 1;

	/** The days after today in the season: 0 on its last day. */
	int32 DaysLeft = 0;

	bool IsFirstDayOfSeason() const { return DayOfSeason == 1; }

	/** False when the calendar has no seasons. */
	bool HasSeason() const { return SeasonIndex != INDEX_NONE; }
};

/**
 * The calendar's definition and its rule: the seasons in order and the days in each, and how a day count becomes a date. The
 * Calendar of the game data (DA_GameData), which the clock copies; code never names a season (Systems.md, section 4). Holds no
 * state of its own. BlueprintType only so editor scripts can make and set it; Blueprints don't read it.
 */
USTRUCT(BlueprintType)
struct FVestigeCalendar
{
	GENERATED_BODY()

public:
	/** The days in every season, at least 1. */
	int32 GetDaysPerSeason() const { return FMath::Max(1, DaysPerSeason); }

	/** The seasons in the order they come. */
	const TArray<FVestigeSeason>& GetSeasons() const { return Seasons; }

	/**
	 * The date of day count Day, a day before 1 counting as day 1. The seasons follow each other in order, then a new year begins
	 * with the first. Without seasons the date has none, its year is 1, and it still counts the day of the season and the days left.
	 */
	FVestigeDate GetDate(int32 Day) const;

	/** The season with Tag, or nullptr when the calendar has none. */
	const FVestigeSeason* FindSeason(const FGameplayTag& Tag) const;

	/** The display name of Date's season, empty when it has none. */
	FText GetSeasonName(const FVestigeDate& Date) const;

	/**
	 * The values a feedback report about day count Day gives its text: Day (the day count), Season (its season's name, empty
	 * without one) and DayOfSeason. Every report that names a date uses these, so they all name it the same way.
	 */
	FFormatNamedArguments GetReportValues(int32 Day) const;

	/**
	 * GetReportValues(Day) plus PreviousSeason, the name of the season of the day before Day: the season that has just ended.
	 * For day 1, which no season starts on through a new day, it is the day's own season.
	 */
	FFormatNamedArguments GetSeasonStartValues(int32 Day) const;

#if WITH_EDITOR
	/**
	 * Adds one error to Context for each problem: no seasons, fewer than 1 day per season, a season tag that is empty, is
	 * Vestige.Season itself or isn't under it, a tag used twice, and an empty name.
	 */
	EDataValidationResult Validate(FDataValidationContext& Context) const;
#endif

#if WITH_DEV_AUTOMATION_TESTS
	/** For tests only. The game's calendar is set in DA_GameData. */
	static FVestigeCalendar MakeForTests(int32 InDaysPerSeason, TArray<FVestigeSeason> InSeasons);
#endif

private:
	/** The days in every season. Below 1 counts as 1 and is reported by data validation. */
	UPROPERTY(EditDefaultsOnly, Category = "Calendar", meta = (AllowPrivateAccess = "true", ClampMin = 1))
	int32 DaysPerSeason = 28;

	/** The seasons in the order they come; after the last, a new year starts with the first. */
	UPROPERTY(EditDefaultsOnly, Category = "Calendar", meta = (AllowPrivateAccess = "true", TitleProperty = "DisplayName"))
	TArray<FVestigeSeason> Seasons;
};
