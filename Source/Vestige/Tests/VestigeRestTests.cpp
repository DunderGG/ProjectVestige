// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/VestigeGameplayTags.h"
#include "AbilitySystem/VestigeStaminaSet.h"
#include "AbilitySystemComponent.h"
#include "Components/ActorTestSpawner.h"
#include "Content/VestigeGameData.h"
#include "Core/VestigeFeedbackSubsystem.h"
#include "CQTest.h"
#include "Farming/VestigeRestComponent.h"
#include "Farming/VestigeRestSaveData.h"
#include "Game/VestigeGameMode.h"
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

		/** How far a drained amount may be off: an hour's drain is 60 small float steps. */
		static constexpr float DrainTolerance = 0.01f;

		BEFORE_EACH()
		{
			// The stamina effects are GE_ assets named by the game data, which the asset manager loaded at startup.
			const UVestigeGameData& GameData = UVestigeGameData::Get();
			ASSERT_THAT(IsNotNull(GameData.GetStaminaCostEffect().Get()));
			ASSERT_THAT(IsNotNull(GameData.GetStaminaRefillEffect().Get()));
			ASSERT_THAT(IsNotNull(GameData.GetLateNightDrainEffect().Get()));
			ASSERT_THAT(IsNotNull(GameData.GetExhaustedEffect().Get()));
			Spawner.InitializeGameSubsystems();
			Player.Clock = UVestigeClockSubsystem::Get(&Spawner.GetWorld());
			ASSERT_THAT(IsNotNull(Player.Clock));
			// The game mode starts the next day once the day has ended.
			Spawner.SpawnActor<AVestigeGameMode>().DispatchBeginPlay();
			UClass* PlayerStateClass =
				LoadClass<AVestigePlayerState>(nullptr, TEXT("/Game/Vestige/Blueprints/BP_PlayerState.BP_PlayerState_C"));
			UClass* CharacterClass =
				LoadClass<AVestigePlayerCharacter>(nullptr, TEXT("/Game/Vestige/Blueprints/BP_PlayerCharacter.BP_PlayerCharacter_C"));
			ASSERT_THAT(IsNotNull(PlayerStateClass));
			ASSERT_THAT(IsNotNull(CharacterClass));
			AVestigePlayerState& PlayerState = Spawner.SpawnActor<AVestigePlayerState>(FActorSpawnParameters(), PlayerStateClass);
			AVestigePlayerCharacter& Character = Spawner.SpawnActor<AVestigePlayerCharacter>(FActorSpawnParameters(), CharacterClass);
			Character.SetPlayerState(&PlayerState);
			PlayerState.DispatchBeginPlay();
			Character.DispatchBeginPlay();
			Player.AbilitySystem = PlayerState.GetAbilitySystemComponent();
			Player.RestComponent = PlayerState.FindComponentByClass<UVestigeRestComponent>();
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
				AbilitySystem.MakeOutgoingSpec(UVestigeGameData::Get().GetStaminaCostEffect(), 1.f, AbilitySystem.MakeEffectContext());
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
			ASSERT_THAT(IsTrue(GetStamina(Setup) == GetMaxStamina(Setup)));

			PassHours(*Setup.Clock, 1.f);
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(GetStamina(Setup), GetMaxStamina(Setup) - 5.f, DrainTolerance)));
		}

		TEST_METHOD(APlayerArrivingAfterMidnightDrainsStamina)
		{
			// A map opened after midnight: the new map's player state never hears the midnight hour.
			SetClock(*Player.Clock, 25.f);
			UClass* PlayerStateClass =
				LoadClass<AVestigePlayerState>(nullptr, TEXT("/Game/Vestige/Blueprints/BP_PlayerState.BP_PlayerState_C"));
			AVestigePlayerState& NewPlayer = Spawner.SpawnActor<AVestigePlayerState>(FActorSpawnParameters(), PlayerStateClass);
			NewPlayer.DispatchBeginPlay();
			const UAbilitySystemComponent* NewAbilitySystem = NewPlayer.GetAbilitySystemComponent();
			ASSERT_THAT(IsNotNull(NewAbilitySystem));

			PassHours(*Player.Clock, 0.5f);

			const float NewStamina = NewAbilitySystem->GetNumericAttribute(UVestigeStaminaSet::GetStaminaAttribute());
			const float NewMaxStamina = NewAbilitySystem->GetNumericAttribute(UVestigeStaminaSet::GetMaxStaminaAttribute());
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(NewStamina, NewMaxStamina - 5.f, DrainTolerance)));
		}

		TEST_METHOD(ANightWithoutABodyStillRefillsStaminaAndSetsExhaustion)
		{
			const FPlayerSetup& Setup = Player;
			SpendAllStamina(Setup);
			APlayerState* PlayerState = Cast<APlayerState>(Setup.RestComponent->GetOwner());
			ASSERT_THAT(IsNotNull(PlayerState));
			APawn* Body = PlayerState->GetPawn();
			ASSERT_THAT(IsNotNull(Body));
			Body->Destroy();

			SetClock(*Setup.Clock, Setup.Clock->GetDayEndHour());
			PassHours(*Setup.Clock, 0.1f);

			ASSERT_THAT(IsTrue(Setup.Clock->GetCurrentDay() == 2));
			ASSERT_THAT(IsTrue(Setup.RestComponent->IsExhausted()));
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(GetStamina(Setup), 0.75f * GetMaxStamina(Setup))));
		}

		TEST_METHOD(TheRestComponentIsFoundThroughThePlayersBody)
		{
			const APlayerState* PlayerState = Cast<APlayerState>(Player.RestComponent->GetOwner());
			ASSERT_THAT(IsNotNull(PlayerState));

			ASSERT_THAT(IsTrue(UVestigeRestComponent::Find(PlayerState->GetPawn()) == Player.RestComponent));
			ASSERT_THAT(IsTrue(UVestigeRestComponent::Find(PlayerState) == Player.RestComponent));
			ASSERT_THAT(IsNull(UVestigeRestComponent::Find(nullptr)));
		}

		TEST_METHOD(TheLateNightDrainsItsRatePerGameHour)
		{
			const FPlayerSetup& Setup = Player;
			SetClock(*Setup.Clock, 23.5f);

			// Nothing drains at the midnight instant, only for the game minutes after it.
			PassHours(*Setup.Clock, 0.5f);
			ASSERT_THAT(IsTrue(GetStamina(Setup) == GetMaxStamina(Setup)));

			// The rest component's default of 10 stamina per game hour.
			PassHours(*Setup.Clock, 1.f);
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(GetStamina(Setup), GetMaxStamina(Setup) - 10.f, DrainTolerance)));
		}

		TEST_METHOD(ASkipPastMidnightDrainsAsIfTheTimePassed)
		{
			const FPlayerSetup& Setup = Player;
			SetClock(*Setup.Clock, 23.f);

			ASSERT_THAT(IsTrue(Setup.Clock->AdvanceTo(26.f)));

			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(GetStamina(Setup), GetMaxStamina(Setup) - 20.f, DrainTolerance)));
		}

		TEST_METHOD(TheLateNightLeavesNoActiveEffect)
		{
			// The drain follows the clock's minutes; an active effect would drain on a real-time period of its own.
			const FPlayerSetup& Setup = Player;
			SetClock(*Setup.Clock, 23.5f);

			PassHours(*Setup.Clock, 1.5f);

			FGameplayEffectQuery DrainQuery;
			DrainQuery.EffectDefinition = UVestigeGameData::Get().GetLateNightDrainEffect();
			ASSERT_THAT(IsTrue(Setup.AbilitySystem->GetActiveEffects(DrainQuery).IsEmpty()));
		}

		TEST_METHOD(TheDrainStopsWhenTheNextDayStarts)
		{
			const FPlayerSetup& Setup = Player;
			SetClock(*Setup.Clock, 23.5f);
			PassHours(*Setup.Clock, 1.f);
			Setup.RestComponent->Sleep();
			const float MorningStamina = GetStamina(Setup);

			PassHours(*Setup.Clock, 1.f);

			ASSERT_THAT(IsTrue(Setup.Clock->GetCurrentDay() == 2));
			ASSERT_THAT(IsTrue(GetStamina(Setup) == MorningStamina));
		}

		TEST_METHOD(AMinuteThatEndsTheDayDrainsNothingAfterTheRefill)
		{
			// A rule that ends the day at a minute: the new morning's stamina is the refill, whichever listener hears the minute first.
			const FPlayerSetup& Setup = Player;
			SpendAllStamina(Setup);
			SetClock(*Setup.Clock, 24.5f);
			const int32 EndMinute = VestigeClock::ToWholeMinutes(24.5f) + 1;
			Setup.Clock->OnMinuteStarted.AddLambda(
				[&Setup, EndMinute](int32 Minute)
				{
					if (Minute == EndMinute)
					{
						Setup.Clock->EndDay();
					}
				});

			PassHours(*Setup.Clock, 1.f);

			ASSERT_THAT(IsTrue(Setup.Clock->GetCurrentDay() == 2));
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(GetStamina(Setup), 0.75f * GetMaxStamina(Setup))));
		}

		TEST_METHOD(ThePlayerIsWarnedAtTenAndAtMidnight)
		{
			const FPlayerSetup& Setup = Player;
			UVestigeFeedbackSubsystem* Feedback = Spawner.GetWorld().GetSubsystem<UVestigeFeedbackSubsystem>();
			ASSERT_THAT(IsNotNull(Feedback));
			TArray<FGameplayTag> Reports;
			Feedback->OnFeedback.AddLambda(
				[&Reports](const FVestigeFeedbackReport& Report)
				{
					Reports.Add(Report.Tag);
				});
			SetClock(*Setup.Clock, 21.5f);

			PassHours(*Setup.Clock, 1.f);
			ASSERT_THAT(IsTrue(Reports == TArray<FGameplayTag>{VestigeGameplayTags::Feedback_Rest_GettingLate}));

			PassHours(*Setup.Clock, 2.f);
			ASSERT_THAT(IsTrue(Reports
				== TArray<FGameplayTag>{VestigeGameplayTags::Feedback_Rest_GettingLate, VestigeGameplayTags::Feedback_Rest_PastMidnight}));
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

		TEST_METHOD(TheNewDaysSaveHoldsTheNightsStaminaAndExhaustion)
		{
			const FPlayerSetup& Setup = Player;
			SpendAllStamina(Setup);
			bool bSavedExhausted = false;
			float SavedStamina = 0.f;
			Setup.Clock->OnNewDayFinished.AddLambda(
				[&Setup, &bSavedExhausted, &SavedStamina](int32 Day)
				{
					bSavedExhausted = Setup.RestComponent->GetSaveData().bExhausted;
					SavedStamina = GetStamina(Setup);
				});

			SleepAt(Setup, 25.f);

			ASSERT_THAT(IsTrue(bSavedExhausted));
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(SavedStamina, 0.75f * GetMaxStamina(Setup))));
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
