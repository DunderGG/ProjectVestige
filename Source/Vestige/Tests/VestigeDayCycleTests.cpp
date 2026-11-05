// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/ActorTestSpawner.h"
#include "CQTest.h"
#include "Engine/DirectionalLight.h"
#include "Time/VestigeClockSubsystem.h"
#include "Time/VestigeDayCycle.h"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace VestigeDayCycleTests
{
	TEST_CLASS(DayCycle, "Vestige")
	{
		FActorTestSpawner Spawner;

		/** The test game instance's clock, which the day cycle follows. */
		UVestigeClockSubsystem* Clock = nullptr;

		BEFORE_EACH()
		{
			Spawner.InitializeGameSubsystems();
			Clock = UVestigeClockSubsystem::Get(&Spawner.GetWorld());
			ASSERT_THAT(IsNotNull(Clock));
		}

		/** Runs the clock forward by GameHours, then lets the day cycle follow it, the way a frame does. */
		void PassHours(AVestigeDayCycle& Cycle, float GameHours)
		{
			const float Seconds = GameHours * Clock->GetRealSecondsPerGameHour();
			Clock->Tick(Seconds);
			Cycle.TickActor(Seconds, LEVELTICK_All, Cycle.PrimaryActorTick);
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

			// One and a half game seconds per frame keep each frame's movement well under a step, day and night.
			constexpr int32 FramesPerGameHour = 2400;
			const int32 Frames = FMath::FloorToInt32((Clock->GetDayEndHour() - Clock->GetDayStartHour()) * FramesPerGameHour);
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

		TEST_METHOD(SunFollowsTheClockOnTheNextFrameOfANewDay)
		{
			ADirectionalLight* Sun = nullptr;
			AVestigeDayCycle& Cycle = SpawnCycleWithSun(Sun);
			PassHours(Cycle, 9.f);

			Clock->StartNextDay();
			PassHours(Cycle, 0.f);

			ASSERT_THAT(IsTrue(DegreesBetween(Sun->GetActorQuat(), Cycle.GetSunRotation().Quaternion()) < RoundingDegrees));
		}
	};
}

#endif
