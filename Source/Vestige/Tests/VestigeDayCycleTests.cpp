// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/ActorTestSpawner.h"
#include "CQTest.h"
#include "Engine/DirectionalLight.h"
#include "Tests/VestigeTestEventRecorder.h"
#include "Time/VestigeClockSaveData.h"
#include "Time/VestigeDayCycle.h"
#include "UObject/StrongObjectPtr.h"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace VestigeDayCycleTests
{
	TEST_CLASS(DayCycle, "Vestige")
	{
		FActorTestSpawner Spawner;

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

		TEST_METHOD(BlueprintsHearOfTheNewDayAfterTheFarmAndBeforeTheSave)
		{
			AVestigeDayCycle& Cycle = Spawner.SpawnActor<AVestigeDayCycle>();
			const TStrongObjectPtr<UVestigeTestEventRecorder> Recorder(NewObject<UVestigeTestEventRecorder>());
			UVestigeTestEventRecorder* Listener = Recorder.Get();
			Cycle.OnNewDay.AddLambda(
				[Listener](int32 Day)
				{
					Listener->Record(TEXT("Farm"));
				});
			Cycle.OnNewDayBlueprint.AddDynamic(Listener, &UVestigeTestEventRecorder::RecordNewDay);
			Cycle.OnNewDayFinished.AddLambda(
				[Listener](int32 Day)
				{
					Listener->Record(TEXT("Save"));
				});

			Cycle.StartNextDay();

			const TArray<FString> Expected = {TEXT("Farm"), TEXT("NewDay 2"), TEXT("Save")};
			ASSERT_THAT(IsTrue(Recorder->GetEvents() == Expected));
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
			CountBroadcasts(Cycle.OnHourStarted, Broadcasts);

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

		TEST_METHOD(EachWholeHourIsBroadcastOnceInOrder)
		{
			AVestigeDayCycle& Cycle = Spawner.SpawnActor<AVestigeDayCycle>();
			TArray<int32> Hours;
			Cycle.OnHourStarted.AddLambda(
				[&Hours](int32 Hour)
				{
					Hours.Add(Hour);
				});
			const int32 StartHour = FMath::FloorToInt32(Cycle.GetDayStartHour());

			PassHours(Cycle, 0.5f);
			ASSERT_THAT(IsTrue(Hours.IsEmpty()));

			// One long frame that passes three hours still broadcasts each of them.
			PassHours(Cycle, 3.f);
			const TArray<int32> Expected = {StartHour + 1, StartHour + 2, StartHour + 3};
			ASSERT_THAT(IsTrue(Hours == Expected));
		}

		TEST_METHOD(HoursAfterMidnightCountOnFromTwentyFour)
		{
			AVestigeDayCycle& Cycle = Spawner.SpawnActor<AVestigeDayCycle>();
			TArray<int32> Hours;
			Cycle.OnHourStarted.AddLambda(
				[&Hours](int32 Hour)
				{
					Hours.Add(Hour);
				});

			PassHours(Cycle, Cycle.GetDayEndHour());

			ASSERT_THAT(IsTrue(Hours.Contains(24)));
			ASSERT_THAT(IsTrue(Hours.Last() == FMath::FloorToInt32(Cycle.GetDayEndHour())));
		}

		/** A day cycle that moves a directional light placed before it, as in L_Farm. */
		AVestigeDayCycle& SpawnCycleWithSun(ADirectionalLight*& OutSun)
		{
			OutSun = &Spawner.SpawnActor<ADirectionalLight>();
			AVestigeDayCycle& Cycle = Spawner.SpawnActor<AVestigeDayCycle>();
			Cycle.DispatchBeginPlay();
			return Cycle;
		}

		/** Allowed error from storing a rotation on the light, in degrees: far below a step. */
		static constexpr double RoundingDegrees = 1e-4;

		static double DegreesBetween(const FQuat& A, const FQuat& B) { return FMath::RadiansToDegrees(A.AngularDistance(B)); }

		TEST_METHOD(SunWaitsUntilTheClockHasMovedItAStep)
		{
			ADirectionalLight* Sun = nullptr;
			AVestigeDayCycle& Cycle = SpawnCycleWithSun(Sun);
			const FQuat Start = Sun->GetActorQuat();

			// A few game seconds move the sun far less than a step.
			PassHours(Cycle, 0.001f);

			ASSERT_THAT(IsTrue(DegreesBetween(Start, Cycle.GetSunRotation().Quaternion()) > 0.0));
			ASSERT_THAT(IsTrue(Sun->GetActorQuat().Equals(Start, 0.0)));
		}

		TEST_METHOD(SunTurnsInStepsAndStaysWithinAStepOfTheClock)
		{
			ADirectionalLight* Sun = nullptr;
			AVestigeDayCycle& Cycle = SpawnCycleWithSun(Sun);
			const double Step = Cycle.GetSunStepDegrees();
			ASSERT_THAT(IsTrue(Step > 0.0));

			// Fifteen game seconds per frame keep each frame's movement well under a step, day and night.
			constexpr int32 FramesPerGameHour = 240;
			const int32 Frames = FMath::FloorToInt32((Cycle.GetDayEndHour() - Cycle.GetDayStartHour()) * FramesPerGameHour);
			int32 Turns = 0;
			for (int32 Frame = 0; Frame < Frames; ++Frame)
			{
				const FQuat Before = Sun->GetActorQuat();
				PassHours(Cycle, 1.f / FramesPerGameHour);
				if (!Sun->GetActorQuat().Equals(Before, 0.0))
				{
					++Turns;
					ASSERT_THAT(IsTrue(DegreesBetween(Before, Sun->GetActorQuat()) >= Step - RoundingDegrees));
				}
				ASSERT_THAT(IsTrue(DegreesBetween(Sun->GetActorQuat(), Cycle.GetSunRotation().Quaternion()) < Step));
			}

			ASSERT_THAT(IsTrue(Turns > 0));
			ASSERT_THAT(IsTrue(Turns < Frames / 2));
		}

		TEST_METHOD(SunFollowsTheClockAtOnceOnANewDay)
		{
			ADirectionalLight* Sun = nullptr;
			AVestigeDayCycle& Cycle = SpawnCycleWithSun(Sun);
			PassHours(Cycle, 9.f);

			Cycle.StartNextDay();

			ASSERT_THAT(IsTrue(DegreesBetween(Sun->GetActorQuat(), Cycle.GetSunRotation().Quaternion()) < RoundingDegrees));
		}

		TEST_METHOD(StartingTheNextDayBroadcastsNoHour)
		{
			AVestigeDayCycle& Cycle = Spawner.SpawnActor<AVestigeDayCycle>();
			PassHours(Cycle, 2.5f);
			int32 Broadcasts = 0;
			CountBroadcasts(Cycle.OnHourStarted, Broadcasts);

			Cycle.StartNextDay();

			ASSERT_THAT(IsTrue(Broadcasts == 0));
		}
	};
}

#endif
