// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/VolumetricCloudComponent.h"
#include "CQTest.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ConfigCacheIni.h"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace VestigeRenderingTests
{
	/** The clouds' sample caps in one effects scalability level, as written in the scalability config. */
	struct FCloudSampleCaps
	{
		int32 View = 0;
		int32 Shadow = 0;
	};

	/** Reads a level's cloud sample caps. False when the level doesn't set both, so switching to it keeps the old ones. */
	bool ReadCloudSampleCaps(const FString& Level, FCloudSampleCaps& OutCaps)
	{
		const FString Section = TEXT("EffectsQuality@") + Level;
		return GConfig->GetInt(*Section, TEXT("r.VolumetricCloud.ViewRaySampleMaxCount"), OutCaps.View, GScalabilityIni)
			&& GConfig->GetInt(*Section, TEXT("r.VolumetricCloud.Shadow.ViewRaySampleMaxCount"), OutCaps.Shadow, GScalabilityIni);
	}

	/** Checks the rendering settings for the target PC (Architecture.md, section 10), as the engine reads them. */
	TEST_CLASS(Rendering, "Vestige")
	{
		TEST_METHOD(HardwareRayTracingIsOff)
		{
			const IConsoleVariable* RayTracing = IConsoleManager::Get().FindConsoleVariable(TEXT("r.RayTracing"));
			ASSERT_THAT(IsNotNull(RayTracing));
			ASSERT_THAT(IsTrue(RayTracing->GetInt() == 0));
		}

		/** Below Epic the clouds take at most the engine's own sample counts, and never more at a lower level. */
		TEST_METHOD(CloudSamplesAreCappedBelowEpic)
		{
			FCloudSampleCaps HigherLevelCaps;
			HigherLevelCaps.View = static_cast<int32>(UVolumetricCloudComponent::BaseViewRaySampleCount);
			HigherLevelCaps.Shadow = static_cast<int32>(UVolumetricCloudComponent::BaseShadowRaySampleCount);
			for (const TCHAR* Level : {TEXT("2"), TEXT("1"), TEXT("0")})
			{
				FCloudSampleCaps Caps;
				ASSERT_THAT(IsTrue(ReadCloudSampleCaps(Level, Caps)));
				ASSERT_THAT(IsTrue(Caps.View > 0 && Caps.View <= HigherLevelCaps.View));
				ASSERT_THAT(IsTrue(Caps.Shadow > 0 && Caps.Shadow <= HigherLevelCaps.Shadow));
				HigherLevelCaps = Caps;
			}
		}

		/** Epic and Cinematic set the caps back to at least the engine's counts after a lower level lowered them. */
		TEST_METHOD(CloudSamplesAreFullAtEpic)
		{
			for (const TCHAR* Level : {TEXT("3"), TEXT("Cine")})
			{
				FCloudSampleCaps Caps;
				ASSERT_THAT(IsTrue(ReadCloudSampleCaps(Level, Caps)));
				ASSERT_THAT(IsTrue(Caps.View >= UVolumetricCloudComponent::BaseViewRaySampleCount));
				ASSERT_THAT(IsTrue(Caps.Shadow >= UVolumetricCloudComponent::BaseShadowRaySampleCount));
			}
		}
	};
}

#endif
