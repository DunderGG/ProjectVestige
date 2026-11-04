// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "CQTest.h"
#include "HAL/IConsoleManager.h"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace VestigeRenderingTests
{
	/** Checks the rendering settings for the target PC (Architecture.md, section 10), as the engine reads them. */
	TEST_CLASS(Rendering, "Vestige")
	{
		TEST_METHOD(HardwareRayTracingIsOff)
		{
			const IConsoleVariable* RayTracing = IConsoleManager::Get().FindConsoleVariable(TEXT("r.RayTracing"));
			ASSERT_THAT(IsNotNull(RayTracing));
			ASSERT_THAT(IsTrue(RayTracing->GetInt() == 0));
		}
	};
}

#endif
