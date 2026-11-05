// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/ActorTestSpawner.h"
#include "CQTest.h"
#include "Engine/DirectionalLight.h"
#include "Time/VestigeClockSaveData.h"
#include "UObject/CoreRedirects.h"
#include "Time/VestigeClockSubsystem.h"
#include "Time/VestigeSkyLighting.h"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace VestigeSkyLightingTests
{
	TEST_CLASS(SkyLighting, "Vestige")
	{
		FActorTestSpawner Spawner;

		/** The test game instance's clock, which the sky lighting follows. */
		UVestigeClockSubsystem* Clock = nullptr;

		BEFORE_EACH()
		{
			Spawner.InitializeGameSubsystems();
			Clock = UVestigeClockSubsystem::Get(&Spawner.GetWorld());
			ASSERT_THAT(IsNotNull(Clock));
		}

		/** Runs the clock forward by GameHours, then lets the sky lighting follow it, the way a frame does. */
		void PassHours(AVestigeSkyLighting& Sky, float GameHours)
		{
			const float Seconds = GameHours * Clock->GetRealSecondsPerGameHour();
			Clock->Tick(Seconds);
			Sky.TickActor(Seconds, LEVELTICK_All, Sky.PrimaryActorTick);
		}

		/** Sky lighting that moves a directional light placed before it, as in L_Farm. */
		AVestigeSkyLighting& SpawnSkyLightingWithSun(ADirectionalLight*& OutSun)
		{
			OutSun = &Spawner.SpawnActor<ADirectionalLight>();
			AVestigeSkyLighting& Sky = Spawner.SpawnActor<AVestigeSkyLighting>();
			Sky.DispatchBeginPlay();
			return Sky;
		}

		/** Allowed error from storing a rotation on the light, in degrees: far below a step. */
		static constexpr double RoundingDegrees = 1e-4;

		static double DegreesBetween(const FQuat& A, const FQuat& B) { return FMath::RadiansToDegrees(A.AngularDistance(B)); }

		TEST_METHOD(SunWaitsUntilTheClockHasMovedItAStep)
		{
			ADirectionalLight* Sun = nullptr;
			AVestigeSkyLighting& Sky = SpawnSkyLightingWithSun(Sun);
			const FQuat Start = Sun->GetActorQuat();

			// A few game seconds move the sun far less than a step.
			PassHours(Sky, 0.001f);

			ASSERT_THAT(IsTrue(DegreesBetween(Start, Sky.GetSunRotation().Quaternion()) > 0.0));
			ASSERT_THAT(IsTrue(Sun->GetActorQuat().Equals(Start, 0.0)));
		}

		TEST_METHOD(SunTurnsInStepsAndStaysWithinAStepOfTheClock)
		{
			ADirectionalLight* Sun = nullptr;
			AVestigeSkyLighting& Sky = SpawnSkyLightingWithSun(Sun);
			const double Step = Sky.GetSunStepDegrees();
			ASSERT_THAT(IsTrue(Step > 0.0));

			// One and a half game seconds per frame keep each frame's movement well under a step, day and night.
			constexpr int32 FramesPerGameHour = 2400;
			const int32 Frames = FMath::FloorToInt32((Clock->GetDayEndHour() - Clock->GetDayStartHour()) * FramesPerGameHour);
			int32 Turns = 0;
			for (int32 Frame = 0; Frame < Frames; ++Frame)
			{
				const FQuat Before = Sun->GetActorQuat();
				PassHours(Sky, 1.f / FramesPerGameHour);
				if (!Sun->GetActorQuat().Equals(Before, 0.0))
				{
					++Turns;
					ASSERT_THAT(IsTrue(DegreesBetween(Before, Sun->GetActorQuat()) >= Step - RoundingDegrees));
				}
				ASSERT_THAT(IsTrue(DegreesBetween(Sun->GetActorQuat(), Sky.GetSunRotation().Quaternion()) < Step));
			}

			ASSERT_THAT(IsTrue(Turns > 0));
			ASSERT_THAT(IsTrue(Turns < Frames / 2));
		}

		TEST_METHOD(SunFollowsTheClockOnTheNextFrameOfANewDay)
		{
			ADirectionalLight* Sun = nullptr;
			AVestigeSkyLighting& Sky = SpawnSkyLightingWithSun(Sun);
			PassHours(Sky, 9.f);

			Clock->StartNextDay();
			PassHours(Sky, 0.f);

			ASSERT_THAT(IsTrue(DegreesBetween(Sun->GetActorQuat(), Sky.GetSunRotation().Quaternion()) < RoundingDegrees));
		}

		/** Degrees above the horizon that Sky puts the sun at Hour on the clock. */
		float SunElevationAt(const AVestigeSkyLighting& Sky, float Hour)
		{
			FVestigeClockSaveData Time;
			Time.Day = Clock->GetCurrentDay();
			Time.TimeOfDay = Hour;
			Clock->RestoreFromSaveData(Time);
			return -Sky.GetSunRotation().Pitch;
		}

		TEST_METHOD(SunRisesAndSetsAtTheClocksHours)
		{
			ADirectionalLight* Sun = nullptr;
			const AVestigeSkyLighting& Sky = SpawnSkyLightingWithSun(Sun);
			const float Sunrise = Clock->GetSunriseHour();
			const float Sunset = Clock->GetSunsetHour();
			const float Midday = (Sunrise + Sunset) / 2.f;
			const float JustAfterSunrise = Sunrise + 0.5f;
			const float JustBeforeSunset = Sunset - 0.5f;

			// The sun climbs from its lowest at sunrise to its highest halfway to sunset, then sinks again.
			const float AtMidday = SunElevationAt(Sky, Midday);
			ASSERT_THAT(IsTrue(SunElevationAt(Sky, Sunrise) < SunElevationAt(Sky, JustAfterSunrise)));
			ASSERT_THAT(IsTrue(SunElevationAt(Sky, JustAfterSunrise) < AtMidday));
			ASSERT_THAT(IsTrue(SunElevationAt(Sky, Midday - 1.f) < AtMidday));
			ASSERT_THAT(IsTrue(SunElevationAt(Sky, Midday + 1.f) < AtMidday));
			ASSERT_THAT(IsTrue(SunElevationAt(Sky, JustBeforeSunset) < AtMidday));
			ASSERT_THAT(IsTrue(SunElevationAt(Sky, Sunset) < SunElevationAt(Sky, JustBeforeSunset)));
		}

		TEST_METHOD(OldClassNamesLoadAsTheSkyLighting)
		{
			// L_Farm places the actor under an older class name until the level is saved again (Config/DefaultEngine.ini).
			const FString NewName = AVestigeSkyLighting::StaticClass()->GetPathName();
			for (const TCHAR* OldName : {TEXT("/Script/Vestige.VestigeDayCycle"), TEXT("/Script/Vestige.FarmDayCycle")})
			{
				const FCoreRedirectObjectName Redirected =
					FCoreRedirects::GetRedirectedName(ECoreRedirectFlags::Type_Class, FCoreRedirectObjectName(OldName));
				ASSERT_THAT(AreEqual(NewName, Redirected.ToString()));
			}
		}
	};
}

#endif
