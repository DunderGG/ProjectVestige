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

	/** The name the HUD shows and messages use at the start of a sentence ("Spring"). */
	UPROPERTY(EditDefaultsOnly, Category = "Calendar")
	FText DisplayName;

	/**
	 * The name messages use in the middle of a sentence ("spring"). Its own text rather than DisplayName in lower case, since
	 * languages differ in which names they capitalise.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Calendar")
	FText NameInSentence;
};

/** Which kind of day a day is in its season, for the reports that tell the player about it. A fixed set of calendar states. */
enum class EVestigeDayKind : uint8
{
	/** A day that is neither of the others. */
	Usual,

	/** The first day of a season, after a day of another season. Never day 1, nor in a calendar of one season or none. */
	NewSeason,

	/** A season's last day, with no days left, unless it is also a new season (a season of one day). */
	LastDay
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

	/** True on the season's last day, when no days are left. */
	bool IsLastDayOfSeason() const { return DaysLeft == 0; }

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

	/** The name of Date's season in the middle of a sentence, empty when it has none. */
	FText GetSeasonNameInSentence(const FVestigeDate& Date) const;

	/**
	 * Which kind of day count Day is: NewSeason when it is a season's first day and the day before was another season, else
	 * LastDay when it has no days left, else Usual. The one rule for a season starting: the clock's OnSeasonStarted and the
	 * reports' tags follow it.
	 */
	EVestigeDayKind GetDayKind(int32 Day) const;

	/**
	 * The values a feedback report about day count Day gives its text: Day (the day count), Season (its season's name, empty
	 * without one), SeasonInSentence (that name in a sentence), DayOfSeason and DaysLeft (a number). Every report that names a
	 * date uses these, so they all name it the same way. None is a finished phrase: the texts are the responses'.
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
	 * Vestige.Season itself or isn't under it, a tag used twice, an empty DisplayName and an empty NameInSentence.
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
