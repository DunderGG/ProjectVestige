// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/VestigeExhaustedEffect.h"
#include "AbilitySystem/VestigeStaminaSet.h"
#include "AbilitySystemComponent.h"
#include "Components/ActorTestSpawner.h"
#include "CQTest.h"
#include "Core/VestigeSaveRecord.h"
#include "Core/VestigeSaveRegistry.h"
#include "Farming/FarmingComponent.h"
#include "Farming/VestigeRestComponent.h"
#include "Inventory/VestigeInventoryComponent.h"
#include "Player/VestigePlayerCharacter.h"
#include "Player/VestigePlayerState.h"
#include "Tests/VestigeTestSaveable.h"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace VestigeSaveRegistryTests
{
	TEST_CLASS(SaveRegistry, "Vestige")
	{
		FActorTestSpawner Spawner;

		static inline const FName TestKey = TEXT("Base.Test");
		static inline const FName ModKey = TEXT("SomeMod.Thing");

		UVestigeSaveRegistry& GetRegistry()
		{
			if (!UVestigeSaveRegistry::Get(&Spawner.GetWorld()))
			{
				Spawner.InitializeGameSubsystems();
			}
			return *UVestigeSaveRegistry::Get(&Spawner.GetWorld());
		}

		UVestigeTestSaveable& MakeSaveable(FName Key, int32 OwnerVersion = FVestigeTestSaveData::Version)
		{
			UVestigeTestSaveable& Saveable = Spawner.SpawnObject<UVestigeTestSaveable>();
			Saveable.Setup(Key, OwnerVersion);
			return Saveable;
		}

		static FVestigeSaveRecord MakeRecord(int32 Value, FName Name = NAME_None)
		{
			FVestigeTestSaveData Data;
			Data.Value = Value;
			Data.Name = Name;
			return FVestigeSaveRecord::Write(Data);
		}

		static int32 ReadValue(const FVestigeSaveRecord& Record)
		{
			FVestigeTestSaveData Data;
			return Record.Read(Data) ? Data.Value : INDEX_NONE;
		}

		TEST_METHOD(ARecordRoundTripsItsStruct)
		{
			FVestigeTestSaveData Read;
			const bool bRead = MakeRecord(42, TEXT("Base.Turnip")).Read(Read);

			ASSERT_THAT(IsTrue(bRead));
			ASSERT_THAT(IsTrue(Read.Value == 42));
			ASSERT_THAT(IsTrue(Read.Name == TEXT("Base.Turnip")));
		}

		TEST_METHOD(AnEmptyRecordCantBeRead)
		{
			FVestigeTestSaveData Read;

			ASSERT_THAT(IsFalse(FVestigeSaveRecord().Read(Read)));
		}

		TEST_METHOD(AnOwnerRegisteringAfterALoadGetsItsPartAtOnce)
		{
			UVestigeSaveRegistry& Registry = GetRegistry();
			Registry.Load({{TestKey, MakeRecord(7)}});
			UVestigeTestSaveable& Saveable = MakeSaveable(TestKey);

			Registry.RegisterGameOwner(Saveable);

			ASSERT_THAT(IsTrue(Saveable.State.Value == 7));
			ASSERT_THAT(IsTrue(Saveable.NewGameCount == 0));
		}

		TEST_METHOD(AnOwnerRegisteredBeforeALoadGetsItsPartWhenItLoads)
		{
			UVestigeSaveRegistry& Registry = GetRegistry();
			UVestigeTestSaveable& Saveable = MakeSaveable(TestKey);
			Registry.RegisterGameOwner(Saveable);

			Registry.Load({{TestKey, MakeRecord(7)}});

			ASSERT_THAT(IsTrue(Saveable.State.Value == 7));
		}

		TEST_METHOD(AnOwnerWithoutAPartStartsANewGame)
		{
			UVestigeTestSaveable& Saveable = MakeSaveable(TestKey);

			GetRegistry().RegisterGameOwner(Saveable);

			ASSERT_THAT(IsTrue(Saveable.NewGameCount == 1));
		}

		TEST_METHOD(PartsNoOwnerReadsAreKeptWhenSaving)
		{
			UVestigeSaveRegistry& Registry = GetRegistry();
			Registry.Load({{ModKey, MakeRecord(99)}});
			UVestigeTestSaveable& Saveable = MakeSaveable(TestKey);
			Registry.RegisterGameOwner(Saveable);
			Saveable.State.Value = 5;

			const TMap<FName, FVestigeSaveRecord>& Records = Registry.CollectRecords();

			ASSERT_THAT(IsTrue(ReadValue(Records[ModKey]) == 99));
			ASSERT_THAT(IsTrue(ReadValue(Records[TestKey]) == 5));
		}

		TEST_METHOD(AnOlderPartIsLeftOutAndTheOwnerStartsANewGame)
		{
			// Before the first public build there are no upgrades, so an older part is dropped and the rest loads.
			UVestigeSaveRegistry& Registry = GetRegistry();
			Registry.Load({{TestKey, MakeRecord(7)}});
			UVestigeTestSaveable& Saveable = MakeSaveable(TestKey, FVestigeTestSaveData::Version + 1);

			Registry.RegisterGameOwner(Saveable);

			ASSERT_THAT(IsTrue(Saveable.State.Value == 0));
			ASSERT_THAT(IsTrue(Saveable.NewGameCount == 1));
			ASSERT_THAT(IsTrue(Registry.GetLeftOutCount() == 1));
			ASSERT_THAT(IsFalse(Registry.HasNewerParts()));
		}

		TEST_METHOD(ANewerPartIsNotReadAndStopsSaving)
		{
			UVestigeSaveRegistry& Registry = GetRegistry();
			Registry.Load({{TestKey, MakeRecord(7)}});
			UVestigeTestSaveable& Saveable = MakeSaveable(TestKey, FVestigeTestSaveData::Version - 1);

			Assert.ExpectError(TEXT("is from a newer build than this one"));
			Registry.RegisterGameOwner(Saveable);

			ASSERT_THAT(IsTrue(Saveable.State.Value == 0));
			ASSERT_THAT(IsTrue(Registry.HasNewerParts()));
		}

		TEST_METHOD(ASecondOwnerWithTheSameKeyIsNotRegistered)
		{
			UVestigeSaveRegistry& Registry = GetRegistry();
			UVestigeTestSaveable& First = MakeSaveable(TestKey);
			UVestigeTestSaveable& Second = MakeSaveable(TestKey);
			Registry.RegisterGameOwner(First);
			First.State.Value = 1;

			Registry.RegisterGameOwner(Second);
			Second.State.Value = 2;

			ASSERT_THAT(IsTrue(ReadValue(Registry.CollectRecords()[TestKey]) == 1));
		}

		TEST_METHOD(AWorldOwnerLeavingKeepsItsPartForTheNextOne)
		{
			const UWorld* World = &Spawner.GetWorld();
			GetRegistry();
			UVestigeTestSaveable& Leaving = MakeSaveable(TestKey);
			UVestigeSaveRegistry::RegisterWorldOwner(World, Leaving);
			Leaving.State.Value = 3;

			UVestigeSaveRegistry::UnregisterWorldOwner(World, Leaving);
			UVestigeTestSaveable& Arriving = MakeSaveable(TestKey);
			UVestigeSaveRegistry::RegisterWorldOwner(World, Arriving);

			ASSERT_THAT(IsTrue(Arriving.State.Value == 3));
		}

		TEST_METHOD(ATearingDownMapKeepsItsOwnersPartsAndForgetsThem)
		{
			UWorld& World = Spawner.GetWorld();
			UVestigeSaveRegistry& Registry = GetRegistry();
			UVestigeTestSaveable& Saveable = MakeSaveable(TestKey);
			UVestigeSaveRegistry::RegisterWorldOwner(&World, Saveable);
			Saveable.State.Value = 3;

			Registry.KeepWorldOwnersPartsForTests(World);
			Saveable.State.Value = 4;
			UVestigeSaveRegistry::UnregisterWorldOwner(&World, Saveable);

			// The part from the start of the teardown: later, the owner may no longer reach what it saves.
			ASSERT_THAT(IsTrue(ReadValue(Registry.CollectRecords()[TestKey]) == 3));
		}

		TEST_METHOD(ANewGameResetsGameOwnersAndForgetsWorldOwners)
		{
			const UWorld* World = &Spawner.GetWorld();
			UVestigeSaveRegistry& Registry = GetRegistry();
			UVestigeTestSaveable& GameOwner = MakeSaveable(TestKey);
			UVestigeTestSaveable& WorldOwner = MakeSaveable(ModKey);
			Registry.RegisterGameOwner(GameOwner);
			UVestigeSaveRegistry::RegisterWorldOwner(World, WorldOwner);
			GameOwner.State.Value = 1;
			WorldOwner.State.Value = 2;

			Registry.StartNewGame();
			UVestigeSaveRegistry::UnregisterWorldOwner(World, WorldOwner);

			// The old map's owner leaves after the new game started, and what it holds belongs to the old game.
			const TMap<FName, FVestigeSaveRecord>& Records = Registry.CollectRecords();
			ASSERT_THAT(IsTrue(GameOwner.State.Value == 0));
			ASSERT_THAT(IsFalse(Records.Contains(ModKey)));
			ASSERT_THAT(IsTrue(Records.Contains(TestKey)));
		}

		/** The player as a map has it: the player state Blueprint and the player Blueprint possessing it, both in play. */
		struct FPlayer
		{
			AVestigePlayerState* PlayerState = nullptr;
			AVestigePlayerCharacter* Character = nullptr;
		};

		FPlayer SpawnPlayer()
		{
			UClass* PlayerStateClass =
				LoadClass<AVestigePlayerState>(nullptr, TEXT("/Game/Vestige/Blueprints/BP_PlayerState.BP_PlayerState_C"));
			UClass* CharacterClass =
				LoadClass<AVestigePlayerCharacter>(nullptr, TEXT("/Game/Vestige/Blueprints/BP_PlayerCharacter.BP_PlayerCharacter_C"));
			FPlayer Player;
			if (!PlayerStateClass || !CharacterClass)
			{
				return Player;
			}
			Player.PlayerState = &Spawner.SpawnActor<AVestigePlayerState>(FActorSpawnParameters(), PlayerStateClass);
			Player.Character = &Spawner.SpawnActor<AVestigePlayerCharacter>(FActorSpawnParameters(), CharacterClass);
			Player.Character->SetPlayerState(Player.PlayerState);
			Player.PlayerState->DispatchBeginPlay();
			Player.Character->DispatchBeginPlay();
			return Player;
		}

		TEST_METHOD(ThePlayersDataSurvivesReopeningTheMap)
		{
			UVestigeSaveRegistry& Registry = GetRegistry();
			const FPlayer Before = SpawnPlayer();
			ASSERT_THAT(IsNotNull(Before.Character));
			UVestigeInventoryComponent* Inventory = Before.PlayerState->GetInventory();
			UAbilitySystemComponent* AbilitySystem = Before.PlayerState->GetAbilitySystemComponent();
			UFarmingComponent* Farming = Before.Character->FindComponentByClass<UFarmingComponent>();
			ASSERT_THAT(IsNotNull(Farming));
			const FPrimaryAssetId SeedId = Farming->GetSeedItemId();
			ASSERT_THAT(IsTrue(Inventory->TryAddItem(SeedId, 3)));
			const int32 Seeds = Inventory->GetItemCount(SeedId);
			AbilitySystem->SetNumericAttributeBase(UVestigeStaminaSet::GetStaminaAttribute(), 37.f);
			AbilitySystem->ApplyGameplayEffectToSelf(GetDefault<UVestigeExhaustedEffect>(), 1.f, AbilitySystem->MakeEffectContext());
			Farming->SelectToolbarSlot(2);
			const UVestigeToolData* Tool = Farming->GetCurrentTool();

			// What opening a map does: the old map's owners give their parts as it starts tearing down, then its actors go.
			Registry.KeepWorldOwnersPartsForTests(Spawner.GetWorld());
			Before.Character->Destroy();
			Before.PlayerState->Destroy();
			const FPlayer After = SpawnPlayer();

			const UAbilitySystemComponent* NewAbilitySystem = After.PlayerState->GetAbilitySystemComponent();
			ASSERT_THAT(IsTrue(After.PlayerState->GetInventory()->GetItemCount(SeedId) == Seeds));
			ASSERT_THAT(IsTrue(NewAbilitySystem->GetNumericAttributeBase(UVestigeStaminaSet::GetStaminaAttribute()) == 37.f));
			ASSERT_THAT(IsTrue(After.Character->FindComponentByClass<UVestigeRestComponent>()->IsExhausted()));
			ASSERT_THAT(IsTrue(After.Character->FindComponentByClass<UFarmingComponent>()->GetCurrentTool() == Tool));
		}
	};
}

#endif
