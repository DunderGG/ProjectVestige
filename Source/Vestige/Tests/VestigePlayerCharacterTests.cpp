// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/ActorTestSpawner.h"
#include "Content/VestigeContentCatalog.h"
#include "Content/VestigeItemData.h"
#include "CQTest.h"
#include "Farming/FarmingComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Inventory/VestigeInventoryComponent.h"
#include "Inventory/VestigeInventorySaveData.h"
#include "Player/VestigeCharacterMovementComponent.h"
#include "Player/VestigePlayerCharacter.h"
#include "Player/VestigePlayerState.h"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace VestigePlayerCharacterTests
{
	TEST_CLASS(PlayerCharacter, "Vestige")
	{
		FActorTestSpawner Spawner;

		/** The content catalogue lives in the game instance. */
		BEFORE_EACH() { Spawner.InitializeGameSubsystems(); }

		/** An inventory with SlotCount slots on Owner, with Items made known to the content catalogue. */
		UVestigeInventoryComponent* MakeInventory(AActor& Owner, int32 SlotCount, TConstArrayView<const UVestigeItemData*> Items)
		{
			UVestigeContentCatalog* Catalog = UVestigeContentCatalog::Get(&Spawner.GetWorld());
			for (const UVestigeItemData* Item : Items)
			{
				Catalog->AddForTests(*Item);
			}
			UVestigeInventoryComponent* Inventory = NewObject<UVestigeInventoryComponent>(&Owner);
			Inventory->ResetForTests(SlotCount);
			return Inventory;
		}

		/** The game's character Blueprint, which names the input config and the mapping contexts the native class lacks. */
		static UClass* LoadFarmerClass()
		{
			return LoadClass<AVestigePlayerCharacter>(nullptr, TEXT("/Game/Vestige/Blueprints/BP_PlayerCharacter.BP_PlayerCharacter_C"));
		}

		/** A farmer spawned from the game's character Blueprint, not begun play. */
		AVestigePlayerCharacter& SpawnBlueprintFarmer()
		{
			UClass* FarmerClass = LoadFarmerClass();
			check(FarmerClass);
			return Spawner.SpawnActor<AVestigePlayerCharacter>(FActorSpawnParameters(), FarmerClass);
		}

		TEST_METHOD(ActionKeyHintsNameTheBoundKeys)
		{
			const AVestigePlayerCharacter& Farmer = SpawnBlueprintFarmer();

			const TArray<FVestigeKeyHint> Hints = Farmer.GetActionKeyHints(EVestigeHintDevice::KeyboardAndMouse);

			// One hint per action with a description, with its keyboard key: the gamepad keys (X, B) don't show.
			ASSERT_THAT(IsTrue(Hints.Num() == 2));
			ASSERT_THAT(IsTrue(Hints[0].Key.ToString() == TEXT("E")));
			ASSERT_THAT(IsTrue(Hints[1].Key.ToString() == TEXT("F")));
			for (const FVestigeKeyHint& Hint : Hints)
			{
				ASSERT_THAT(IsFalse(Hint.Action.IsEmpty()));
			}
		}

		TEST_METHOD(GamepadActionHintsNameTheGamepadKeys)
		{
			const AVestigePlayerCharacter& Farmer = SpawnBlueprintFarmer();

			const TArray<FVestigeKeyHint> KeyboardHints = Farmer.GetActionKeyHints(EVestigeHintDevice::KeyboardAndMouse);
			const TArray<FVestigeKeyHint> GamepadHints = Farmer.GetActionKeyHints(EVestigeHintDevice::Gamepad);

			// The same actions, with the gamepad's keys in their short names.
			ASSERT_THAT(IsTrue(GamepadHints.Num() == 2));
			ASSERT_THAT(IsTrue(GamepadHints[0].Key.ToString() == TEXT("X")));
			ASSERT_THAT(IsTrue(GamepadHints[1].Key.ToString() == TEXT("B")));
			ASSERT_THAT(IsTrue(KeyboardHints.Num() == GamepadHints.Num()));
			for (int32 Index = 0; Index < GamepadHints.Num(); ++Index)
			{
				ASSERT_THAT(IsTrue(GamepadHints[Index].Action.EqualTo(KeyboardHints[Index].Action)));
			}
		}

		TEST_METHOD(TheInteractKeyFollowsTheDevice)
		{
			const AVestigePlayerCharacter& Farmer = SpawnBlueprintFarmer();

			ASSERT_THAT(IsTrue(Farmer.GetInteractKey(EVestigeHintDevice::KeyboardAndMouse).ToString() == TEXT("F")));
			ASSERT_THAT(IsTrue(Farmer.GetInteractKey(EVestigeHintDevice::Gamepad).ToString() == TEXT("B")));
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

			// As the farming component announces the key when a tool is in hand (with nothing in hand the walk isn't slowed).
			Farming->OnToolUseHeldChanged.Broadcast(true);
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Movement->GetMaxSpeed(), WalkSpeed * 0.5f)));

			Farming->OnToolUseHeldChanged.Broadcast(false);
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

			Farming->OnToolUseHeldChanged.Broadcast(true);

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
			AVestigePlayerCharacter& Farmer = SpawnBlueprintFarmer();
			UVestigeItemData& Hoe = Spawner.SpawnObject<UVestigeItemData>();
			Hoe.SetContentIdForTests(TEXT("Base.Hoe"));
			UVestigeItemData& Can = Spawner.SpawnObject<UVestigeItemData>();
			Can.SetContentIdForTests(TEXT("Base.WateringCan"));
			UVestigeInventoryComponent* Inventory = MakeInventory(Farmer, 12, {&Hoe, &Can});
			FVestigeInventorySaveData Bag;
			Bag.Slots = {{0, Hoe.GetContentId(), 1}, {2, Can.GetContentId(), 1}, {10, Hoe.GetContentId(), 1}};
			Inventory->RestoreFromSaveData(Bag);

			// Slot 1 is empty and slot 10 is past the hotbar, so two hints, for the keys 1 and 3.
			const TArray<FVestigeKeyHint> Hints = Farmer.GetHotbarKeyHints(*Inventory, EVestigeHintDevice::KeyboardAndMouse);
			ASSERT_THAT(IsTrue(Hints.Num() == 2));
			ASSERT_THAT(IsTrue(Hints[0].Key.ToString() == EKeys::One.GetDisplayName(false).ToString()));
			ASSERT_THAT(IsTrue(Hints[1].Key.ToString() == EKeys::Three.GetDisplayName(false).ToString()));
		}

		TEST_METHOD(TheGamepadHotbarHintIsTheSteppingKeys)
		{
			AVestigePlayerCharacter& Farmer = SpawnBlueprintFarmer();
			UVestigeItemData& Hoe = Spawner.SpawnObject<UVestigeItemData>();
			Hoe.SetContentIdForTests(TEXT("Base.Hoe"));
			UVestigeInventoryComponent* Inventory = MakeInventory(Farmer, 12, {&Hoe});

			// An empty hotbar has nothing to step through.
			ASSERT_THAT(IsTrue(Farmer.GetHotbarKeyHints(*Inventory, EVestigeHintDevice::Gamepad).IsEmpty()));

			FVestigeInventorySaveData Bag;
			Bag.Slots = {{0, Hoe.GetContentId(), 1}, {2, Hoe.GetContentId(), 1}};
			Inventory->RestoreFromSaveData(Bag);

			// The gamepad has no slot keys, so one hint: the previous and next keys.
			const TArray<FVestigeKeyHint> Hints = Farmer.GetHotbarKeyHints(*Inventory, EVestigeHintDevice::Gamepad);
			ASSERT_THAT(IsTrue(Hints.Num() == 1));
			ASSERT_THAT(IsTrue(Hints[0].Key.ToString() == TEXT("LB / RB")));
			ASSERT_THAT(IsTrue(Hints[0].Action.ToString() == TEXT("Hotbar")));
		}

		TEST_METHOD(WithoutAnInputConfigTheHintsAreEmpty)
		{
			// The native class names no input config; asking for hints is quiet (an error here would fail the test).
			AVestigePlayerCharacter& Farmer = Spawner.SpawnActor<AVestigePlayerCharacter>();
			UVestigeItemData& Hoe = Spawner.SpawnObject<UVestigeItemData>();
			Hoe.SetContentIdForTests(TEXT("Base.Hoe"));
			UVestigeInventoryComponent* Inventory = MakeInventory(Farmer, 12, {&Hoe});
			FVestigeInventorySaveData Bag;
			Bag.Slots = {{0, Hoe.GetContentId(), 1}};
			Inventory->RestoreFromSaveData(Bag);

			for (const EVestigeHintDevice Device : {EVestigeHintDevice::KeyboardAndMouse, EVestigeHintDevice::Gamepad})
			{
				ASSERT_THAT(IsTrue(Farmer.GetActionKeyHints(Device).IsEmpty()));
				ASSERT_THAT(IsTrue(Farmer.GetHotbarKeyHints(*Inventory, Device).IsEmpty()));
				ASSERT_THAT(IsTrue(Farmer.GetInteractKey(Device).IsEmpty()));
			}
		}

		TEST_METHOD(UnpossessingReleasesTheUseKey)
		{
			// The setup of Vestige.Farming.AFarmerPossessedBeforeBeginPlayFollowsTheHand: the game's player state, with its
			// inventory and ability system, and the farmer Blueprint possessed by a player controller.
			UClass* PlayerStateClass =
				LoadClass<AVestigePlayerState>(nullptr, TEXT("/Game/Vestige/Blueprints/BP_PlayerState.BP_PlayerState_C"));
			ASSERT_THAT(IsNotNull(PlayerStateClass));
			AVestigePlayerState& PlayerState = Spawner.SpawnActor<AVestigePlayerState>(FActorSpawnParameters(), PlayerStateClass);
			PlayerState.DispatchBeginPlay();
			UVestigeInventoryComponent* Inventory = PlayerState.GetInventory();
			ASSERT_THAT(IsNotNull(Inventory));
			APlayerController& Controller = Spawner.SpawnActor<APlayerController>();
			Controller.PlayerState = &PlayerState;
			AVestigePlayerCharacter& Farmer = SpawnBlueprintFarmer();
			Controller.Possess(&Farmer);
			Farmer.DispatchBeginPlay();
			UVestigeCharacterMovementComponent* Movement = Farmer.GetCharacterMovement<UVestigeCharacterMovementComponent>();
			UFarmingComponent* Farming = Farmer.FindComponentByClass<UFarmingComponent>();
			ASSERT_THAT(IsNotNull(Movement));
			ASSERT_THAT(IsNotNull(Farming));
			Movement->SetMovementMode(MOVE_Walking);
			const float WalkSpeed = Movement->GetMaxSpeed();
			FVestigeInventorySaveData Bag;
			Bag.Slots = {{0, TEXT("Base.Hoe"), 1}};
			ASSERT_THAT(IsTrue(Inventory->RestoreFromSaveData(Bag) == 0));
			TArray<bool> HeldChanges;
			const FDelegateHandle Recorder = Farming->OnToolUseHeldChanged.AddLambda(
				[&HeldChanges](bool bHeld)
				{
					HeldChanges.Add(bHeld);
				});

			Farming->StartUsingTool();
			ASSERT_THAT(IsTrue(HeldChanges.Num() == 1 && HeldChanges[0]));
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Movement->GetMaxSpeed(), WalkSpeed * 0.5f)));

			Controller.UnPossess();

			// The input component, with the key's release binding, is gone, so only UnPossessed can release the key.
			ASSERT_THAT(IsTrue(HeldChanges.Num() == 2 && !HeldChanges[1]));
			ASSERT_THAT(IsTrue(Movement->GetMaxSpeed() == WalkSpeed));
			Farming->OnToolUseHeldChanged.Remove(Recorder);
		}
	};
}

#endif
