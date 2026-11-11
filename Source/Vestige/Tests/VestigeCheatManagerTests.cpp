// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/ActorTestSpawner.h"
#include "Content/FarmCropData.h"
#include "CQTest.h"
#include "Farming/FarmField.h"
#include "Farming/FarmFieldMeshesComponent.h"
#include "Farming/FarmGrid.h"
#include "Farming/VestigeRestComponent.h"
#include "Game/VestigeGameMode.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Player/VestigeCheatManager.h"
#include "Time/VestigeClockSubsystem.h"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace VestigeCheatManagerTests
{
	TEST_CLASS(Cheats, "Vestige")
	{
		FActorTestSpawner Spawner;

		/** The game instance holds the clock and the farm fields the commands work on. */
		BEFORE_EACH() { Spawner.InitializeGameSubsystems(); }

		TEST_METHOD(TheGamesPlayerControllerUsesTheVestigeCheatManager)
		{
			const UClass* ControllerClass =
				LoadClass<APlayerController>(nullptr, TEXT("/Game/Vestige/Blueprints/BP_PlayerController.BP_PlayerController_C"));
			ASSERT_THAT(IsNotNull(ControllerClass));

			const APlayerController* Controller = GetDefault<APlayerController>(ControllerClass);

			ASSERT_THAT(IsTrue(Controller->CheatClass == UVestigeCheatManager::StaticClass()));
		}

		TEST_METHOD(SkipDaySleepsWhereThePlayerStands)
		{
			const UVestigeClockSubsystem* Clock = UVestigeClockSubsystem::Get(&Spawner.GetWorld());
			ASSERT_THAT(IsNotNull(Clock));
			Spawner.SpawnActor<AVestigeGameMode>().DispatchBeginPlay();
			APlayerState& PlayerState = Spawner.SpawnActor<APlayerState>();
			UVestigeRestComponent* Rest = NewObject<UVestigeRestComponent>(&PlayerState);
			Rest->RegisterComponent();
			PlayerState.DispatchBeginPlay();
			APlayerController& Controller = Spawner.SpawnActor<APlayerController>();
			Controller.PlayerState = &PlayerState;
			UVestigeCheatManager* Cheats = NewObject<UVestigeCheatManager>(&Controller);

			Cheats->SkipDay();

			ASSERT_THAT(IsTrue(Clock->GetCurrentDay() == 2));
		}

		/** A cheat manager for a player controller in the test world. */
		UVestigeCheatManager* SpawnCheats()
		{
			APlayerController& Controller = Spawner.SpawnActor<APlayerController>();
			return NewObject<UVestigeCheatManager>(&Controller);
		}

		TEST_METHOD(BenchmarkFieldPlantsARipeCropOnEveryTileOfTheNewSize)
		{
			AFarmGrid& Grid = Spawner.SpawnActor<AFarmGrid>();
			Grid.DispatchBeginPlay();

			SpawnCheats()->BenchmarkField(3, TEXT("Base.Turnip"));

			ASSERT_THAT(IsTrue(Grid.GetField()->GetSize() == FIntPoint(3, 3)));
			ASSERT_THAT(IsNull(Grid.GetTile(FIntPoint(3, 0))));
			for (int32 Y = 0; Y < 3; ++Y)
			{
				for (int32 X = 0; X < 3; ++X)
				{
					const FFarmTileState* Tile = Grid.GetTile(FIntPoint(X, Y));
					ASSERT_THAT(IsTrue(Tile && Tile->IsRipe() && Tile->GetCrop()->GetContentId() == TEXT("Base.Turnip")));
				}
			}
			// A soil and a crop instance per tile.
			ASSERT_THAT(IsTrue(Grid.GetMeshes()->GetTotalInstanceCount() == 18));
		}

		TEST_METHOD(BenchmarkFieldWithAnUnknownCropLeavesTheFarmAlone)
		{
			AFarmGrid& Grid = Spawner.SpawnActor<AFarmGrid>();
			Grid.DispatchBeginPlay();
			const FIntPoint Size = Grid.GetField()->GetSize();

			SpawnCheats()->BenchmarkField(3, TEXT("Base.NoSuchCrop"));

			ASSERT_THAT(IsTrue(Grid.GetField()->GetSize() == Size));
		}

		/** The test game instance's clock, which SkipToTime runs forward. */
		UVestigeClockSubsystem& GetClock()
		{
			UVestigeClockSubsystem* Clock = UVestigeClockSubsystem::Get(&Spawner.GetWorld());
			check(Clock);
			return *Clock;
		}

		/** The clock's whole minutes since midnight at the start of the day. */
		int32 GetMinute() { return VestigeClock::ToWholeMinutes(GetClock().GetTimeOfDay()); }

		TEST_METHOD(SkipToTimeTakesTheTimeAsDigits)
		{
			UVestigeCheatManager* Cheats = SpawnCheats();

			Cheats->SkipToTime(TEXT("945"));
			ASSERT_THAT(IsTrue(GetMinute() == 9 * 60 + 45));

			Cheats->SkipToTime(TEXT("1930"));
			ASSERT_THAT(IsTrue(GetMinute() == 19 * 60 + 30));

			Cheats->SkipToTime(TEXT("21"));
			ASSERT_THAT(IsTrue(GetMinute() == 21 * 60));
		}

		TEST_METHOD(SkipToTimeReachesTheTypedMinuteAfterMidnight)
		{
			// 00:10 is 24.1666... hours, which a float holds just short of the minute.
			int32 LastMinute = 0;
			GetClock().OnMinuteStarted.AddLambda(
				[&LastMinute](int32 Minute)
				{
					LastMinute = Minute;
				});

			SpawnCheats()->SkipToTime(TEXT("0010"));

			ASSERT_THAT(IsTrue(LastMinute == 24 * 60 + 10));
			ASSERT_THAT(IsTrue(GetMinute() == 24 * 60 + 10));
		}

		TEST_METHOD(SkipToTimeBeforeTheDayStartIsAfterMidnight)
		{
			const float DayStart = GetClock().GetDayStartHour();
			ASSERT_THAT(IsTrue(DayStart > 1.f));

			SpawnCheats()->SkipToTime(TEXT("0100"));

			ASSERT_THAT(IsTrue(GetMinute() == 25 * 60));
		}

		TEST_METHOD(SkipToTimeIgnoresEarlierAndUnreadableTimes)
		{
			UVestigeCheatManager* Cheats = SpawnCheats();
			Cheats->SkipToTime(TEXT("2000"));
			const float Skipped = GetClock().GetTimeOfDay();

			for (const TCHAR* Time : {TEXT("1900"), TEXT(""), TEXT("evening"), TEXT("2175"), TEXT("25"), TEXT("2401"), TEXT("21:30"),
					 TEXT("-2100"), TEXT("21000")})
			{
				Cheats->SkipToTime(Time);
				ASSERT_THAT(IsTrue(GetClock().GetTimeOfDay() == Skipped));
			}
		}
	};
}

#endif
