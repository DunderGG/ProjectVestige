// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Content/VestigeCalendar.h"

#include "AbilitySystem/VestigeGameplayTags.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "VestigeCalendar"

FVestigeDate FVestigeCalendar::GetDate(int32 Day) const
{
	const int32 Days = GetDaysPerSeason();
	const int32 DayIndex = FMath::Max(Day, 1) - 1;

	FVestigeDate Date;
	Date.Day = DayIndex + 1;
	Date.DayOfSeason = DayIndex % Days + 1;
	Date.DaysLeft = Days - Date.DayOfSeason;
	if (Seasons.IsEmpty())
	{
		return Date;
	}

	const int32 SeasonCount = DayIndex / Days;
	Date.SeasonIndex = SeasonCount % Seasons.Num();
	Date.Season = Seasons[Date.SeasonIndex].Tag;
	Date.Year = SeasonCount / Seasons.Num() + 1;
	return Date;
}

const FVestigeSeason* FVestigeCalendar::FindSeason(const FGameplayTag& Tag) const
{
	return Seasons.FindByPredicate(
		[&Tag](const FVestigeSeason& Season)
		{
			return Season.Tag == Tag;
		});
}

FText FVestigeCalendar::GetSeasonName(const FVestigeDate& Date) const
{
	return Seasons.IsValidIndex(Date.SeasonIndex) ? Seasons[Date.SeasonIndex].DisplayName : FText::GetEmpty();
}

FText FVestigeCalendar::GetSeasonNameInSentence(const FVestigeDate& Date) const
{
	return Seasons.IsValidIndex(Date.SeasonIndex) ? Seasons[Date.SeasonIndex].NameInSentence : FText::GetEmpty();
}

EVestigeDayKind FVestigeCalendar::GetDayKind(int32 Day) const
{
	const FVestigeDate Date = GetDate(Day);

	// GetDate counts a day before 1 as day 1, so day 1's day before is in its own season.
	if (Date.HasSeason() && Date.IsFirstDayOfSeason() && GetDate(Date.Day - 1).SeasonIndex != Date.SeasonIndex)
	{
		return EVestigeDayKind::NewSeason;
	}
	return Date.IsLastDayOfSeason() ? EVestigeDayKind::LastDay : EVestigeDayKind::Usual;
}

FFormatNamedArguments FVestigeCalendar::GetReportValues(int32 Day) const
{
	const FVestigeDate Date = GetDate(Day);
	FFormatNamedArguments Values;
	Values.Add(TEXT("Day"), Date.Day);
	Values.Add(TEXT("Season"), GetSeasonName(Date));
	Values.Add(TEXT("SeasonInSentence"), GetSeasonNameInSentence(Date));
	Values.Add(TEXT("DayOfSeason"), Date.DayOfSeason);
	Values.Add(TEXT("DaysLeft"), Date.DaysLeft);
	return Values;
}

FFormatNamedArguments FVestigeCalendar::GetSeasonStartValues(int32 Day) const
{
	// GetDate counts a day before 1 as day 1, so day 1's previous season is its own.
	FFormatNamedArguments Values = GetReportValues(Day);
	Values.Add(TEXT("PreviousSeason"), GetSeasonName(GetDate(Day - 1)));
	return Values;
}

#if WITH_EDITOR
EDataValidationResult FVestigeCalendar::Validate(FDataValidationContext& Context) const
{
	EDataValidationResult Result = EDataValidationResult::Valid;
	const auto AddError = [&Context, &Result](const FText& Error)
	{
		Context.AddError(Error);
		Result = EDataValidationResult::Invalid;
	};

	if (Seasons.IsEmpty())
	{
		AddError(LOCTEXT("NoSeasons", "The calendar has no seasons, so the date shows only the day count."));
	}
	if (DaysPerSeason < 1)
	{
		AddError(FText::Format(
			LOCTEXT("NoDays", "The calendar's Days Per Season is {0}; a season needs at least 1 day."), FText::AsNumber(DaysPerSeason)));
	}

	const FGameplayTag ParentTag = VestigeGameplayTags::Season;
	TSet<FGameplayTag> SeenTags;
	for (int32 Index = 0; Index < Seasons.Num(); ++Index)
	{
		const FVestigeSeason& Season = Seasons[Index];
		const FText IndexText = FText::AsNumber(Index);
		if (!Season.Tag.IsValid())
		{
			AddError(FText::Format(LOCTEXT("NoTag", "Season {0} of the calendar has no tag."), IndexText));
		}
		else if (Season.Tag == ParentTag || !Season.Tag.MatchesTag(ParentTag))
		{
			AddError(FText::Format(LOCTEXT("TagNotASeason", "Season {0} of the calendar has the tag {1}, which isn't a tag under {2}."),
				IndexText, FText::FromName(Season.Tag.GetTagName()), FText::FromName(ParentTag.GetTagName())));
		}
		else if (SeenTags.Contains(Season.Tag))
		{
			AddError(FText::Format(LOCTEXT("RepeatedTag", "Season {0} of the calendar repeats the tag {1}; each season needs its own."),
				IndexText, FText::FromName(Season.Tag.GetTagName())));
		}
		SeenTags.Add(Season.Tag);

		if (Season.DisplayName.IsEmpty())
		{
			AddError(FText::Format(LOCTEXT("NoName", "Season {0} of the calendar has no Display Name."), IndexText));
		}
		if (Season.NameInSentence.IsEmpty())
		{
			AddError(FText::Format(LOCTEXT("NoNameInSentence", "Season {0} of the calendar has no Name In Sentence."), IndexText));
		}
	}
	return Result;
}
#endif

#if WITH_DEV_AUTOMATION_TESTS
FVestigeCalendar FVestigeCalendar::MakeForTests(int32 InDaysPerSeason, TArray<FVestigeSeason> InSeasons)
{
	FVestigeCalendar Calendar;
	Calendar.DaysPerSeason = InDaysPerSeason;
	Calendar.Seasons = MoveTemp(InSeasons);
	return Calendar;
}
#endif

#undef LOCTEXT_NAMESPACE
