// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/VestigeGameplayTags.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "CQTest.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/GameModeBase.h"
#include "Sound/AudioSettings.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundWave.h"
#include "UI/VestigeFeedbackResponses.h"
#include "UI/VestigeHUD.h"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace VestigeAudioTests
{
	const TCHAR* const GameModePath = TEXT("/Game/Vestige/Blueprints/BP_GameMode.BP_GameMode_C");
	const TCHAR* const SplashPath = TEXT("/Game/Vestige/Audio/Water/S_Water_FillCan.S_Water_FillCan");
	const TCHAR* const OverallClassPath = TEXT("/Game/Vestige/Audio/Mix/SCL_Overall.SCL_Overall");

	// Bounds of a one-shot splash, in seconds, and the mono channel count.
	constexpr float MinSplashSeconds = 0.3f;
	constexpr float MaxSplashSeconds = 1.5f;
	constexpr int32 MonoChannels = 1;

	TEST_CLASS(Audio, "Vestige")
	{
		TEST_METHOD(TheFilledCanSplashes)
		{
			const UClass* GameModeClass = LoadClass<AGameModeBase>(nullptr, GameModePath);
			ASSERT_THAT(IsNotNull(GameModeClass));
			const UClass* HudActorClass = GameModeClass->GetDefaultObject<AGameModeBase>()->HUDClass;
			ASSERT_THAT(IsTrue(HudActorClass && HudActorClass->IsChildOf<AVestigeHUD>()));
			const UVestigeFeedbackResponses* Responses = HudActorClass->GetDefaultObject<AVestigeHUD>()->GetFeedbackResponses();
			ASSERT_THAT(IsNotNull(Responses));
			const USoundBase* Splash = LoadObject<USoundBase>(nullptr, SplashPath);
			ASSERT_THAT(IsNotNull(Splash));

			const FVestigeFeedbackResponse* Filled = Responses->FindResponse(VestigeGameplayTags::Feedback_Water_CanFilled);
			const FVestigeFeedbackResponse* RanDry = Responses->FindResponse(VestigeGameplayTags::Feedback_Water_WellRanDry);
			ASSERT_THAT(IsNotNull(Filled));
			ASSERT_THAT(IsNotNull(RanDry));
			ASSERT_THAT(IsTrue(Filled->Sound == Splash));
			ASSERT_THAT(IsTrue(RanDry->Sound == Splash));

			const TArray<FGameplayTag> Silent = {VestigeGameplayTags::Feedback_Water_CanEmpty,
				VestigeGameplayTags::Feedback_Water_CanFull, VestigeGameplayTags::Feedback_Water_WellDry};
			for (const FGameplayTag& Tag : Silent)
			{
				const FVestigeFeedbackResponse* Response = Responses->FindResponse(Tag);
				ASSERT_THAT(IsNotNull(Response));
				ASSERT_THAT(IsNull(Response->Sound.Get()));
			}
		}

		TEST_METHOD(TheSplashIsAShortRetainedOneShot)
		{
			const USoundWave* Splash = LoadObject<USoundWave>(nullptr, SplashPath);
			ASSERT_THAT(IsNotNull(Splash));

			ASSERT_THAT(AreEqual(Splash->NumChannels, MonoChannels));
			ASSERT_THAT(IsFalse(Splash->bLooping));
			ASSERT_THAT(IsTrue(Splash->Duration >= MinSplashSeconds && Splash->Duration <= MaxSplashSeconds));
			ASSERT_THAT(IsNull(Splash->AttenuationSettings.Get()));
			// The property, not GetLoadingBehavior(true), which asserts unless the inherited behavior was cached first.
			ASSERT_THAT(IsTrue(Splash->LoadingBehavior == ESoundWaveLoadingBehavior::RetainOnLoad));
		}

		TEST_METHOD(EverySoundHasAVolumeCategory)
		{
			const USoundClass* Overall = LoadObject<USoundClass>(nullptr, OverallClassPath);
			ASSERT_THAT(IsNotNull(Overall));

			FAssetRegistryModule& Module = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
			FARFilter Filter;
			Filter.ClassPaths.Add(USoundBase::StaticClass()->GetClassPathName());
			Filter.bRecursiveClasses = true;
			Filter.PackagePaths.Add(TEXT("/Game/Vestige"));
			Filter.bRecursivePaths = true;
			TArray<FAssetData> Sounds;
			Module.Get().GetAssets(Filter, Sounds);
			ASSERT_THAT(IsTrue(!Sounds.IsEmpty()));

			for (const FAssetData& Asset : Sounds)
			{
				const USoundBase* Sound = Cast<USoundBase>(Asset.GetAsset());
				ASSERT_THAT(IsNotNull(Sound));
				// The property itself: GetSoundClass() returns the default class for none.
				const USoundClass* Category = Sound->SoundClassObject;
				ASSERT_THAT(IsNotNull(Category, *FString::Printf(TEXT("%s has no sound class"), *Asset.AssetName.ToString())));

				bool bBelowOverall = false;
				for (const USoundClass* Parent = Category->ParentClass; Parent; Parent = Parent->ParentClass)
				{
					bBelowOverall |= Parent == Overall;
				}
				ASSERT_THAT(IsTrue(bBelowOverall, *FString::Printf(TEXT("%s is not under SCL_Overall"), *Asset.AssetName.ToString())));
			}
		}

		TEST_METHOD(TheDefaultSoundClassIsOverall)
		{
			const UAudioSettings* Settings = GetDefault<UAudioSettings>();
			ASSERT_THAT(IsNotNull(Settings));
			ASSERT_THAT(AreEqual(Settings->DefaultSoundClassName.ToString(), FString(OverallClassPath)));
		}
	};
}

#endif
