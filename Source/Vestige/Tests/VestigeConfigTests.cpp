// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "CQTest.h"
#include "GameFramework/InputSettings.h"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace VestigeConfigTests
{
	/** Checks project settings that look safe to change but aren't, as the engine reads them. */
	TEST_CLASS(Config, "Vestige")
	{
		TEST_METHOD(MotionControlsStayEnabledInTheInputSettings)
		{
			// Every player controller enables motion controls, and with them off in the input settings the engine logs a
			// warning each time. They only read the tilt of phones and motion controllers, so leaving them on costs nothing.
			ASSERT_THAT(IsTrue(GetDefault<UInputSettings>()->bEnableMotionControls));
		}

		TEST_METHOD(TheConsoleOpensWithTildeAndSection)
		{
			// Tilde is the console key on a US keyboard; Section, the section sign, is the key under Esc on a Swedish one.
			const TArray<FKey>& ConsoleKeys = GetDefault<UInputSettings>()->ConsoleKeys;

			ASSERT_THAT(IsTrue(ConsoleKeys.Contains(EKeys::Tilde)));
			ASSERT_THAT(IsTrue(ConsoleKeys.Contains(EKeys::Section)));
		}
	};
}

#endif
