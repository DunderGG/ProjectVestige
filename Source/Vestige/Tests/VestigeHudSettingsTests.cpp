// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/ActorTestSpawner.h"
#include "CQTest.h"
#include "UI/VestigeHUD.h"
#include "UI/VestigeHudSettings.h"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace VestigeHudSettingsTests
{
	/** The settings are tuning, so these check the limits and where the HUD reads them, never their values. */
	TEST_CLASS(HudSettings, "Vestige")
	{
		FActorTestSpawner Spawner;

		TEST_METHOD(TheMessageCapStaysInItsRange)
		{
			ASSERT_THAT(IsTrue(UVestigeHudSettings::ClampMaxMessages(0) == VestigeHudLimits::FewestMessages));
			ASSERT_THAT(IsTrue(UVestigeHudSettings::ClampMaxMessages(5) == 5));
			ASSERT_THAT(
				IsTrue(UVestigeHudSettings::ClampMaxMessages(VestigeHudLimits::MostMessages + 1) == VestigeHudLimits::MostMessages));
		}

		TEST_METHOD(TheProjectSettingsAreInRange)
		{
			const UVestigeHudSettings& Settings = UVestigeHudSettings::Get();

			ASSERT_THAT(IsTrue(Settings.GetMaxMessages() >= VestigeHudLimits::FewestMessages));
			ASSERT_THAT(IsTrue(Settings.GetMaxMessages() <= VestigeHudLimits::MostMessages));
			ASSERT_THAT(IsTrue(Settings.GetMessageSeconds() >= VestigeHudLimits::ShortestMessageSeconds));
		}

		TEST_METHOD(TheHudTakesItsMessageCapFromTheSettings)
		{
			AVestigeHUD& Hud = Spawner.SpawnActor<AVestigeHUD>();
			Hud.DispatchBeginPlay();

			ASSERT_THAT(IsTrue(Hud.GetMaxMessages() == UVestigeHudSettings::Get().GetMaxMessages()));
		}
	};
}

#endif
