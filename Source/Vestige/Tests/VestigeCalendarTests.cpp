// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/VestigeGameplayTags.h"
#include "Content/VestigeCalendar.h"
#include "CQTest.h"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace VestigeCalendarTests
{
	constexpr int32 DaysPerSeason = 10;

	/** A season of a made-up calendar: a tag that exists (Vestige.Season.<Name>) and a name that is not the game's own. */
	FVestigeSeason MakeSeason(const TCHAR* TagName, const TCHAR* DisplayName)
	{
		FVestigeSeason Season;
		Season.Tag = FGameplayTag::RequestGameplayTag(FName(TagName));
		Season.DisplayName = FText::FromString(DisplayName);
		return Season;
	}

	/** Four seasons of ten days, so the numbers in the tests are not the game's. */
	FVestigeCalendar MakeCalendar()
	{
		return FVestigeCalendar::MakeForTests(DaysPerSeason,
			{MakeSeason(TEXT("Vestige.Season.Spring"), TEXT("Alpha")), MakeSeason(TEXT("Vestige.Season.Summer"), TEXT("Beta")),
				MakeSeason(TEXT("Vestige.Season.Autumn"), TEXT("Gamma")), MakeSeason(TEXT("Vestige.Season.Winter"), TEXT("Delta"))});
	}

	/** The text of the report value Name, or an empty string when it is missing or not a text. */
	FString GetText(const FFormatNamedArguments& Values, const TCHAR* Name)
	{
		const FFormatArgumentValue* Value = Values.Find(Name);
		return Value && Value->GetType() == EFormatArgumentType::Text ? Value->GetTextValue().ToString() : FString();
	}

	/** The number of the report value Name, or INDEX_NONE when it is missing or not a number. */
	int64 GetNumber(const FFormatNamedArguments& Values, const TCHAR* Name)
	{
		const FFormatArgumentValue* Value = Values.Find(Name);
		return Value && Value->GetType() == EFormatArgumentType::Int ? Value->GetIntValue() : INDEX_NONE;
	}

	TEST_CLASS(Calendar, "Vestige")
	{
		TEST_METHOD(DayOneIsTheFirstDayOfTheFirstSeason)
		{
			const FVestigeCalendar Calendar = MakeCalendar();

			const FVestigeDate Date = Calendar.GetDate(1);

			ASSERT_THAT(IsTrue(Date.Day == 1));
			ASSERT_THAT(IsTrue(Date.Year == 1));
			ASSERT_THAT(IsTrue(Date.SeasonIndex == 0));
			ASSERT_THAT(IsTrue(Date.Season == Calendar.GetSeasons()[0].Tag));
			ASSERT_THAT(IsTrue(Date.DayOfSeason == 1));
			ASSERT_THAT(IsTrue(Date.IsFirstDayOfSeason()));
			ASSERT_THAT(IsTrue(Date.HasSeason()));
		}

		TEST_METHOD(DaysLeftAreTheDaysAfterToday)
		{
			const FVestigeCalendar Calendar = MakeCalendar();

			ASSERT_THAT(IsTrue(Calendar.GetDate(1).DaysLeft == DaysPerSeason - 1));
			ASSERT_THAT(IsTrue(Calendar.GetDate(3).DaysLeft == DaysPerSeason - 3));
			ASSERT_THAT(IsTrue(Calendar.GetDate(DaysPerSeason - 1).DaysLeft == 1));
			ASSERT_THAT(IsTrue(Calendar.GetDate(DaysPerSeason).DaysLeft == 0));
			ASSERT_THAT(IsFalse(Calendar.GetDate(DaysPerSeason).IsFirstDayOfSeason()));
		}

		TEST_METHOD(TheNextSeasonStartsAfterTheLastDay)
		{
			const FVestigeCalendar Calendar = MakeCalendar();

			const FVestigeDate Last = Calendar.GetDate(DaysPerSeason);
			ASSERT_THAT(IsTrue(Last.SeasonIndex == 0));
			ASSERT_THAT(IsTrue(Last.DayOfSeason == DaysPerSeason));

			const FVestigeDate Next = Calendar.GetDate(DaysPerSeason + 1);
			ASSERT_THAT(IsTrue(Next.SeasonIndex == 1));
			ASSERT_THAT(IsTrue(Next.Season == Calendar.GetSeasons()[1].Tag));
			ASSERT_THAT(IsTrue(Next.DayOfSeason == 1));
			ASSERT_THAT(IsTrue(Next.DaysLeft == DaysPerSeason - 1));
			ASSERT_THAT(IsTrue(Next.IsFirstDayOfSeason()));

			const FVestigeDate LastOfTheYear = Calendar.GetDate(DaysPerSeason * Calendar.GetSeasons().Num());
			ASSERT_THAT(IsTrue(LastOfTheYear.SeasonIndex == Calendar.GetSeasons().Num() - 1));
			ASSERT_THAT(IsTrue(LastOfTheYear.DaysLeft == 0));
			ASSERT_THAT(IsTrue(LastOfTheYear.Year == 1));
		}

		TEST_METHOD(ANewYearStartsAfterTheLastSeason)
		{
			const FVestigeCalendar Calendar = MakeCalendar();

			const FVestigeDate Date = Calendar.GetDate(DaysPerSeason * Calendar.GetSeasons().Num() + 1);

			ASSERT_THAT(IsTrue(Date.Year == 2));
			ASSERT_THAT(IsTrue(Date.SeasonIndex == 0));
			ASSERT_THAT(IsTrue(Date.DayOfSeason == 1));
			ASSERT_THAT(IsTrue(Date.IsFirstDayOfSeason()));
		}

		TEST_METHOD(ADayBeforeOneIsDayOne)
		{
			const FVestigeCalendar Calendar = MakeCalendar();

			for (const int32 Day : {0, -5})
			{
				const FVestigeDate Date = Calendar.GetDate(Day);
				ASSERT_THAT(IsTrue(Date.Day == 1));
				ASSERT_THAT(IsTrue(Date.SeasonIndex == 0));
				ASSERT_THAT(IsTrue(Date.DayOfSeason == 1));
				ASSERT_THAT(IsTrue(Date.Year == 1));
			}
			ASSERT_THAT(IsTrue(GetNumber(Calendar.GetReportValues(-5), TEXT("Day")) == 1));
		}

		TEST_METHOD(ACalendarWithoutSeasonsStillCountsTheDays)
		{
			const FVestigeCalendar Calendar = FVestigeCalendar::MakeForTests(DaysPerSeason, {});

			const FVestigeDate Date = Calendar.GetDate(25);

			ASSERT_THAT(IsFalse(Date.HasSeason()));
			ASSERT_THAT(IsTrue(Date.SeasonIndex == INDEX_NONE));
			ASSERT_THAT(IsFalse(Date.Season.IsValid()));
			ASSERT_THAT(IsTrue(Date.Year == 1));
			ASSERT_THAT(IsTrue(Date.Day == 25));
			ASSERT_THAT(IsTrue(Date.DayOfSeason == 5));
			ASSERT_THAT(IsTrue(Date.DaysLeft == DaysPerSeason - 5));
			ASSERT_THAT(IsTrue(Calendar.GetSeasonName(Date).IsEmpty()));
			ASSERT_THAT(IsTrue(GetText(Calendar.GetReportValues(25), TEXT("Season")).IsEmpty()));
		}

		TEST_METHOD(ACalendarWithNoDaysCountsOneDayASeason)
		{
			const FVestigeCalendar Calendar = FVestigeCalendar::MakeForTests(
				0, {MakeSeason(TEXT("Vestige.Season.Spring"), TEXT("Alpha")), MakeSeason(TEXT("Vestige.Season.Summer"), TEXT("Beta"))});

			ASSERT_THAT(IsTrue(Calendar.GetDaysPerSeason() == 1));
			const FVestigeDate Second = Calendar.GetDate(2);
			ASSERT_THAT(IsTrue(Second.SeasonIndex == 1));
			ASSERT_THAT(IsTrue(Second.DayOfSeason == 1));
			ASSERT_THAT(IsTrue(Second.DaysLeft == 0));
		}

		TEST_METHOD(FindsASeasonByItsTag)
		{
			const FVestigeCalendar Calendar = MakeCalendar();

			const FVestigeSeason* Summer = Calendar.FindSeason(FGameplayTag::RequestGameplayTag(FName(TEXT("Vestige.Season.Summer"))));
			ASSERT_THAT(IsNotNull(Summer));
			ASSERT_THAT(AreEqual(Summer->DisplayName.ToString(), FString(TEXT("Beta"))));

			// The parent tag and the empty tag are no season.
			ASSERT_THAT(IsNull(Calendar.FindSeason(VestigeGameplayTags::Season)));
			ASSERT_THAT(IsNull(Calendar.FindSeason(FGameplayTag())));
		}

		TEST_METHOD(TheReportValuesNameTheSeason)
		{
			const FVestigeCalendar Calendar = MakeCalendar();

			const FFormatNamedArguments Values = Calendar.GetReportValues(DaysPerSeason + 1);

			ASSERT_THAT(IsTrue(GetNumber(Values, TEXT("Day")) == DaysPerSeason + 1));
			ASSERT_THAT(AreEqual(GetText(Values, TEXT("Season")), FString(TEXT("Beta"))));
			ASSERT_THAT(IsTrue(GetNumber(Values, TEXT("DayOfSeason")) == 1));
			ASSERT_THAT(IsTrue(Values.Find(TEXT("PreviousSeason")) == nullptr));
		}

		TEST_METHOD(TheSeasonStartValuesNameThePreviousSeason)
		{
			const FVestigeCalendar Calendar = MakeCalendar();

			const FFormatNamedArguments Summer = Calendar.GetSeasonStartValues(DaysPerSeason + 1);
			ASSERT_THAT(IsTrue(GetNumber(Summer, TEXT("Day")) == DaysPerSeason + 1));
			ASSERT_THAT(AreEqual(GetText(Summer, TEXT("Season")), FString(TEXT("Beta"))));
			ASSERT_THAT(AreEqual(GetText(Summer, TEXT("PreviousSeason")), FString(TEXT("Alpha"))));
			ASSERT_THAT(IsTrue(GetNumber(Summer, TEXT("DayOfSeason")) == 1));

			// The first season of a new year follows the last one.
			const FFormatNamedArguments NewYear = Calendar.GetSeasonStartValues(DaysPerSeason * Calendar.GetSeasons().Num() + 1);
			ASSERT_THAT(AreEqual(GetText(NewYear, TEXT("Season")), FString(TEXT("Alpha"))));
			ASSERT_THAT(AreEqual(GetText(NewYear, TEXT("PreviousSeason")), FString(TEXT("Delta"))));

			// Day 1 has no season before it, so its own season is named.
			ASSERT_THAT(AreEqual(GetText(Calendar.GetSeasonStartValues(1), TEXT("PreviousSeason")), FString(TEXT("Alpha"))));
		}
	};
}

#endif
