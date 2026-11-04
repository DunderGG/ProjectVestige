// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/VestigeGameplayTags.h"
#include "AbilitySystem/VestigeStaminaCostEffect.h"
#include "AbilitySystem/VestigeStaminaDrainEffect.h"
#include "AbilitySystem/VestigeStaminaSet.h"
#include "AbilitySystemComponent.h"
#include "Components/ActorTestSpawner.h"
#include "Core/VestigeFeedbackSubsystem.h"
#include "CQTest.h"
#include "Farming/VestigeRestComponent.h"
#include "Farming/VestigeRestSaveData.h"
#include "Player/VestigePlayerCharacter.h"
#include "Player/VestigePlayerState.h"
#include "Time/VestigeClockSaveData.h"
#include "Time/VestigeClockSubsystem.h"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace VestigeRestTests
{
	TEST_CLASS(Rest, "Vestige")
	{
		FActorTestSpawner Spawner;

		/** The game's clock and farmer with its player state, so the rest component has stamina to work on. */
		struct FPlayerSetup
		{
			UVestigeClockSubsystem* Clock = nullptr;
			UAbilitySystemComponent* AbilitySystem = nullptr;
			UVestigeRestComponent* RestComponent = nullptr;
		};

		FPlayerSetup Player;

		BEFORE_EACH()
		{
			Spawner.InitializeGameSubsystems();
			Player.Clock = UVestigeClockSubsystem::Get(&Spawner.GetWorld());
			ASSERT_THAT(IsNotNull(Player.Clock));
			UClass* PlayerStateClass =
				LoadClass<AVestigePlayerState>(nullptr, TEXT("/Game/Vestige/Blueprints/BP_PlayerState.BP_PlayerState_C"));
			UClass* CharacterClass =
				LoadClass<AVestigePlayerCharacter>(nullptr, TEXT("/Game/Vestige/Blueprints/BP_PlayerCharacter.BP_PlayerCharacter_C"));
			ASSERT_THAT(IsNotNull(PlayerStateClass));
			ASSERT_THAT(IsNotNull(CharacterClass));
			AVestigePlayerState& PlayerState = Spawner.SpawnActor<AVestigePlayerState>(FActorSpawnParameters(), PlayerStateClass);
			AVestigePlayerCharacter& Character = Spawner.SpawnActor<AVestigePlayerCharacter>(FActorSpawnParameters(), CharacterClass);
			Character.SetPlayerState(&PlayerState);
			Character.DispatchBeginPlay();
			Player.AbilitySystem = PlayerState.GetAbilitySystemComponent();
			Player.RestComponent = Character.FindComponentByClass<UVestigeRestComponent>();
			ASSERT_THAT(IsNotNull(Player.AbilitySystem));
			ASSERT_THAT(IsNotNull(Player.RestComponent));
		}

		static float GetStamina(const FPlayerSetup& Setup)
		{
			return Setup.AbilitySystem->GetNumericAttribute(UVestigeStaminaSet::GetStaminaAttribute());
		}

		static float GetMaxStamina(const FPlayerSetup& Setup)
		{
			return Setup.AbilitySystem->GetNumericAttribute(UVestigeStaminaSet::GetMaxStaminaAttribute());
		}

		/** Spends all stamina through the cost effect, as tool uses do. */
		static void SpendAllStamina(const FPlayerSetup& Setup)
		{
			UAbilitySystemComponent& AbilitySystem = *Setup.AbilitySystem;
			const FGameplayEffectSpecHandle Cost =
				AbilitySystem.MakeOutgoingSpec(UVestigeStaminaCostEffect::StaticClass(), 1.f, AbilitySystem.MakeEffectContext());
			Cost.Data->SetSetByCallerMagnitude(VestigeGameplayTags::Data_StaminaCost, -GetMaxStamina(Setup));
			AbilitySystem.ApplyGameplayEffectSpecToSelf(*Cost.Data);
		}

		/** Sets the clock to Hour on the current day, the way loading a save does, without broadcasting anything. */
		static void SetClock(UVestigeClockSubsystem& GameClock, float Hour)
		{
			FVestigeClockSaveData Data;
			Data.Day = GameClock.GetCurrentDay();
			Data.TimeOfDay = Hour;
			GameClock.RestoreFromSaveData(Data);
		}

		/** Runs the clock forward by GameHours, the way a frame's tick does. */
		static void PassHours(UVestigeClockSubsystem& GameClock, float GameHours)
		{
			GameClock.Tick(GameHours * GameClock.GetRealSecondsPerGameHour());
		}

		/** Sleeps at Hour on the current day. */
		static void SleepAt(const FPlayerSetup& Setup, float Hour)
		{
			SetClock(*Setup.Clock, Hour);
			Setup.RestComponent->Sleep();
		}

		TEST_METHOD(SleepingStartsTheNextDayEvenWithoutStamina)
		{
			AActor& Owner = Spawner.SpawnActor<AActor>();
			UVestigeRestComponent* RestComponent = NewObject<UVestigeRestComponent>(&Owner);
			RestComponent->RegisterComponent();
			Owner.DispatchBeginPlay();

			RestComponent->Sleep();

			ASSERT_THAT(IsTrue(Player.Clock->GetCurrentDay() == 2));
		}

		TEST_METHOD(SleepingBeforeMidnightRefillsStaminaFully)
		{
			const FPlayerSetup& Setup = Player;
			SpendAllStamina(Setup);

			SleepAt(Setup, 23.f);

			ASSERT_THAT(IsTrue(GetStamina(Setup) == GetMaxStamina(Setup)));
			ASSERT_THAT(IsFalse(Setup.RestComponent->IsExhausted()));
		}

		TEST_METHOD(SleepingAfterMidnightWakesExhaustedWithLessStamina)
		{
			const FPlayerSetup& Setup = Player;
			SpendAllStamina(Setup);

			SleepAt(Setup, 24.5f);

			ASSERT_THAT(IsTrue(Setup.RestComponent->IsExhausted()));
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(GetStamina(Setup), 0.75f * GetMaxStamina(Setup))));
		}

		TEST_METHOD(StayingUpUntilTheDayEndStartsTheNextDayExhausted)
		{
			const FPlayerSetup& Setup = Player;

			SetClock(*Setup.Clock, Setup.Clock->GetDayEndHour());
			PassHours(*Setup.Clock, 0.1f);

			ASSERT_THAT(IsTrue(Setup.Clock->GetCurrentDay() == 2));
			ASSERT_THAT(IsTrue(Setup.Clock->GetTimeOfDay() == Setup.Clock->GetDayStartHour()));
			ASSERT_THAT(IsTrue(Setup.RestComponent->IsExhausted()));
		}

		TEST_METHOD(TheDayEndsAtSunriseRatherThanAtTwoInTheMorning)
		{
			ASSERT_THAT(IsTrue(Player.Clock->GetDayEndHour() == Player.Clock->GetDayStartHour() + 24.f));
		}

		TEST_METHOD(ANightInBedBeforeMidnightClearsExhaustion)
		{
			const FPlayerSetup& Setup = Player;
			SleepAt(Setup, 25.f);
			SpendAllStamina(Setup);

			SleepAt(Setup, 22.f);

			ASSERT_THAT(IsFalse(Setup.RestComponent->IsExhausted()));
			ASSERT_THAT(IsTrue(GetStamina(Setup) == GetMaxStamina(Setup)));
		}

		TEST_METHOD(ExhaustionHasOneLevelOnly)
		{
			const FPlayerSetup& Setup = Player;
			SleepAt(Setup, 25.f);
			SleepAt(Setup, 26.f);
			SleepAt(Setup, 27.f);

			SleepAt(Setup, 21.f);

			ASSERT_THAT(IsFalse(Setup.RestComponent->IsExhausted()));
		}

		TEST_METHOD(StaminaStartsDrainingAtMidnight)
		{
			const FPlayerSetup& Setup = Player;
			SetClock(*Setup.Clock, 21.5f);

			PassHours(*Setup.Clock, 2.f);
			ASSERT_THAT(IsFalse(Setup.RestComponent->IsDrainingStamina()));

			PassHours(*Setup.Clock, 1.f);
			ASSERT_THAT(IsTrue(Setup.RestComponent->IsDrainingStamina()));
		}

		TEST_METHOD(TheDrainTakesItsRatePerGameHourWhateverTheClockSpeed)
		{
			const FPlayerSetup& Setup = Player;
			SetClock(*Setup.Clock, 23.5f);
			PassHours(*Setup.Clock, 1.f);

			FGameplayEffectQuery DrainQuery;
			DrainQuery.EffectDefinition = UVestigeStaminaDrainEffect::StaticClass();
			const TArray<FActiveGameplayEffectHandle> Drains = Setup.AbilitySystem->GetActiveEffects(DrainQuery);
			ASSERT_THAT(IsTrue(Drains.Num() == 1));
			const FActiveGameplayEffect* Drain = Setup.AbilitySystem->GetActiveGameplayEffect(Drains[0]);
			ASSERT_THAT(IsNotNull(Drain));

			// The rest component's default of 10 stamina per game hour, spread over the periods in one game hour.
			const float PeriodsPerGameHour = Setup.Clock->GetRealSecondsPerGameHour() / UVestigeStaminaDrainEffect::PeriodSeconds;
			const float PerPeriod = Drain->Spec.GetSetByCallerMagnitude(VestigeGameplayTags::Data_StaminaDrain, false);
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(PerPeriod * PeriodsPerGameHour, -10.f)));
		}

		TEST_METHOD(TheDrainStopsWhenTheNextDayStarts)
		{
			const FPlayerSetup& Setup = Player;
			SetClock(*Setup.Clock, 23.5f);
			PassHours(*Setup.Clock, 1.f);

			Setup.RestComponent->Sleep();

			FGameplayEffectQuery DrainQuery;
			DrainQuery.EffectDefinition = UVestigeStaminaDrainEffect::StaticClass();
			ASSERT_THAT(IsFalse(Setup.RestComponent->IsDrainingStamina()));
			ASSERT_THAT(IsTrue(Setup.AbilitySystem->GetActiveEffects(DrainQuery).IsEmpty()));
		}

		TEST_METHOD(ThePlayerIsWarnedAtTenAndAtMidnight)
		{
			const FPlayerSetup& Setup = Player;
			UVestigeFeedbackSubsystem* Feedback = Spawner.GetWorld().GetSubsystem<UVestigeFeedbackSubsystem>();
			ASSERT_THAT(IsNotNull(Feedback));
			TArray<FString> Messages;
			Feedback->OnFeedback.AddLambda(
				[&Messages](const FText& Message)
				{
					Messages.Add(Message.ToString());
				});
			SetClock(*Setup.Clock, 21.5f);

			PassHours(*Setup.Clock, 1.f);
			ASSERT_THAT(IsTrue(Messages.Num() == 1));
			ASSERT_THAT(IsTrue(Messages[0].Contains(TEXT("getting late"))));

			PassHours(*Setup.Clock, 2.f);
			ASSERT_THAT(IsTrue(Messages.Num() == 2));
			ASSERT_THAT(IsTrue(Messages[1].Contains(TEXT("midnight"))));
		}

		TEST_METHOD(ExhaustionRoundTripsThroughTheSave)
		{
			const FPlayerSetup& Setup = Player;
			SleepAt(Setup, 25.f);
			const FVestigeRestSaveData Data = Setup.RestComponent->GetSaveData();
			SleepAt(Setup, 21.f);
			ASSERT_THAT(IsFalse(Setup.RestComponent->IsExhausted()));

			Setup.RestComponent->RestoreFromSaveData(Data);

			ASSERT_THAT(IsTrue(Data.bExhausted));
			ASSERT_THAT(IsTrue(Setup.RestComponent->IsExhausted()));
		}

		TEST_METHOD(RestoringExhaustionChangesNoStamina)
		{
			const FPlayerSetup& Setup = Player;
			SpendAllStamina(Setup);
			FVestigeRestSaveData Data;
			Data.bExhausted = true;

			Setup.RestComponent->RestoreFromSaveData(Data);

			ASSERT_THAT(IsTrue(Setup.RestComponent->IsExhausted()));
			ASSERT_THAT(IsTrue(GetStamina(Setup) == 0.f));
		}
	};
}

#endif
