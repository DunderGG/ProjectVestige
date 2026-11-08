// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/VestigeGameplayTags.h"
#include "Components/ActorTestSpawner.h"
#include "Core/VestigeFeedbackSubsystem.h"
#include "CQTest.h"
#include "Game/VestigeGameMode.h"
#include "Game/VestigePlayerController.h"
#include "GameMapsSettings.h"
#include "Player/VestigePlayerCharacter.h"
#include "Player/VestigePlayerState.h"
#include "Time/VestigeClockSaveData.h"
#include "Time/VestigeClockSubsystem.h"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace VestigeGameModeTests
{
	TEST_CLASS(GameMode, "Vestige")
	{
		FActorTestSpawner Spawner;

		/** The test game instance's clock. */
		UVestigeClockSubsystem* Clock = nullptr;

		/** The game mode, playing, without any player or body. */
		AVestigeGameMode* PlayingGameMode = nullptr;

		BEFORE_EACH()
		{
			Spawner.InitializeGameSubsystems();
			Clock = UVestigeClockSubsystem::Get(&Spawner.GetWorld());
			ASSERT_THAT(IsNotNull(Clock));
			PlayingGameMode = &Spawner.SpawnActor<AVestigeGameMode>();
			PlayingGameMode->DispatchBeginPlay();
		}

		/** Sets the clock to Hour on day 1, the way loading a save does, without broadcasting anything. */
		void SetClock(float Hour)
		{
			FVestigeClockSaveData Data;
			Data.Day = 1;
			Data.TimeOfDay = Hour;
			Clock->RestoreFromSaveData(Data);
		}

		/** Runs the clock forward by GameHours, the way a frame's tick does. */
		void PassHours(float GameHours) { Clock->Tick(GameHours * Clock->GetRealSecondsPerGameHour()); }

		/** Every feedback report from now on. */
		void RecordReports(TArray<FVestigeFeedbackReport>& OutReports)
		{
			UVestigeFeedbackSubsystem* Feedback = Spawner.GetWorld().GetSubsystem<UVestigeFeedbackSubsystem>();
			if (Feedback)
			{
				Feedback->OnFeedback.AddLambda(
					[&OutReports](const FVestigeFeedbackReport& Report)
					{
						OutReports.Add(Report);
					});
			}
		}

		/** The Day value of Report, or INDEX_NONE if it has none. */
		static int64 GetDay(const FVestigeFeedbackReport& Report)
		{
			const FFormatArgumentValue* Day = Report.Arguments.Find(TEXT("Day"));
			return Day && Day->GetType() == EFormatArgumentType::Int ? Day->GetIntValue() : INDEX_NONE;
		}

		TEST_METHOD(TheNextDayStartsAtTheDayEndWithoutAPlayer)
		{
			SetClock(Clock->GetDayEndHour() - 0.5f);

			PassHours(1.f);

			ASSERT_THAT(IsTrue(Clock->GetCurrentDay() == 2));
			ASSERT_THAT(IsTrue(Clock->GetTimeOfDay() == Clock->GetDayStartHour()));
			ASSERT_THAT(IsTrue(Clock->IsRunning()));
		}

		TEST_METHOD(EndingTheDayEarlyStartsExactlyOneNewDay)
		{
			SetClock(21.f);

			ASSERT_THAT(IsTrue(Clock->EndDay()));

			ASSERT_THAT(IsTrue(Clock->GetCurrentDay() == 2));
			ASSERT_THAT(IsTrue(Clock->IsRunning()));
		}

		TEST_METHOD(AClockRestoredAtTheDayEndStartsTheNextDayOnItsFirstTick)
		{
			SetClock(Clock->GetDayEndHour());

			PassHours(0.1f);

			ASSERT_THAT(IsTrue(Clock->GetCurrentDay() == 2));
		}

		TEST_METHOD(TheReportSaysWhetherThePlayerSleptOrStayedUp)
		{
			TArray<FVestigeFeedbackReport> Reports;
			RecordReports(Reports);

			SetClock(22.f);
			Clock->EndDay();
			PassHours(Clock->GetDayEndHour());

			ASSERT_THAT(IsTrue(Reports.Num() == 2));
			ASSERT_THAT(IsTrue(Reports[0].Tag == VestigeGameplayTags::Feedback_Day_Slept));
			ASSERT_THAT(IsTrue(GetDay(Reports[0]) == 2));
			ASSERT_THAT(IsTrue(Reports[1].Tag == VestigeGameplayTags::Feedback_Day_StayedUp));
			ASSERT_THAT(IsTrue(GetDay(Reports[1]) == 3));
		}

		TEST_METHOD(AGameModeThatHasStoppedPlayingNoLongerStartsDays)
		{
			PlayingGameMode->Destroy();
			SetClock(22.f);
			TestRunner->AddExpectedMessagePlain(TEXT("nothing starts the next one"), ELogVerbosity::Warning);

			Clock->EndDay();

			ASSERT_THAT(IsTrue(Clock->GetCurrentDay() == 1));
			ASSERT_THAT(IsFalse(Clock->IsRunning()));
		}

		TEST_METHOD(TheGamesDefaultGameModeIsOurs)
		{
			const FString GameModePath = UGameMapsSettings::GetGlobalDefaultGameMode();
			const UClass* GameModeClass = LoadClass<AGameModeBase>(nullptr, *GameModePath);
			ASSERT_THAT(IsNotNull(GameModeClass));
			ASSERT_THAT(IsTrue(GameModeClass->IsChildOf<AVestigeGameMode>()));

			const AGameModeBase* Defaults = GetDefault<AGameModeBase>(GameModeClass);
			ASSERT_THAT(IsTrue(Defaults->PlayerControllerClass->IsChildOf<AVestigePlayerController>()));
			ASSERT_THAT(IsTrue(Defaults->PlayerStateClass->IsChildOf<AVestigePlayerState>()));
			ASSERT_THAT(IsTrue(Defaults->DefaultPawnClass->IsChildOf<AVestigePlayerCharacter>()));
		}
	};
}

#endif
