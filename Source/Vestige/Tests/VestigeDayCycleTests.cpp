// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/ActorTestSpawner.h"
#include "CQTest.h"
#include "Time/VestigeClockSaveData.h"
#include "Time/VestigeDayCycle.h"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace VestigeDayCycleTests
{
	TEST_CLASS(DayCycle, "Vestige")
	{
		FActorTestSpawner Spawner;

		/** Counts every broadcast of Event into Count, and remembers the last day it carried in LastDay. */
		static void CountBroadcasts(FOnVestigeNewDay& Event, int32& Count, int32* LastDay = nullptr)
		{
			Event.AddLambda(
				[&Count, LastDay](int32 Day)
				{
					++Count;
					if (LastDay)
					{
						*LastDay = Day;
					}
				});
		}

		static FVestigeClockSaveData MakeClock(int32 Day, float TimeOfDay)
		{
			FVestigeClockSaveData Clock;
			Clock.Day = Day;
			Clock.TimeOfDay = TimeOfDay;
			return Clock;
		}

		/** Runs the clock forward by GameHours, the way a frame's tick does. */
		static void PassHours(AVestigeDayCycle& Cycle, float GameHours)
		{
			Cycle.TickActor(GameHours * Cycle.GetRealSecondsPerGameHour(), LEVELTICK_All, Cycle.PrimaryActorTick);
		}

		TEST_METHOD(StartingTheNextDayAdvancesTheDayAndResetsTheClock)
		{
			AVestigeDayCycle& Cycle = Spawner.SpawnActor<AVestigeDayCycle>();
			int32 Broadcasts = 0;
			int32 NewDay = 0;
			CountBroadcasts(Cycle.OnNewDay, Broadcasts, &NewDay);
			PassHours(Cycle, 5.f);

			Cycle.StartNextDay();

			ASSERT_THAT(IsTrue(Cycle.GetCurrentDay() == 2));
			ASSERT_THAT(IsTrue(Broadcasts == 1));
			ASSERT_THAT(IsTrue(NewDay == 2));
			ASSERT_THAT(IsTrue(Cycle.GetTimeOfDay() == Cycle.GetDayStartHour()));
		}

		TEST_METHOD(ClockStopsAtTheDayEnd)
		{
			AVestigeDayCycle& Cycle = Spawner.SpawnActor<AVestigeDayCycle>();

			PassHours(Cycle, Cycle.GetDayEndHour());

			ASSERT_THAT(IsTrue(Cycle.GetTimeOfDay() == Cycle.GetDayEndHour()));
		}

		TEST_METHOD(DayEndIsBroadcastOnceWhenTheClockReachesIt)
		{
			AVestigeDayCycle& Cycle = Spawner.SpawnActor<AVestigeDayCycle>();
			int32 Broadcasts = 0;
			CountBroadcasts(Cycle.OnDayEndReached, Broadcasts);

			PassHours(Cycle, Cycle.GetDayEndHour() - Cycle.GetDayStartHour() - 1.f);
			ASSERT_THAT(IsTrue(Broadcasts == 0));

			PassHours(Cycle, 2.f);
			PassHours(Cycle, 1.f);
			ASSERT_THAT(IsTrue(Broadcasts == 1));
		}

		TEST_METHOD(DayEndIsBroadcastAgainTheNextDay)
		{
			AVestigeDayCycle& Cycle = Spawner.SpawnActor<AVestigeDayCycle>();
			int32 Broadcasts = 0;
			CountBroadcasts(Cycle.OnDayEndReached, Broadcasts);
			PassHours(Cycle, Cycle.GetDayEndHour());

			Cycle.StartNextDay();
			PassHours(Cycle, Cycle.GetDayEndHour());

			ASSERT_THAT(IsTrue(Broadcasts == 2));
		}

		TEST_METHOD(RestoringTheClockClampsItAndBroadcastsNothing)
		{
			AVestigeDayCycle& Cycle = Spawner.SpawnActor<AVestigeDayCycle>();
			int32 Broadcasts = 0;
			CountBroadcasts(Cycle.OnNewDay, Broadcasts);
			CountBroadcasts(Cycle.OnNewDayFinished, Broadcasts);
			CountBroadcasts(Cycle.OnDayEndReached, Broadcasts);

			Cycle.RestoreFromSaveData(MakeClock(0, Cycle.GetDayEndHour() + 5.f));

			ASSERT_THAT(IsTrue(Cycle.GetCurrentDay() == 1));
			ASSERT_THAT(IsTrue(Cycle.GetTimeOfDay() == Cycle.GetDayEndHour()));
			ASSERT_THAT(IsTrue(Broadcasts == 0));
		}

		TEST_METHOD(ClockRestoredAtTheDayEndStillEndsTheDay)
		{
			AVestigeDayCycle& Cycle = Spawner.SpawnActor<AVestigeDayCycle>();
			int32 Broadcasts = 0;
			CountBroadcasts(Cycle.OnDayEndReached, Broadcasts);

			Cycle.RestoreFromSaveData(MakeClock(3, Cycle.GetDayEndHour()));
			PassHours(Cycle, 0.1f);

			ASSERT_THAT(IsTrue(Broadcasts == 1));
		}
	};
}

#endif
