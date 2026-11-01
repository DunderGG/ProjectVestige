// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/ActorTestSpawner.h"
#include "CQTest.h"
#include "Farming/VestigeRestComponent.h"
#include "Time/VestigeClockSaveData.h"
#include "Time/VestigeDayCycle.h"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace VestigeRestTests
{
	TEST_CLASS(Rest, "Vestige")
	{
		FActorTestSpawner Spawner;

		/** A day cycle and an actor with a rest component that has begun play, so it listens to the day cycle. */
		struct FRestSetup
		{
			AVestigeDayCycle* Cycle = nullptr;
			UVestigeRestComponent* RestComponent = nullptr;
		};

		FRestSetup MakeRestingActor()
		{
			FRestSetup Setup;
			Setup.Cycle = &Spawner.SpawnActor<AVestigeDayCycle>();
			AActor& Owner = Spawner.SpawnActor<AActor>();
			Setup.RestComponent = NewObject<UVestigeRestComponent>(&Owner);
			Setup.RestComponent->RegisterComponent();
			Owner.DispatchBeginPlay();
			return Setup;
		}

		TEST_METHOD(SleepingStartsTheNextDay)
		{
			const FRestSetup Setup = MakeRestingActor();

			Setup.RestComponent->Sleep();

			ASSERT_THAT(IsTrue(Setup.Cycle->GetCurrentDay() == 2));
		}

		TEST_METHOD(ReachingTheDayEndPassesOutIntoTheNextDay)
		{
			const FRestSetup Setup = MakeRestingActor();

			FVestigeClockSaveData LateClock;
			LateClock.TimeOfDay = Setup.Cycle->GetDayEndHour();
			Setup.Cycle->RestoreFromSaveData(LateClock);
			Setup.Cycle->TickActor(0.1f, LEVELTICK_All, Setup.Cycle->PrimaryActorTick);

			ASSERT_THAT(IsTrue(Setup.Cycle->GetCurrentDay() == 2));
			ASSERT_THAT(IsTrue(Setup.Cycle->GetTimeOfDay() == Setup.Cycle->GetDayStartHour()));
		}
	};
}

#endif
