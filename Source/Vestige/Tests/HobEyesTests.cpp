// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "CQTest.h"
#include "Materials/MaterialInterface.h"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace HobEyesTests
{
	/** Hob's eyes: the saved material has to give a visible, cyan glow with no one setting a parameter. */
	TEST_CLASS(HobEyes, "Vestige")
	{
		static constexpr const TCHAR* MaterialPath = TEXT("/Game/Vestige/Materials/M_HobEyes.M_HobEyes");
		/** Above this the centre of the eye reads white (the art review's point); below the cyan is lost in daylight. */
		static constexpr float MaxStrength = 4.0f;

		const UMaterialInterface* LoadEyes() const { return LoadObject<UMaterialInterface>(nullptr, MaterialPath); }

		TEST_METHOD(EyeStrengthIsSavedAndVisible)
		{
			const UMaterialInterface* Eyes = LoadEyes();
			ASSERT_THAT(IsNotNull(Eyes));
			float Strength = 0.0f;
			ASSERT_THAT(IsTrue(Eyes->GetScalarParameterDefaultValue(FHashedMaterialParameterInfo(TEXT("EyeStrength")), Strength)));
			ASSERT_THAT(IsTrue(Strength > 1.0f));
			ASSERT_THAT(IsTrue(Strength <= MaxStrength));
		}

		TEST_METHOD(EyeColorIsCyanGreenerThanBlue)
		{
			const UMaterialInterface* Eyes = LoadEyes();
			ASSERT_THAT(IsNotNull(Eyes));
			FLinearColor Color = FLinearColor::Black;
			ASSERT_THAT(IsTrue(Eyes->GetVectorParameterDefaultValue(FHashedMaterialParameterInfo(TEXT("EyeColor")), Color)));
			ASSERT_THAT(IsTrue(Color.G > 0.5f));
			ASSERT_THAT(IsTrue(Color.G > Color.B));
			ASSERT_THAT(IsTrue(Color.R < 0.1f));
		}

		TEST_METHOD(BlinkPeriodIsSaved)
		{
			const UMaterialInterface* Eyes = LoadEyes();
			ASSERT_THAT(IsNotNull(Eyes));
			float Period = 0.0f;
			ASSERT_THAT(IsTrue(Eyes->GetScalarParameterDefaultValue(FHashedMaterialParameterInfo(TEXT("BlinkPeriod")), Period)));
			ASSERT_THAT(IsTrue(Period > 0.0f));
		}
	};
} // namespace HobEyesTests

#endif // WITH_DEV_AUTOMATION_TESTS
