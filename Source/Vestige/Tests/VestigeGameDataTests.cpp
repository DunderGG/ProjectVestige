// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/VestigeGameplayTags.h"
#include "Content/VestigeAssetManager.h"
#include "Content/VestigeCalendar.h"
#include "Content/VestigeGameData.h"
#include "CQTest.h"
#include "GameplayEffect.h"
#include "Tests/VestigeTestTimedDrainEffect.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace VestigeGameDataTests
{
	TEST_CLASS(GameData, "Vestige")
	{
		TEST_METHOD(NamesEveryEffect)
		{
			// Loaded by the asset manager at startup, from GameDataPath in Config/DefaultGame.ini.
			const UVestigeGameData* Loaded = UVestigeAssetManager::GetGameData();
			ASSERT_THAT(IsNotNull(Loaded));
			ASSERT_THAT(AreEqual(Loaded->GetPathName(), FString(TEXT("/Game/Vestige/DA_GameData.DA_GameData"))));
			ASSERT_THAT(IsTrue(&UVestigeGameData::Get() == Loaded));

			ASSERT_THAT(IsNotNull(Loaded->GetStaminaCostEffect().Get()));
			ASSERT_THAT(IsNotNull(Loaded->GetStaminaRefillEffect().Get()));
			ASSERT_THAT(IsNotNull(Loaded->GetLateNightDrainEffect().Get()));
			ASSERT_THAT(IsNotNull(Loaded->GetExhaustedEffect().Get()));
#if WITH_EDITOR
			FDataValidationContext Context;
			ASSERT_THAT(IsTrue(Loaded->IsDataValid(Context) != EDataValidationResult::Invalid));
			ASSERT_THAT(AreEqual(Context.GetNumErrors(), 0u));

			// The drain is applied once per game minute, and the asset itself is valid (warnings allowed).
			const UGameplayEffect* Drain = Loaded->GetLateNightDrainEffect().GetDefaultObject();
			ASSERT_THAT(IsTrue(Drain->DurationPolicy == EGameplayEffectDurationType::Instant));
			FDataValidationContext DrainContext;
			Drain->IsDataValid(DrainContext);
			ASSERT_THAT(AreEqual(DrainContext.GetNumErrors(), 0u));
#endif
		}

		TEST_METHOD(TheCalendarHasFourSeasonsOf28Days)
		{
			// The one test that names the game's calendar; the others take it from the data.
			const FVestigeCalendar& Calendar = UVestigeGameData::Get().GetCalendar();

			ASSERT_THAT(IsTrue(Calendar.GetDaysPerSeason() == 28));
			const TArray<FString> ExpectedTags = {
				TEXT("Vestige.Season.Spring"), TEXT("Vestige.Season.Summer"), TEXT("Vestige.Season.Autumn"), TEXT("Vestige.Season.Winter")};
			ASSERT_THAT(IsTrue(Calendar.GetSeasons().Num() == ExpectedTags.Num()));
			for (int32 Index = 0; Index < ExpectedTags.Num(); ++Index)
			{
				const FVestigeSeason& Season = Calendar.GetSeasons()[Index];
				ASSERT_THAT(AreEqual(Season.Tag.ToString(), ExpectedTags[Index]));
				ASSERT_THAT(IsFalse(Season.DisplayName.IsEmpty(), ExpectedTags[Index]));
				ASSERT_THAT(IsFalse(Season.NameInSentence.IsEmpty(), ExpectedTags[Index]));
			}
		}

#if WITH_EDITOR
		/** A calendar of two valid seasons, which a test then breaks in one way. */
		static FVestigeSeason MakeSeason(const TCHAR* TagName, const TCHAR* DisplayName)
		{
			FVestigeSeason Season;
			Season.Tag = FGameplayTag::RequestGameplayTag(FName(TagName), false);
			Season.DisplayName = FText::FromString(DisplayName);
			Season.NameInSentence = FText::FromString(FString(DisplayName).ToLower());
			return Season;
		}

		/** The errors the calendar's validation reports. */
		static uint32 CountCalendarErrors(const FVestigeCalendar& Calendar)
		{
			FDataValidationContext Context;
			const EDataValidationResult Result = Calendar.Validate(Context);
			const uint32 Errors = Context.GetNumErrors();
			return (Errors > 0) == (Result == EDataValidationResult::Invalid) ? Errors : MAX_uint32;
		}

		TEST_METHOD(AnInvalidCalendarIsReported)
		{
			const FVestigeSeason Spring = MakeSeason(TEXT("Vestige.Season.Spring"), TEXT("Spring"));
			const FVestigeSeason Summer = MakeSeason(TEXT("Vestige.Season.Summer"), TEXT("Summer"));
			ASSERT_THAT(IsTrue(CountCalendarErrors(FVestigeCalendar::MakeForTests(28, {Spring, Summer})) == 0u));

			ASSERT_THAT(AreEqual(CountCalendarErrors(FVestigeCalendar::MakeForTests(28, {})), 1u, TEXT("no seasons")));
			ASSERT_THAT(AreEqual(CountCalendarErrors(FVestigeCalendar::MakeForTests(0, {Spring, Summer})), 1u, TEXT("no days")));

			FVestigeSeason NoTag = Summer;
			NoTag.Tag = FGameplayTag();
			ASSERT_THAT(AreEqual(CountCalendarErrors(FVestigeCalendar::MakeForTests(28, {Spring, NoTag})), 1u, TEXT("empty tag")));

			FVestigeSeason Parent = Summer;
			Parent.Tag = VestigeGameplayTags::Season;
			ASSERT_THAT(AreEqual(CountCalendarErrors(FVestigeCalendar::MakeForTests(28, {Spring, Parent})), 1u, TEXT("the parent tag")));

			FVestigeSeason Outside = Summer;
			Outside.Tag = VestigeGameplayTags::Feedback_Day_Slept;
			ASSERT_THAT(AreEqual(CountCalendarErrors(FVestigeCalendar::MakeForTests(28, {Spring, Outside})), 1u, TEXT("a tag outside")));

			ASSERT_THAT(AreEqual(CountCalendarErrors(FVestigeCalendar::MakeForTests(28, {Spring, Spring})), 1u, TEXT("a repeated tag")));

			FVestigeSeason NoName = Summer;
			NoName.DisplayName = FText::GetEmpty();
			ASSERT_THAT(AreEqual(CountCalendarErrors(FVestigeCalendar::MakeForTests(28, {Spring, NoName})), 1u, TEXT("empty name")));

			FVestigeSeason NoSentenceName = Summer;
			NoSentenceName.NameInSentence = FText::GetEmpty();
			ASSERT_THAT(AreEqual(
				CountCalendarErrors(FVestigeCalendar::MakeForTests(28, {Spring, NoSentenceName})), 1u, TEXT("empty name in sentence")));
		}

		TEST_METHOD(TheGameDataReportsItsInvalidCalendar)
		{
			const UVestigeGameData& Loaded = UVestigeGameData::Get();
			UVestigeGameData* NoSeasons = NewObject<UVestigeGameData>();
			NoSeasons->SetEffectsForTests(Loaded.GetStaminaCostEffect(), Loaded.GetStaminaRefillEffect(), Loaded.GetLateNightDrainEffect(),
				Loaded.GetExhaustedEffect());
			NoSeasons->SetCalendarForTests(FVestigeCalendar::MakeForTests(28, {}));

			FDataValidationContext Context;
			ASSERT_THAT(IsTrue(NoSeasons->IsDataValid(Context) == EDataValidationResult::Invalid));
			ASSERT_THAT(AreEqual(Context.GetNumErrors(), 1u));
		}

		TEST_METHOD(ADrainThatIsntInstantIsInvalid)
		{
			const UVestigeGameData& Loaded = UVestigeGameData::Get();
			ASSERT_THAT(IsNotNull(Loaded.GetExhaustedEffect().Get()));

			// Applied every game minute, an infinite effect would leave one more active effect each minute.
			UVestigeGameData* InfiniteDrain = NewObject<UVestigeGameData>();
			InfiniteDrain->SetCalendarForTests(Loaded.GetCalendar());
			InfiniteDrain->SetEffectsForTests(
				Loaded.GetStaminaCostEffect(), Loaded.GetStaminaRefillEffect(), Loaded.GetExhaustedEffect(), Loaded.GetExhaustedEffect());
			FDataValidationContext InfiniteContext;
			ASSERT_THAT(IsTrue(InfiniteDrain->IsDataValid(InfiniteContext) == EDataValidationResult::Invalid));
			ASSERT_THAT(AreEqual(InfiniteContext.GetNumErrors(), 1u));

			// A periodic drain with a duration would drain on real time, on top of the clock's minutes.
			UVestigeGameData* TimedDrain = NewObject<UVestigeGameData>();
			TimedDrain->SetCalendarForTests(Loaded.GetCalendar());
			TimedDrain->SetEffectsForTests(Loaded.GetStaminaCostEffect(), Loaded.GetStaminaRefillEffect(),
				UVestigeTestTimedDrainEffect::StaticClass(), Loaded.GetExhaustedEffect());
			FDataValidationContext TimedContext;
			ASSERT_THAT(IsTrue(TimedDrain->IsDataValid(TimedContext) == EDataValidationResult::Invalid));
			ASSERT_THAT(AreEqual(TimedContext.GetNumErrors(), 1u));

			// Without effects, each one is reported.
			UVestigeGameData* Empty = NewObject<UVestigeGameData>();
			Empty->SetCalendarForTests(Loaded.GetCalendar());
			FDataValidationContext EmptyContext;
			ASSERT_THAT(IsTrue(Empty->IsDataValid(EmptyContext) == EDataValidationResult::Invalid));
			ASSERT_THAT(AreEqual(EmptyContext.GetNumErrors(), 4u));
		}
#endif
	};
}

#endif
