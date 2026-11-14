// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/ActorTestSpawner.h"
#include "Content/VestigeCalendar.h"
#include "Content/VestigeGameData.h"
#include "CQTest.h"
#include "Tests/VestigeTestEventRecorder.h"
#include "Time/VestigeClockSaveData.h"
#include "Time/VestigeClockSettings.h"
#include "Time/VestigeClockSubsystem.h"
#include "UObject/StrongObjectPtr.h"

#include <cmath>

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace VestigeClockTests
{
	TEST_CLASS(Clock, "Vestige")
	{
		FActorTestSpawner Spawner;

		/** The test game instance's clock, as the game has one. */
		UVestigeClockSubsystem* GameClock = nullptr;

		BEFORE_EACH()
		{
			Spawner.InitializeGameSubsystems();
			GameClock = UVestigeClockSubsystem::Get(&Spawner.GetWorld());
			ASSERT_THAT(IsNotNull(GameClock));
		}

		/** Counts every broadcast of Event into Count, and remembers the last day or hour it carried in LastValue. */
		template <typename EventType> static void CountBroadcasts(EventType& Event, int32& Count, int32* LastValue = nullptr)
		{
			Event.AddLambda(
				[&Count, LastValue](int32 Value)
				{
					++Count;
					if (LastValue)
					{
						*LastValue = Value;
					}
				});
		}

		/** Counts every broadcast of the clock's OnDayEnded into Count, and remembers the last one in LastDayEnd. */
		void CountDayEnds(int32& Count, FVestigeDayEnd* LastDayEnd = nullptr)
		{
			GameClock->OnDayEnded.AddLambda(
				[&Count, LastDayEnd](const FVestigeDayEnd& DayEnd)
				{
					++Count;
					if (LastDayEnd)
					{
						*LastDayEnd = DayEnd;
					}
				});
		}

		static FVestigeClockSaveData MakeClock(int32 Day, float TimeOfDay)
		{
			FVestigeClockSaveData Data;
			Data.Day = Day;
			Data.TimeOfDay = TimeOfDay;
			return Data;
		}

		/** Runs the clock forward by GameHours, the way a frame's tick does. */
		void PassHours(float GameHours) { GameClock->Tick(GameHours * GameClock->GetRealSecondsPerGameHour()); }

		TEST_METHOD(TheClockRunsWithItsGameInstancesWorld)
		{
			ASSERT_THAT(IsTrue(GameClock->GetTickableGameObjectWorld() == &Spawner.GetWorld()));
			ASSERT_THAT(IsTrue(GameClock->IsTickable()));
			ASSERT_THAT(IsTrue(GameClock->GetCurrentDay() == 1));
			ASSERT_THAT(IsTrue(GameClock->GetTimeOfDay() == GameClock->GetDayStartHour()));
		}

		TEST_METHOD(SunriseAndSunsetComeFromTheProjectSettings)
		{
			const UVestigeClockSettings& Settings = UVestigeClockSettings::Get();
			ASSERT_THAT(IsTrue(GameClock->GetSunriseHour() == Settings.GetSunriseHour()));
			ASSERT_THAT(IsTrue(GameClock->GetSunsetHour() == Settings.GetSunsetHour()));
			ASSERT_THAT(IsTrue(GameClock->GetSunsetHour() > GameClock->GetSunriseHour()));
		}

		TEST_METHOD(StartingTheNextDayAdvancesTheDayAndResetsTheClock)
		{
			int32 Broadcasts = 0;
			int32 NewDay = 0;
			CountBroadcasts(GameClock->OnNewDay, Broadcasts, &NewDay);
			PassHours(5.f);

			GameClock->StartNextDay();

			ASSERT_THAT(IsTrue(GameClock->GetCurrentDay() == 2));
			ASSERT_THAT(IsTrue(Broadcasts == 1));
			ASSERT_THAT(IsTrue(NewDay == 2));
			ASSERT_THAT(IsTrue(GameClock->GetTimeOfDay() == GameClock->GetDayStartHour()));
		}

		TEST_METHOD(BlueprintsHearOfTheNewDayAfterTheFarmAndBeforeTheSave)
		{
			const TStrongObjectPtr<UVestigeTestEventRecorder> Recorder(NewObject<UVestigeTestEventRecorder>());
			UVestigeTestEventRecorder* Listener = Recorder.Get();
			GameClock->OnNewDay.AddLambda(
				[Listener](int32 Day)
				{
					Listener->Record(TEXT("Farm"));
				});
			GameClock->OnNewDayBlueprint.AddDynamic(Listener, &UVestigeTestEventRecorder::RecordNewDay);
			GameClock->OnNewDayFinished.AddLambda(
				[Listener](int32 Day)
				{
					Listener->Record(TEXT("Save"));
				});

			GameClock->StartNextDay();

			const TArray<FString> Expected = {TEXT("Farm"), TEXT("NewDay 2"), TEXT("Save")};
			ASSERT_THAT(IsTrue(Recorder->GetEvents() == Expected));
		}

		TEST_METHOD(ClockStopsAtTheDayEnd)
		{
			PassHours(GameClock->GetDayEndHour());

			ASSERT_THAT(IsTrue(GameClock->GetTimeOfDay() == GameClock->GetDayEndHour()));
			ASSERT_THAT(IsFalse(GameClock->IsRunning()));
			ASSERT_THAT(IsFalse(GameClock->IsTickable()));
		}

		TEST_METHOD(DayEndIsBroadcastOnceWhenTheClockReachesIt)
		{
			int32 Broadcasts = 0;
			CountDayEnds(Broadcasts);

			PassHours(GameClock->GetDayEndHour() - GameClock->GetDayStartHour() - 1.f);
			ASSERT_THAT(IsTrue(Broadcasts == 0));

			PassHours(2.f);
			PassHours(1.f);
			ASSERT_THAT(IsTrue(Broadcasts == 1));
		}

		TEST_METHOD(DayEndIsBroadcastAgainTheNextDay)
		{
			int32 Broadcasts = 0;
			CountDayEnds(Broadcasts);
			PassHours(GameClock->GetDayEndHour());

			GameClock->StartNextDay();
			ASSERT_THAT(IsTrue(GameClock->IsRunning()));
			PassHours(GameClock->GetDayEndHour());

			ASSERT_THAT(IsTrue(Broadcasts == 2));
		}

		TEST_METHOD(RestoringTheClockClampsItAndBroadcastsNothing)
		{
			int32 Broadcasts = 0;
			CountBroadcasts(GameClock->OnNewDay, Broadcasts);
			CountBroadcasts(GameClock->OnNewDayFinished, Broadcasts);
			CountDayEnds(Broadcasts);
			CountBroadcasts(GameClock->OnHourStarted, Broadcasts);

			GameClock->RestoreFromSaveData(MakeClock(0, GameClock->GetDayEndHour() + 5.f));

			ASSERT_THAT(IsTrue(GameClock->GetCurrentDay() == 1));
			ASSERT_THAT(IsTrue(GameClock->GetTimeOfDay() == GameClock->GetDayEndHour()));
			ASSERT_THAT(IsTrue(Broadcasts == 0));
		}

		TEST_METHOD(ClockRestoredAtTheDayEndStillEndsTheDay)
		{
			int32 Broadcasts = 0;
			CountDayEnds(Broadcasts);

			GameClock->RestoreFromSaveData(MakeClock(3, GameClock->GetDayEndHour()));
			PassHours(0.1f);

			ASSERT_THAT(IsTrue(Broadcasts == 1));
		}

		TEST_METHOD(ClockRoundTripsThroughTheSave)
		{
			GameClock->RestoreFromSaveData(MakeClock(4, 13.5f));
			const FVestigeClockSaveData Saved = GameClock->GetSaveData();
			GameClock->StartNewGame();

			GameClock->RestoreFromSaveData(Saved);

			ASSERT_THAT(IsTrue(GameClock->GetCurrentDay() == 4));
			ASSERT_THAT(IsTrue(GameClock->GetTimeOfDay() == 13.5f));
		}

		TEST_METHOD(ANewGameStartsOnTheMorningOfDayOneWithoutBroadcasting)
		{
			GameClock->StartNextDay();
			PassHours(GameClock->GetDayEndHour());
			int32 Broadcasts = 0;
			CountBroadcasts(GameClock->OnNewDay, Broadcasts);
			CountBroadcasts(GameClock->OnHourStarted, Broadcasts);

			GameClock->StartNewGame();

			ASSERT_THAT(IsTrue(GameClock->GetCurrentDay() == 1));
			ASSERT_THAT(IsTrue(GameClock->GetTimeOfDay() == GameClock->GetDayStartHour()));
			ASSERT_THAT(IsTrue(GameClock->IsRunning()));
			ASSERT_THAT(IsTrue(Broadcasts == 0));
		}

		TEST_METHOD(EachWholeHourIsBroadcastOnceInOrder)
		{
			TArray<int32> Hours;
			GameClock->OnHourStarted.AddLambda(
				[&Hours](int32 Hour)
				{
					Hours.Add(Hour);
				});
			const int32 StartHour = FMath::FloorToInt32(GameClock->GetDayStartHour());

			PassHours(0.5f);
			ASSERT_THAT(IsTrue(Hours.IsEmpty()));

			// One long frame that passes three hours still broadcasts each of them.
			PassHours(3.f);
			const TArray<int32> Expected = {StartHour + 1, StartHour + 2, StartHour + 3};
			ASSERT_THAT(IsTrue(Hours == Expected));
		}

		TEST_METHOD(HoursAfterMidnightCountOnFromTwentyFour)
		{
			TArray<int32> Hours;
			GameClock->OnHourStarted.AddLambda(
				[&Hours](int32 Hour)
				{
					Hours.Add(Hour);
				});

			PassHours(GameClock->GetDayEndHour());

			ASSERT_THAT(IsTrue(Hours.Contains(24)));
			ASSERT_THAT(IsTrue(Hours.Last() == FMath::FloorToInt32(GameClock->GetDayEndHour())));
		}

		TEST_METHOD(StartingTheNextDayBroadcastsNoHour)
		{
			PassHours(2.5f);
			int32 Broadcasts = 0;
			CountBroadcasts(GameClock->OnHourStarted, Broadcasts);

			GameClock->StartNextDay();

			ASSERT_THAT(IsTrue(Broadcasts == 0));
		}

		TEST_METHOD(AdvancingTheClockBroadcastsEachHourItPasses)
		{
			TArray<int32> Hours;
			GameClock->OnHourStarted.AddLambda(
				[&Hours](int32 Hour)
				{
					Hours.Add(Hour);
				});

			ASSERT_THAT(IsTrue(GameClock->AdvanceTo(22.f)));

			ASSERT_THAT(IsTrue(GameClock->GetTimeOfDay() == 22.f));
			ASSERT_THAT(IsTrue(Hours.Num() == 22 - FMath::FloorToInt32(GameClock->GetDayStartHour())));
			ASSERT_THAT(IsTrue(Hours.Last() == 22));
		}

		TEST_METHOD(TheClockOnlyAdvancesForward)
		{
			GameClock->AdvanceTo(20.f);
			int32 Broadcasts = 0;
			CountBroadcasts(GameClock->OnHourStarted, Broadcasts);

			ASSERT_THAT(IsFalse(GameClock->AdvanceTo(19.f)));
			ASSERT_THAT(IsFalse(GameClock->AdvanceTo(20.f)));

			ASSERT_THAT(IsTrue(GameClock->GetTimeOfDay() == 20.f));
			ASSERT_THAT(IsTrue(Broadcasts == 0));
		}

		TEST_METHOD(AdvancingPastTheDayEndStopsTheClockThere)
		{
			int32 DayEnds = 0;
			CountDayEnds(DayEnds);

			ASSERT_THAT(IsTrue(GameClock->AdvanceTo(GameClock->GetDayEndHour() + 5.f)));

			ASSERT_THAT(IsTrue(GameClock->GetTimeOfDay() == GameClock->GetDayEndHour()));
			ASSERT_THAT(IsTrue(DayEnds == 1));
			ASSERT_THAT(IsFalse(GameClock->IsRunning()));
			ASSERT_THAT(IsFalse(GameClock->AdvanceTo(GameClock->GetDayEndHour() + 6.f)));
		}

		TEST_METHOD(EndingTheDayEarlyStopsTheClockWhereItIs)
		{
			int32 DayEnds = 0;
			FVestigeDayEnd DayEnd;
			CountDayEnds(DayEnds, &DayEnd);
			GameClock->RestoreFromSaveData(MakeClock(4, 22.5f));

			ASSERT_THAT(IsTrue(GameClock->EndDay()));

			ASSERT_THAT(IsFalse(GameClock->IsRunning()));
			ASSERT_THAT(IsTrue(GameClock->GetTimeOfDay() == 22.5f));
			ASSERT_THAT(IsTrue(DayEnds == 1));
			ASSERT_THAT(IsTrue(DayEnd.Day == 4));
			ASSERT_THAT(IsTrue(DayEnd.TimeOfDay == 22.5f));
			ASSERT_THAT(IsTrue(DayEnd.bEndedEarly));
			ASSERT_THAT(IsTrue(GameClock->GetLastDayEnd().Day == 4));
		}

		TEST_METHOD(ADayEndsOnlyOnce)
		{
			int32 DayEnds = 0;
			CountDayEnds(DayEnds);
			ASSERT_THAT(IsTrue(GameClock->EndDay()));

			ASSERT_THAT(IsFalse(GameClock->EndDay()));
			ASSERT_THAT(IsFalse(GameClock->AdvanceTo(GameClock->GetTimeOfDay() + 1.f)));
			PassHours(GameClock->GetDayEndHour());

			ASSERT_THAT(IsTrue(DayEnds == 1));
		}

		TEST_METHOD(ReachingTheDayEndIsNotEndingItEarly)
		{
			int32 DayEnds = 0;
			FVestigeDayEnd DayEnd;
			CountDayEnds(DayEnds, &DayEnd);

			PassHours(GameClock->GetDayEndHour());

			ASSERT_THAT(IsTrue(DayEnds == 1));
			ASSERT_THAT(IsFalse(DayEnd.bEndedEarly));
			ASSERT_THAT(IsTrue(DayEnd.TimeOfDay == GameClock->GetDayEndHour()));
		}

		TEST_METHOD(TheNewDayKnowsHowTheLastOneEnded)
		{
			float EndedAt = 0.f;
			GameClock->OnDayEnded.AddLambda(
				[this](const FVestigeDayEnd& DayEnd)
				{
					GameClock->StartNextDay();
				});
			GameClock->OnNewDay.AddLambda(
				[this, &EndedAt](int32 Day)
				{
					EndedAt = GameClock->GetLastDayEnd().TimeOfDay;
				});
			GameClock->RestoreFromSaveData(MakeClock(1, 25.f));

			GameClock->EndDay();

			ASSERT_THAT(IsTrue(GameClock->GetCurrentDay() == 2));
			ASSERT_THAT(IsTrue(EndedAt == 25.f));
		}

		TEST_METHOD(AnHourThatEndsTheDayIsTheLastHourBroadcast)
		{
			// A rule that ends the day at an hour, with the next day started at once as the game mode does.
			TArray<int32> Hours;
			GameClock->OnHourStarted.AddLambda(
				[this, &Hours](int32 Hour)
				{
					Hours.Add(Hour);
					if (Hour == 25)
					{
						GameClock->EndDay();
					}
				});
			GameClock->OnDayEnded.AddLambda(
				[this](const FVestigeDayEnd& DayEnd)
				{
					GameClock->StartNextDay();
				});
			GameClock->RestoreFromSaveData(MakeClock(1, 24.5f));

			PassHours(3.f);

			const TArray<int32> Expected = {25};
			ASSERT_THAT(IsTrue(Hours == Expected));
			ASSERT_THAT(IsTrue(GameClock->GetCurrentDay() == 2));
			ASSERT_THAT(IsTrue(GameClock->GetTimeOfDay() == GameClock->GetDayStartHour()));
		}

		/** Records every minute, hour and day end the clock broadcasts, in order, as "M420", "H7" and "End". */
		void RecordTime(TArray<FString>& Events)
		{
			GameClock->OnMinuteStarted.AddLambda(
				[&Events](int32 Minute)
				{
					Events.Add(FString::Printf(TEXT("M%d"), Minute));
				});
			GameClock->OnHourStarted.AddLambda(
				[&Events](int32 Hour)
				{
					Events.Add(FString::Printf(TEXT("H%d"), Hour));
				});
			GameClock->OnDayEnded.AddLambda(
				[&Events](const FVestigeDayEnd& DayEnd)
				{
					Events.Add(TEXT("End"));
				});
		}

		TEST_METHOD(EachWholeMinuteIsBroadcastOnceInOrder)
		{
			TArray<int32> Minutes;
			GameClock->OnMinuteStarted.AddLambda(
				[&Minutes](int32 Minute)
				{
					Minutes.Add(Minute);
				});
			const int32 StartMinute = VestigeClock::ToWholeMinutes(GameClock->GetDayStartHour());

			PassHours(0.5f / VestigeClock::MinutesPerHour);
			ASSERT_THAT(IsTrue(Minutes.IsEmpty()));

			PassHours(3.f / VestigeClock::MinutesPerHour);
			const TArray<int32> Expected = {StartMinute + 1, StartMinute + 2, StartMinute + 3};
			ASSERT_THAT(IsTrue(Minutes == Expected));
		}

		TEST_METHOD(AMinuteThatStartsAnHourComesBeforeTheHour)
		{
			TArray<FString> Events;
			RecordTime(Events);
			GameClock->RestoreFromSaveData(MakeClock(1, 6.99f));

			PassHours(0.02f);

			const TArray<FString> Expected = {TEXT("M420"), TEXT("H7")};
			ASSERT_THAT(IsTrue(Events == Expected));
		}

		TEST_METHOD(TheLastMinuteAndHourComeBeforeTheDayEnds)
		{
			TArray<FString> Events;
			RecordTime(Events);
			GameClock->RestoreFromSaveData(MakeClock(1, GameClock->GetDayEndHour() - 0.01f));

			PassHours(1.f);

			const int32 EndHour = FMath::FloorToInt32(GameClock->GetDayEndHour());
			const TArray<FString> Expected = {
				FString::Printf(TEXT("M%d"), EndHour * VestigeClock::MinutesPerHour), FString::Printf(TEXT("H%d"), EndHour), TEXT("End")};
			ASSERT_THAT(IsTrue(Events == Expected));
		}

		TEST_METHOD(AClockJustBeforeAnHourBroadcastsNoHour)
		{
			TArray<FString> Events;
			RecordTime(Events);
			GameClock->RestoreFromSaveData(MakeClock(1, 6.99f));

			ASSERT_THAT(IsTrue(GameClock->AdvanceTo(std::nextafter(7.f, 0.f))));

			ASSERT_THAT(IsTrue(Events.IsEmpty()));
		}

		TEST_METHOD(AdvancingTheClockBroadcastsEachMinuteItPasses)
		{
			TArray<int32> Minutes;
			GameClock->OnMinuteStarted.AddLambda(
				[&Minutes](int32 Minute)
				{
					Minutes.Add(Minute);
				});
			const float From = GameClock->GetTimeOfDay();

			ASSERT_THAT(IsTrue(GameClock->AdvanceTo(From + 2.f)));

			ASSERT_THAT(IsTrue(Minutes.Num() == 2 * VestigeClock::MinutesPerHour));
			ASSERT_THAT(IsTrue(Minutes[0] == VestigeClock::ToWholeMinutes(From) + 1));
			ASSERT_THAT(IsTrue(Minutes.Last() == VestigeClock::ToWholeMinutes(From + 2.f)));
		}

		TEST_METHOD(RestoringOrStartingADayBroadcastsNoMinute)
		{
			PassHours(2.5f);
			int32 Broadcasts = 0;
			CountBroadcasts(GameClock->OnMinuteStarted, Broadcasts);

			GameClock->StartNextDay();
			GameClock->RestoreFromSaveData(MakeClock(3, 20.f));

			ASSERT_THAT(IsTrue(Broadcasts == 0));
		}

		TEST_METHOD(AMinuteThatEndsTheDayIsTheLastTimeBroadcast)
		{
			// A rule that ends the day at a minute, with the next day started at once as the game mode does.
			TArray<int32> Minutes;
			GameClock->OnMinuteStarted.AddLambda(
				[this, &Minutes](int32 Minute)
				{
					Minutes.Add(Minute);
					if (Minute == 510)
					{
						GameClock->EndDay();
					}
				});
			int32 Hours = 0;
			CountBroadcasts(GameClock->OnHourStarted, Hours);
			GameClock->OnDayEnded.AddLambda(
				[this](const FVestigeDayEnd& DayEnd)
				{
					GameClock->StartNextDay();
				});
			GameClock->RestoreFromSaveData(MakeClock(1, 8.42f));

			PassHours(3.f);

			const TArray<int32> Expected = {506, 507, 508, 509, 510};
			ASSERT_THAT(IsTrue(Minutes == Expected));
			ASSERT_THAT(IsTrue(Hours == 0));
			ASSERT_THAT(IsTrue(GameClock->GetCurrentDay() == 2));
			ASSERT_THAT(IsTrue(GameClock->GetTimeOfDay() == GameClock->GetDayStartHour()));
		}

		/** The last day of the game's first season, from the clock's calendar (the data may tune it). */
		int32 GetLastDayOfFirstSeason() const { return GameClock->GetCalendar().GetDaysPerSeason(); }

		/** Counts every season the clock starts into Count, and remembers the last one in LastDate. */
		void CountSeasonStarts(int32& Count, FVestigeDate* LastDate = nullptr)
		{
			GameClock->OnSeasonStarted.AddLambda(
				[&Count, LastDate](const FVestigeDate& Date)
				{
					++Count;
					if (LastDate)
					{
						*LastDate = Date;
					}
				});
		}

		TEST_METHOD(TheClocksCalendarIsTheGameDatas)
		{
			const FVestigeCalendar& Calendar = GameClock->GetCalendar();
			const FVestigeCalendar& Data = UVestigeGameData::Get().GetCalendar();

			ASSERT_THAT(IsTrue(Calendar.GetSeasons().Num() > 1));
			ASSERT_THAT(IsTrue(Calendar.GetSeasons().Num() == Data.GetSeasons().Num()));
			ASSERT_THAT(IsTrue(Calendar.GetDaysPerSeason() == Data.GetDaysPerSeason()));
			ASSERT_THAT(IsTrue(Calendar.GetSeasons()[0].Tag == Data.GetSeasons()[0].Tag));
		}

		TEST_METHOD(TheDateFollowsTheDayCount)
		{
			const FVestigeCalendar& Calendar = GameClock->GetCalendar();
			const int32 Days = Calendar.GetDaysPerSeason();

			const FVestigeDate First = GameClock->GetDate();
			ASSERT_THAT(IsTrue(First.Day == 1));
			ASSERT_THAT(IsTrue(First.SeasonIndex == 0));
			ASSERT_THAT(IsTrue(First.Season == Calendar.GetSeasons()[0].Tag));
			ASSERT_THAT(IsTrue(First.DayOfSeason == 1));
			ASSERT_THAT(IsTrue(First.DaysLeft == Days - 1));

			GameClock->StartNextDay();
			ASSERT_THAT(IsTrue(GameClock->GetDate().DayOfSeason == 2));
			ASSERT_THAT(IsTrue(GameClock->GetDate().Day == 2));
		}

		TEST_METHOD(TheDateComesBackWithTheSavedDay)
		{
			const FVestigeCalendar& Calendar = GameClock->GetCalendar();
			const int32 Days = Calendar.GetDaysPerSeason();
			GameClock->RestoreFromSaveData(MakeClock(Days + 2, 13.5f));
			const FVestigeClockSaveData Saved = GameClock->GetSaveData();
			GameClock->StartNewGame();

			GameClock->RestoreFromSaveData(Saved);

			const FVestigeDate Date = GameClock->GetDate();
			ASSERT_THAT(IsTrue(Date.Day == Days + 2));
			ASSERT_THAT(IsTrue(Date.SeasonIndex == 1));
			ASSERT_THAT(IsTrue(Date.Season == Calendar.GetSeasons()[1].Tag));
			ASSERT_THAT(IsTrue(Date.DayOfSeason == 2));
			ASSERT_THAT(IsTrue(Date.DaysLeft == Days - 2));
		}

		TEST_METHOD(ASeasonStartsOnItsFirstMorningBeforeTheNewDay)
		{
			const FVestigeCalendar& Calendar = GameClock->GetCalendar();
			const int32 Last = GetLastDayOfFirstSeason();
			const TStrongObjectPtr<UVestigeTestEventRecorder> Recorder(NewObject<UVestigeTestEventRecorder>());
			UVestigeTestEventRecorder* Listener = Recorder.Get();
			FVestigeDate Started;
			GameClock->OnSeasonStarted.AddLambda(
				[Listener, &Started](const FVestigeDate& Date)
				{
					Listener->Record(TEXT("Season"));
					Started = Date;
				});
			GameClock->OnNewDay.AddLambda(
				[Listener](int32 Day)
				{
					Listener->Record(TEXT("NewDay"));
				});
			GameClock->RestoreFromSaveData(MakeClock(Last, 20.f));

			GameClock->StartNextDay();

			const TArray<FString> Expected = {TEXT("Season"), TEXT("NewDay")};
			ASSERT_THAT(IsTrue(Recorder->GetEvents() == Expected));
			ASSERT_THAT(IsTrue(Started.Day == Last + 1));
			ASSERT_THAT(IsTrue(Started.Season == Calendar.GetSeasons()[1].Tag));
			ASSERT_THAT(IsTrue(Started.DayOfSeason == 1));
		}

		TEST_METHOD(ANewYearStartsTheFirstSeasonAgain)
		{
			const FVestigeCalendar& Calendar = GameClock->GetCalendar();
			const int32 YearLength = Calendar.GetDaysPerSeason() * Calendar.GetSeasons().Num();
			int32 Starts = 0;
			FVestigeDate Started;
			CountSeasonStarts(Starts, &Started);
			GameClock->RestoreFromSaveData(MakeClock(YearLength, 20.f));

			GameClock->StartNextDay();

			ASSERT_THAT(IsTrue(Starts == 1));
			ASSERT_THAT(IsTrue(Started.Year == 2));
			ASSERT_THAT(IsTrue(Started.Season == Calendar.GetSeasons()[0].Tag));
		}

		TEST_METHOD(NoSeasonStartsOnOtherDays)
		{
			const int32 Last = GetLastDayOfFirstSeason();
			int32 Starts = 0;
			CountSeasonStarts(Starts);

			// The first new day, the day before the last day, the last day and the second day of the next season.
			GameClock->StartNextDay();
			GameClock->RestoreFromSaveData(MakeClock(Last - 2, 20.f));
			GameClock->StartNextDay();
			GameClock->StartNextDay();
			GameClock->RestoreFromSaveData(MakeClock(Last + 1, 20.f));
			GameClock->StartNextDay();

			ASSERT_THAT(IsTrue(Starts == 0));
		}

		TEST_METHOD(ANewGameARestoreOrAJumpStartsNoSeason)
		{
			const int32 Last = GetLastDayOfFirstSeason();
			int32 Starts = 0;
			CountSeasonStarts(Starts);

			GameClock->RestoreFromSaveData(MakeClock(Last + 1, 20.f));
			ASSERT_THAT(IsTrue(GameClock->GetDate().IsFirstDayOfSeason()));
			GameClock->JumpToDay(Last + 1);
			GameClock->StartNewGame();

			ASSERT_THAT(IsTrue(Starts == 0));
		}

		TEST_METHOD(JumpingToADayKeepsTheTimeAndBroadcastsNothing)
		{
			PassHours(2.5f);
			const float Time = GameClock->GetTimeOfDay();
			int32 Broadcasts = 0;
			CountBroadcasts(GameClock->OnNewDay, Broadcasts);
			CountBroadcasts(GameClock->OnNewDayFinished, Broadcasts);
			CountBroadcasts(GameClock->OnMinuteStarted, Broadcasts);
			CountBroadcasts(GameClock->OnHourStarted, Broadcasts);
			CountDayEnds(Broadcasts);
			CountSeasonStarts(Broadcasts);
			const int32 Day = GetLastDayOfFirstSeason() + 4;

			ASSERT_THAT(IsTrue(GameClock->JumpToDay(Day)));

			ASSERT_THAT(IsTrue(GameClock->GetCurrentDay() == Day));
			ASSERT_THAT(IsTrue(GameClock->GetTimeOfDay() == Time));
			ASSERT_THAT(IsTrue(GameClock->IsRunning()));
			ASSERT_THAT(IsTrue(Broadcasts == 0));
			ASSERT_THAT(IsTrue(GameClock->GetDate().SeasonIndex == 1));
		}

		TEST_METHOD(JumpingToADayBeforeOneChangesNothing)
		{
			GameClock->RestoreFromSaveData(MakeClock(5, 12.f));

			ASSERT_THAT(IsFalse(GameClock->JumpToDay(0)));
			ASSERT_THAT(IsFalse(GameClock->JumpToDay(-3)));

			ASSERT_THAT(IsTrue(GameClock->GetCurrentDay() == 5));
			ASSERT_THAT(IsTrue(GameClock->GetTimeOfDay() == 12.f));
		}
	};
}

#endif
