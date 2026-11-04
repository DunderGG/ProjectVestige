// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/ActorTestSpawner.h"
#include "CQTest.h"
#include "Farming/VestigeRestComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Player/VestigeCheatManager.h"
#include "Time/VestigeDayCycle.h"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace VestigeCheatManagerTests
{
	TEST_CLASS(Cheats, "Vestige")
	{
		FActorTestSpawner Spawner;

		TEST_METHOD(TheGamesPlayerControllerUsesTheVestigeCheatManager)
		{
			const UClass* ControllerClass = LoadClass<APlayerController>(
				nullptr, TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonPlayerController.BP_ThirdPersonPlayerController_C"));
			ASSERT_THAT(IsNotNull(ControllerClass));

			const APlayerController* Controller = GetDefault<APlayerController>(ControllerClass);

			ASSERT_THAT(IsTrue(Controller->CheatClass == UVestigeCheatManager::StaticClass()));
		}

		TEST_METHOD(SkipDaySleepsWhereThePlayerStands)
		{
			const AVestigeDayCycle& Cycle = Spawner.SpawnActor<AVestigeDayCycle>();
			APawn& Pawn = Spawner.SpawnActor<APawn>();
			UVestigeRestComponent* Rest = NewObject<UVestigeRestComponent>(&Pawn);
			Rest->RegisterComponent();
			Pawn.DispatchBeginPlay();
			APlayerController& Controller = Spawner.SpawnActor<APlayerController>();
			Controller.Possess(&Pawn);
			UVestigeCheatManager* Cheats = NewObject<UVestigeCheatManager>(&Controller);

			Cheats->SkipDay();

			ASSERT_THAT(IsTrue(Cycle.GetCurrentDay() == 2));
		}
	};
}

#endif
