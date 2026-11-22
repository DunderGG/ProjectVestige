// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/ActorTestSpawner.h"
#include "Core/VestigeSaveRecord.h"
#include "Core/VestigeSaveRegistry.h"
#include "CQTest.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/VestigeWalletComponent.h"
#include "Inventory/VestigeWalletSaveData.h"
#include "Player/VestigePlayerState.h"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace VestigeWalletTests
{
	const TCHAR* const PlayerStatePath = TEXT("/Game/Vestige/Blueprints/BP_PlayerState.BP_PlayerState_C");
	const TCHAR* const GameModePath = TEXT("/Game/Vestige/Blueprints/BP_GameMode.BP_GameMode_C");

	TEST_CLASS(Wallet, "Vestige")
	{
		FActorTestSpawner Spawner;

		/** The wallet registers with the save registry in the game instance when it begins play. */
		BEFORE_EACH() { Spawner.InitializeGameSubsystems(); }

		UVestigeSaveRegistry& GetRegistry() { return *UVestigeSaveRegistry::Get(&Spawner.GetWorld()); }

		/** A BP_PlayerState in play, and so its wallet. */
		AVestigePlayerState& SpawnPlayerState()
		{
			UClass* PlayerStateClass = LoadClass<AVestigePlayerState>(nullptr, PlayerStatePath);
			check(PlayerStateClass);
			AVestigePlayerState& PlayerState = Spawner.SpawnActor<AVestigePlayerState>(FActorSpawnParameters(), PlayerStateClass);
			PlayerState.DispatchBeginPlay();
			return PlayerState;
		}

		static FVestigeSaveRecord MakeRecord(int32 Money)
		{
			FVestigeWalletSaveData Data;
			Data.Money = Money;
			return FVestigeSaveRecord::Write(Data);
		}

		TEST_METHOD(ANewGameStartsWithTheStartingPurse)
		{
			UVestigeWalletComponent* Wallet = SpawnPlayerState().GetWallet();
			ASSERT_THAT(IsNotNull(Wallet));

			ASSERT_THAT(IsTrue(Wallet->GetStartingMoney() > 0));
			ASSERT_THAT(AreEqual(Wallet->GetMoney(), Wallet->GetStartingMoney()));
		}

		TEST_METHOD(ASaveFromBeforeCoinsStartsWithThePurse)
		{
			// A save with no Base.Wallet record, as every save made before coins.
			GetRegistry().Load({});

			UVestigeWalletComponent* Wallet = SpawnPlayerState().GetWallet();
			ASSERT_THAT(IsNotNull(Wallet));

			ASSERT_THAT(AreEqual(Wallet->GetMoney(), Wallet->GetStartingMoney()));
		}

		TEST_METHOD(ASavedAmountIsRestoredWhenThePlayerBeginsPlay)
		{
			GetRegistry().Load({{TEXT("Base.Wallet"), MakeRecord(250)}});

			UVestigeWalletComponent* Wallet = SpawnPlayerState().GetWallet();
			ASSERT_THAT(IsNotNull(Wallet));

			ASSERT_THAT(AreEqual(Wallet->GetMoney(), 250));
		}

		TEST_METHOD(MoneyIsSavedAndRestored)
		{
			UVestigeWalletComponent* Wallet = SpawnPlayerState().GetWallet();
			ASSERT_THAT(IsNotNull(Wallet));
			ASSERT_THAT(IsTrue(Wallet->GetSaveKey() == FName(TEXT("Base.Wallet"))));
			ASSERT_THAT(AreEqual(Wallet->GetSaveVersion(), 1));
			ASSERT_THAT(IsTrue(Wallet->AddMoney(150)));
			const int32 Saved = Wallet->GetMoney();
			const FVestigeSaveRecord Record = Wallet->WriteSaveRecord();

			Wallet->StartNewGame();
			ASSERT_THAT(AreEqual(Wallet->GetMoney(), Wallet->GetStartingMoney()));
			int32 LeftOut = 0;
			ASSERT_THAT(IsTrue(Wallet->ReadSaveRecord(Record, LeftOut)));

			ASSERT_THAT(AreEqual(Wallet->GetMoney(), Saved));
			ASSERT_THAT(AreEqual(LeftOut, 0));
		}

		TEST_METHOD(TheRegistryCollectsTheMoneyForTheSave)
		{
			UVestigeWalletComponent* Wallet = SpawnPlayerState().GetWallet();
			ASSERT_THAT(IsNotNull(Wallet));
			ASSERT_THAT(IsTrue(Wallet->AddMoney(75)));

			const FVestigeSaveRecord* Record = GetRegistry().CollectRecords().Find(TEXT("Base.Wallet"));
			ASSERT_THAT(IsNotNull(Record));
			FVestigeWalletSaveData Data;
			ASSERT_THAT(IsTrue(Record->Read(Data)));
			ASSERT_THAT(AreEqual(Data.Money, Wallet->GetMoney()));
		}

		TEST_METHOD(ARestoredAmountIsClampedToTheLimits)
		{
			UVestigeWalletComponent* Wallet = SpawnPlayerState().GetWallet();
			ASSERT_THAT(IsNotNull(Wallet));
			int32 LeftOut = 0;

			TestRunner->AddExpectedMessagePlain(TEXT("is outside 0 to"), ELogVerbosity::Warning);
			ASSERT_THAT(IsTrue(Wallet->ReadSaveRecord(MakeRecord(-5), LeftOut)));
			ASSERT_THAT(AreEqual(Wallet->GetMoney(), 0));

			TestRunner->AddExpectedMessagePlain(TEXT("is outside 0 to"), ELogVerbosity::Warning);
			ASSERT_THAT(IsTrue(Wallet->ReadSaveRecord(MakeRecord(MAX_int32), LeftOut)));
			ASSERT_THAT(AreEqual(Wallet->GetMoney(), VestigeWallet::MaxMoney));

			// An amount inside the limits is taken as it is, with no warning.
			ASSERT_THAT(IsTrue(Wallet->ReadSaveRecord(MakeRecord(VestigeWallet::MaxMoney), LeftOut)));
			ASSERT_THAT(AreEqual(Wallet->GetMoney(), VestigeWallet::MaxMoney));
			ASSERT_THAT(IsTrue(Wallet->ReadSaveRecord(MakeRecord(0), LeftOut)));
			ASSERT_THAT(AreEqual(Wallet->GetMoney(), 0));
		}

		TEST_METHOD(AddingMoneyAddsAndBroadcasts)
		{
			UVestigeWalletComponent* Wallet = SpawnPlayerState().GetWallet();
			ASSERT_THAT(IsNotNull(Wallet));
			const int32 Before = Wallet->GetMoney();
			int32 Broadcasts = 0;
			int32 MoneyAtBroadcast = INDEX_NONE;
			Wallet->OnMoneyChanged.AddLambda(
				[&]()
				{
					++Broadcasts;
					MoneyAtBroadcast = Wallet->GetMoney();
				});

			ASSERT_THAT(IsTrue(Wallet->AddMoney(30)));

			ASSERT_THAT(AreEqual(Wallet->GetMoney(), Before + 30));
			ASSERT_THAT(AreEqual(Broadcasts, 1));
			ASSERT_THAT(AreEqual(MoneyAtBroadcast, Before + 30));
		}

		TEST_METHOD(AnAddOfZeroOrLessChangesNothing)
		{
			UVestigeWalletComponent* Wallet = SpawnPlayerState().GetWallet();
			ASSERT_THAT(IsNotNull(Wallet));
			const int32 Before = Wallet->GetMoney();
			int32 Broadcasts = 0;
			Wallet->OnMoneyChanged.AddLambda(
				[&Broadcasts]()
				{
					++Broadcasts;
				});

			ASSERT_THAT(IsFalse(Wallet->AddMoney(0)));
			ASSERT_THAT(IsFalse(Wallet->AddMoney(-20)));

			ASSERT_THAT(AreEqual(Wallet->GetMoney(), Before));
			ASSERT_THAT(AreEqual(Broadcasts, 0));
		}

		TEST_METHOD(MoneyStopsAtTheMaximum)
		{
			UVestigeWalletComponent* Wallet = SpawnPlayerState().GetWallet();
			ASSERT_THAT(IsNotNull(Wallet));

			TestRunner->AddExpectedMessagePlain(TEXT("passes the limit"), ELogVerbosity::Warning);
			ASSERT_THAT(IsTrue(Wallet->AddMoney(VestigeWallet::MaxMoney)));
			ASSERT_THAT(AreEqual(Wallet->GetMoney(), VestigeWallet::MaxMoney));

			// A wallet at the limit takes no more, and the sum never wraps around.
			TestRunner->AddExpectedMessagePlain(TEXT("passes the limit"), ELogVerbosity::Warning);
			ASSERT_THAT(IsTrue(Wallet->AddMoney(MAX_int32)));
			ASSERT_THAT(AreEqual(Wallet->GetMoney(), VestigeWallet::MaxMoney));
		}

		TEST_METHOD(ANewGameRestoresThePurseAndBroadcasts)
		{
			UVestigeWalletComponent* Wallet = SpawnPlayerState().GetWallet();
			ASSERT_THAT(IsNotNull(Wallet));
			ASSERT_THAT(IsTrue(Wallet->AddMoney(500)));
			int32 Broadcasts = 0;
			Wallet->OnMoneyChanged.AddLambda(
				[&Broadcasts]()
				{
					++Broadcasts;
				});

			Wallet->StartNewGame();

			ASSERT_THAT(AreEqual(Wallet->GetMoney(), Wallet->GetStartingMoney()));
			ASSERT_THAT(AreEqual(Broadcasts, 1));
		}

		TEST_METHOD(ThePlayerStartsWithAPurseOf100)
		{
			// The game mode's own player state class, so a value tuned on BP_PlayerState is seen here.
			const UClass* GameModeClass = LoadClass<AGameModeBase>(nullptr, GameModePath);
			ASSERT_THAT(IsNotNull(GameModeClass));
			const UClass* PlayerStateClass = GameModeClass->GetDefaultObject<AGameModeBase>()->PlayerStateClass;
			ASSERT_THAT(IsTrue(PlayerStateClass && PlayerStateClass->IsChildOf<AVestigePlayerState>()));

			const UVestigeWalletComponent* Wallet = GetDefault<AVestigePlayerState>(PlayerStateClass)->GetWallet();
			ASSERT_THAT(IsNotNull(Wallet));

			ASSERT_THAT(AreEqual(Wallet->GetStartingMoney(), 100));
		}

		TEST_METHOD(TheFirstPlayersWalletIsFoundThroughTheController)
		{
			ASSERT_THAT(IsNull(UVestigeWalletComponent::FindForFirstPlayer(&Spawner.GetWorld())));
			AVestigePlayerState& PlayerState = SpawnPlayerState();
			APlayerController& Controller = Spawner.SpawnActor<APlayerController>();
			Controller.PlayerState = &PlayerState;

			ASSERT_THAT(IsTrue(UVestigeWalletComponent::FindForFirstPlayer(&Spawner.GetWorld()) == PlayerState.GetWallet()));
			ASSERT_THAT(IsNull(UVestigeWalletComponent::FindForFirstPlayer(nullptr)));
		}

		// ---- Spending ----

		TEST_METHOD(SpendingTakesTheAmountAndBroadcasts)
		{
			UVestigeWalletComponent* Wallet = SpawnPlayerState().GetWallet();
			ASSERT_THAT(IsNotNull(Wallet));
			const int32 Before = Wallet->GetMoney();
			ASSERT_THAT(IsTrue(Before > 30));
			int32 Broadcasts = 0;
			int32 MoneyAtBroadcast = INDEX_NONE;
			Wallet->OnMoneyChanged.AddLambda(
				[&]()
				{
					++Broadcasts;
					MoneyAtBroadcast = Wallet->GetMoney();
				});

			ASSERT_THAT(IsTrue(Wallet->SpendMoney(30)));

			ASSERT_THAT(AreEqual(Wallet->GetMoney(), Before - 30));
			ASSERT_THAT(AreEqual(Broadcasts, 1));
			ASSERT_THAT(AreEqual(MoneyAtBroadcast, Before - 30));
		}

		TEST_METHOD(SpendingMoreThanThePurseChangesNothing)
		{
			UVestigeWalletComponent* Wallet = SpawnPlayerState().GetWallet();
			ASSERT_THAT(IsNotNull(Wallet));
			const int32 Before = Wallet->GetMoney();
			int32 Broadcasts = 0;
			Wallet->OnMoneyChanged.AddLambda(
				[&Broadcasts]()
				{
					++Broadcasts;
				});

			// There is no debt, by one coin or by a lot.
			ASSERT_THAT(IsFalse(Wallet->SpendMoney(Before + 1)));
			ASSERT_THAT(IsFalse(Wallet->SpendMoney(MAX_int32)));

			ASSERT_THAT(AreEqual(Wallet->GetMoney(), Before));
			ASSERT_THAT(AreEqual(Broadcasts, 0));
		}

		TEST_METHOD(ASpendOfZeroOrLessChangesNothing)
		{
			UVestigeWalletComponent* Wallet = SpawnPlayerState().GetWallet();
			ASSERT_THAT(IsNotNull(Wallet));
			const int32 Before = Wallet->GetMoney();
			int32 Broadcasts = 0;
			Wallet->OnMoneyChanged.AddLambda(
				[&Broadcasts]()
				{
					++Broadcasts;
				});

			// A negative spend would be a hidden add.
			ASSERT_THAT(IsFalse(Wallet->SpendMoney(0)));
			ASSERT_THAT(IsFalse(Wallet->SpendMoney(-20)));
			ASSERT_THAT(IsFalse(Wallet->SpendMoney(MIN_int32)));

			ASSERT_THAT(AreEqual(Wallet->GetMoney(), Before));
			ASSERT_THAT(AreEqual(Broadcasts, 0));
		}

		TEST_METHOD(SpendingTheWholePurseLeavesZero)
		{
			UVestigeWalletComponent* Wallet = SpawnPlayerState().GetWallet();
			ASSERT_THAT(IsNotNull(Wallet));

			ASSERT_THAT(IsTrue(Wallet->SpendMoney(Wallet->GetMoney())));

			ASSERT_THAT(AreEqual(Wallet->GetMoney(), 0));
			// An empty purse can't spend even one coin.
			ASSERT_THAT(IsFalse(Wallet->SpendMoney(1)));
			ASSERT_THAT(AreEqual(Wallet->GetMoney(), 0));
		}

		TEST_METHOD(FindForPawnFindsThePlayerStatesWallet)
		{
			AVestigePlayerState& PlayerState = SpawnPlayerState();
			APlayerController& Controller = Spawner.SpawnActor<APlayerController>();
			Controller.PlayerState = &PlayerState;
			APawn& Pawn = Spawner.SpawnActor<APawn>();
			ASSERT_THAT(IsNull(UVestigeWalletComponent::FindForPawn(&Pawn)));

			Controller.Possess(&Pawn);

			ASSERT_THAT(IsTrue(UVestigeWalletComponent::FindForPawn(&Pawn) == PlayerState.GetWallet()));
			// Not a pawn, and no actor at all.
			ASSERT_THAT(IsNull(UVestigeWalletComponent::FindForPawn(&PlayerState)));
			ASSERT_THAT(IsNull(UVestigeWalletComponent::FindForPawn(nullptr)));
		}
	};
}

#endif
