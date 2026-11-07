// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "Content/VestigeAssetManager.h"
#include "Content/VestigeGameData.h"
#include "CQTest.h"
#include "GameplayEffect.h"
#include "Tests/VestigeTestTimedDrainEffect.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace VestigeGameDataTests
{
	TEST_CLASS(GameData, "Vestige")
	{
		TEST_METHOD(NamesEveryEffect)
		{
			// Loaded by the asset manager at startup, from GameDataPath in Config/DefaultGame.ini.
			const UVestigeGameData* Loaded = UVestigeAssetManager::GetGameData();
			ASSERT_THAT(IsNotNull(Loaded));
			ASSERT_THAT(AreEqual(Loaded->GetPathName(), FString(TEXT("/Game/Vestige/DA_GameData.DA_GameData"))));
			ASSERT_THAT(IsTrue(&UVestigeGameData::Get() == Loaded));

			ASSERT_THAT(IsNotNull(Loaded->GetStaminaCostEffect().Get()));
			ASSERT_THAT(IsNotNull(Loaded->GetStaminaRefillEffect().Get()));
			ASSERT_THAT(IsNotNull(Loaded->GetLateNightDrainEffect().Get()));
			ASSERT_THAT(IsNotNull(Loaded->GetExhaustedEffect().Get()));
#if WITH_EDITOR
			FDataValidationContext Context;
			ASSERT_THAT(IsTrue(Loaded->IsDataValid(Context) != EDataValidationResult::Invalid));
			ASSERT_THAT(AreEqual(Context.GetNumErrors(), 0u));
#endif
		}

#if WITH_EDITOR
		TEST_METHOD(ADrainThatIsntInfiniteAndPeriodicIsInvalid)
		{
			const UVestigeGameData& Loaded = UVestigeGameData::Get();
			ASSERT_THAT(IsNotNull(Loaded.GetStaminaCostEffect().Get()));

			// The instant cost effect has no period, so it can't drain over the night.
			UVestigeGameData* InstantDrain = NewObject<UVestigeGameData>();
			InstantDrain->SetEffectsForTests(
				Loaded.GetStaminaCostEffect(), Loaded.GetStaminaRefillEffect(), Loaded.GetStaminaCostEffect(), Loaded.GetExhaustedEffect());
			FDataValidationContext InstantContext;
			ASSERT_THAT(IsTrue(InstantDrain->IsDataValid(InstantContext) == EDataValidationResult::Invalid));
			ASSERT_THAT(AreEqual(InstantContext.GetNumErrors(), 1u));

			// A drain with a duration would expire before the rest component removes it at the end of the day.
			UVestigeGameData* TimedDrain = NewObject<UVestigeGameData>();
			TimedDrain->SetEffectsForTests(Loaded.GetStaminaCostEffect(), Loaded.GetStaminaRefillEffect(),
				UVestigeTestTimedDrainEffect::StaticClass(), Loaded.GetExhaustedEffect());
			FDataValidationContext TimedContext;
			ASSERT_THAT(IsTrue(TimedDrain->IsDataValid(TimedContext) == EDataValidationResult::Invalid));
			ASSERT_THAT(AreEqual(TimedContext.GetNumErrors(), 1u));

			// Without effects, each one is reported.
			const UVestigeGameData* Empty = NewObject<UVestigeGameData>();
			FDataValidationContext EmptyContext;
			ASSERT_THAT(IsTrue(Empty->IsDataValid(EmptyContext) == EDataValidationResult::Invalid));
			ASSERT_THAT(AreEqual(EmptyContext.GetNumErrors(), 4u));
		}
#endif
	};
}

#endif
