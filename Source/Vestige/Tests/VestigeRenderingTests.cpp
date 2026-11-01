// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "CQTest.h"
#include "Misc/ConfigCacheIni.h"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace VestigeRenderingTests
{
	/** Checks the rendering settings for the target PC (Architecture.md, section 10), as the engine reads them. */
	TEST_CLASS(Rendering, "Vestige")
	{
		static int32 GetHardwareRayTracing(const TCHAR* Level)
		{
			int32 Value = INDEX_NONE;
			GConfig->GetInt(
				*FString::Printf(TEXT("GlobalIlluminationQuality@%s"), Level), TEXT("r.Lumen.HardwareRayTracing"), Value, GScalabilityIni);
			return Value;
		}

		TEST_METHOD(HardwareRayTracingIsOnlyOnAtEpicAndCinematic)
		{
			ASSERT_THAT(IsTrue(GetHardwareRayTracing(TEXT("0")) == 0));
			ASSERT_THAT(IsTrue(GetHardwareRayTracing(TEXT("1")) == 0));
			ASSERT_THAT(IsTrue(GetHardwareRayTracing(TEXT("2")) == 0));
			ASSERT_THAT(IsTrue(GetHardwareRayTracing(TEXT("3")) == 1));
			ASSERT_THAT(IsTrue(GetHardwareRayTracing(TEXT("Cine")) == 1));
		}
	};
}

#endif
