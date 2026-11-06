// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/ActorTestSpawner.h"
#include "CQTest.h"
#include "Tests/VestigeTestEventRecorder.h"
#include "Time/VestigeClockSaveData.h"
#include "Time/VestigeClockSettings.h"
#include "Time/VestigeClockSubsystem.h"
#include "UObject/StrongObjectPtr.h"

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
			CountBroadcasts(GameClock->OnDayEndReached, Broadcasts);

			PassHours(GameClock->GetDayEndHour() - GameClock->GetDayStartHour() - 1.f);
			ASSERT_THAT(IsTrue(Broadcasts == 0));

			PassHours(2.f);
			PassHours(1.f);
			ASSERT_THAT(IsTrue(Broadcasts == 1));
		}

		TEST_METHOD(DayEndIsBroadcastAgainTheNextDay)
		{
			int32 Broadcasts = 0;
			CountBroadcasts(GameClock->OnDayEndReached, Broadcasts);
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
			CountBroadcasts(GameClock->OnDayEndReached, Broadcasts);
			CountBroadcasts(GameClock->OnHourStarted, Broadcasts);

			GameClock->RestoreFromSaveData(MakeClock(0, GameClock->GetDayEndHour() + 5.f));

			ASSERT_THAT(IsTrue(GameClock->GetCurrentDay() == 1));
			ASSERT_THAT(IsTrue(GameClock->GetTimeOfDay() == GameClock->GetDayEndHour()));
			ASSERT_THAT(IsTrue(Broadcasts == 0));
		}

		TEST_METHOD(ClockRestoredAtTheDayEndStillEndsTheDay)
		{
			int32 Broadcasts = 0;
			CountBroadcasts(GameClock->OnDayEndReached, Broadcasts);

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
			CountBroadcasts(GameClock->OnDayEndReached, DayEnds);

			ASSERT_THAT(IsTrue(GameClock->AdvanceTo(GameClock->GetDayEndHour() + 5.f)));

			ASSERT_THAT(IsTrue(GameClock->GetTimeOfDay() == GameClock->GetDayEndHour()));
			ASSERT_THAT(IsTrue(DayEnds == 1));
			ASSERT_THAT(IsFalse(GameClock->IsRunning()));
			ASSERT_THAT(IsFalse(GameClock->AdvanceTo(GameClock->GetDayEndHour() + 6.f)));
		}
	};
}

#endif
