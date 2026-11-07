// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/ActorTestSpawner.h"
#include "Content/VestigeItemData.h"
#include "CQTest.h"
#include "Farming/FarmingComponent.h"
#include "InputCoreTypes.h"
#include "Inventory/VestigeInventoryComponent.h"
#include "Inventory/VestigeInventorySaveData.h"
#include "Player/VestigeCharacterMovementComponent.h"
#include "Player/VestigePlayerCharacter.h"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace VestigePlayerCharacterTests
{
	TEST_CLASS(PlayerCharacter, "Vestige")
	{
		FActorTestSpawner Spawner;

		TEST_METHOD(ActionKeyHintsNameTheBoundKeys)
		{
			const AVestigePlayerCharacter& Farmer = Spawner.SpawnActor<AVestigePlayerCharacter>();

			const TArray<FVestigeKeyHint> Hints = Farmer.GetActionKeyHints();

			// The test keys N and F9 are console commands now (UVestigeCheatManager), so they are neither bound nor hinted.
			ASSERT_THAT(IsTrue(Hints.Num() == 2));
			ASSERT_THAT(IsTrue(Hints[0].Key.ToString() == TEXT("E")));
			ASSERT_THAT(IsTrue(Hints[1].Key.ToString() == TEXT("F")));
			for (const FVestigeKeyHint& Hint : Hints)
			{
				ASSERT_THAT(IsFalse(Hint.Action.IsEmpty()));
			}
		}

		TEST_METHOD(MovementInputRampsInFromThePress)
		{
			constexpr float RampSeconds = 0.15f;

			ASSERT_THAT(IsTrue(AVestigePlayerCharacter::GetMoveInputScale(0.f, RampSeconds) == 0.f));
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(AVestigePlayerCharacter::GetMoveInputScale(RampSeconds * 0.5f, RampSeconds), 0.5f)));
			ASSERT_THAT(IsTrue(AVestigePlayerCharacter::GetMoveInputScale(RampSeconds, RampSeconds) == 1.f));
			ASSERT_THAT(IsTrue(AVestigePlayerCharacter::GetMoveInputScale(2.f, RampSeconds) == 1.f));
			ASSERT_THAT(IsTrue(AVestigePlayerCharacter::GetMoveInputScale(TNumericLimits<float>::Max(), RampSeconds) == 1.f));
		}

		TEST_METHOD(WithoutARampTheInputIsFullAtOnce) { ASSERT_THAT(IsTrue(AVestigePlayerCharacter::GetMoveInputScale(0.f, 0.f) == 1.f)); }

		TEST_METHOD(TheFarmerTurnsWithTheVestigeMovementComponent)
		{
			const AVestigePlayerCharacter& Farmer = Spawner.SpawnActor<AVestigePlayerCharacter>();

			// The template's constructor sets its own rate, so this catches the farmer's rate being overwritten.
			const UVestigeCharacterMovementComponent* Movement = Farmer.GetCharacterMovement<UVestigeCharacterMovementComponent>();
			ASSERT_THAT(IsNotNull(Movement));
			ASSERT_THAT(IsTrue(Movement->RotationRate.Yaw == VestigeMovement::TurnDegreesPerSecond));
		}

		TEST_METHOD(HoldingTheUseKeyHalvesTheWalkingSpeed)
		{
			AVestigePlayerCharacter& Farmer = Spawner.SpawnActor<AVestigePlayerCharacter>();
			Farmer.DispatchBeginPlay();
			UVestigeCharacterMovementComponent* Movement = Farmer.GetCharacterMovement<UVestigeCharacterMovementComponent>();
			UFarmingComponent* Farming = Farmer.FindComponentByClass<UFarmingComponent>();
			ASSERT_THAT(IsNotNull(Movement));
			ASSERT_THAT(IsNotNull(Farming));
			Movement->SetMovementMode(MOVE_Walking);
			const float WalkSpeed = Movement->GetMaxSpeed();

			Farming->StartUsingTool();
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Movement->GetMaxSpeed(), WalkSpeed * 0.5f)));

			Farming->StopUsingTool();
			ASSERT_THAT(IsTrue(Movement->GetMaxSpeed() == WalkSpeed));
		}

		TEST_METHOD(HoldingTheUseKeySlowsJumpsToo)
		{
			AVestigePlayerCharacter& Farmer = Spawner.SpawnActor<AVestigePlayerCharacter>();
			Farmer.DispatchBeginPlay();
			UVestigeCharacterMovementComponent* Movement = Farmer.GetCharacterMovement<UVestigeCharacterMovementComponent>();
			UFarmingComponent* Farming = Farmer.FindComponentByClass<UFarmingComponent>();
			ASSERT_THAT(IsNotNull(Movement));
			ASSERT_THAT(IsNotNull(Farming));
			Movement->SetMovementMode(MOVE_Falling);
			const float FallingSpeed = Movement->GetMaxSpeed();

			Farming->StartUsingTool();

			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Movement->GetMaxSpeed(), FallingSpeed * 0.5f)));
		}

		TEST_METHOD(TheFarmerBlueprintTurnsWithTheVestigeMovementComponent)
		{
			const UClass* FarmerClass =
				LoadClass<AVestigePlayerCharacter>(nullptr, TEXT("/Game/Vestige/Blueprints/BP_PlayerCharacter.BP_PlayerCharacter_C"));
			ASSERT_THAT(IsNotNull(FarmerClass));

			// The Blueprint's rate is tuning, so any rate that turns at all passes.
			const UVestigeCharacterMovementComponent* Movement =
				FarmerClass->GetDefaultObject<AVestigePlayerCharacter>()->GetCharacterMovement<UVestigeCharacterMovementComponent>();
			ASSERT_THAT(IsNotNull(Movement));
			ASSERT_THAT(IsTrue(Movement->RotationRate.Yaw > 0.f));
		}

		TEST_METHOD(HotbarKeyHintsLeaveOutEmptySlots)
		{
			AVestigePlayerCharacter& Farmer = Spawner.SpawnActor<AVestigePlayerCharacter>();
			UVestigeItemData& Hoe = Spawner.SpawnObject<UVestigeItemData>();
			Hoe.SetContentIdForTests(TEXT("Base.Hoe"));
			UVestigeItemData& Can = Spawner.SpawnObject<UVestigeItemData>();
			Can.SetContentIdForTests(TEXT("Base.WateringCan"));
			UVestigeInventoryComponent* Inventory = NewObject<UVestigeInventoryComponent>(&Farmer);
			Inventory->ResetForTests(12, {&Hoe, &Can});
			FVestigeInventorySaveData Bag;
			Bag.Slots = {{0, Hoe.GetContentId(), 1}, {2, Can.GetContentId(), 1}, {10, Hoe.GetContentId(), 1}};
			Inventory->RestoreFromSaveData(Bag);

			// Slot 1 is empty and slot 10 is past the hotbar, so two hints, for the keys 1 and 3.
			const TArray<FVestigeKeyHint> Hints = Farmer.GetHotbarKeyHints(*Inventory);
			ASSERT_THAT(IsTrue(Hints.Num() == 2));
			ASSERT_THAT(IsTrue(Hints[0].Key.ToString() == EKeys::One.GetDisplayName(false).ToString()));
			ASSERT_THAT(IsTrue(Hints[1].Key.ToString() == EKeys::Three.GetDisplayName(false).ToString()));
		}
	};
}

#endif
