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

			// The drain is applied once per game minute, and the asset itself is valid (warnings allowed).
			const UGameplayEffect* Drain = Loaded->GetLateNightDrainEffect().GetDefaultObject();
			ASSERT_THAT(IsTrue(Drain->DurationPolicy == EGameplayEffectDurationType::Instant));
			FDataValidationContext DrainContext;
			Drain->IsDataValid(DrainContext);
			ASSERT_THAT(AreEqual(DrainContext.GetNumErrors(), 0u));
#endif
		}

#if WITH_EDITOR
		TEST_METHOD(ADrainThatIsntInstantIsInvalid)
		{
			const UVestigeGameData& Loaded = UVestigeGameData::Get();
			ASSERT_THAT(IsNotNull(Loaded.GetExhaustedEffect().Get()));

			// Applied every game minute, an infinite effect would leave one more active effect each minute.
			UVestigeGameData* InfiniteDrain = NewObject<UVestigeGameData>();
			InfiniteDrain->SetEffectsForTests(
				Loaded.GetStaminaCostEffect(), Loaded.GetStaminaRefillEffect(), Loaded.GetExhaustedEffect(), Loaded.GetExhaustedEffect());
			FDataValidationContext InfiniteContext;
			ASSERT_THAT(IsTrue(InfiniteDrain->IsDataValid(InfiniteContext) == EDataValidationResult::Invalid));
			ASSERT_THAT(AreEqual(InfiniteContext.GetNumErrors(), 1u));

			// A periodic drain with a duration would drain on real time, on top of the clock's minutes.
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
