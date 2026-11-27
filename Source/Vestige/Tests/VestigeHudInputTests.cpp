// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "Blueprint/UserWidget.h"
#include "Components/ActorTestSpawner.h"
#include "CQTest.h"
#include "UI/VestigeHudWidget.h"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace VestigeHudInputTests
{
	/**
	 * Common UI gives the input config to the leaf-most active widget, and FindLeafmostActiveChild skips a widget that does not support
	 * activation focus. The HUD is the game layer's widget, so when a screen (the dialogue box) closes the HUD has to support it, or
	 * the Menu config stays and the farmer can't move.
	 */
	TEST_CLASS(HudInput, "Vestige")
	{
		FActorTestSpawner Spawner;

		TEST_METHOD(ActiveHudCanTakeTheInputConfigBack)
		{
			UClass* HudClass = LoadClass<UVestigeHudWidget>(nullptr, TEXT("/Game/Vestige/UI/WBP_Hud.WBP_Hud_C"));
			ASSERT_THAT(IsNotNull(HudClass));
			UVestigeHudWidget* Hud = CreateWidget<UVestigeHudWidget>(&Spawner.GetWorld(), HudClass);
			ASSERT_THAT(IsNotNull(Hud));

			Hud->ActivateWidget();

			ASSERT_THAT(IsTrue(Hud->SupportsActivationFocus()));
		}
	};
} // namespace VestigeHudInputTests

#endif // WITH_DEV_AUTOMATION_TESTS
