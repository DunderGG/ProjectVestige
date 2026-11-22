// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Inventory/VestigeWalletComponent.h"

#include "Core/VestigeSaveRegistry.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Inventory/VestigeInventoryLog.h"
#include "Inventory/VestigeWalletSaveData.h"

UVestigeWalletComponent::UVestigeWalletComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

UVestigeWalletComponent* UVestigeWalletComponent::FindForFirstPlayer(const UWorld* World)
{
	const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	const APlayerState* PlayerState = Controller ? Controller->PlayerState.Get() : nullptr;
	return PlayerState ? PlayerState->FindComponentByClass<UVestigeWalletComponent>() : nullptr;
}

UVestigeWalletComponent* UVestigeWalletComponent::FindForPawn(const AActor* Pawn)
{
	const APawn* AsPawn = Cast<APawn>(Pawn);
	const APlayerState* PlayerState = AsPawn ? AsPawn->GetPlayerState() : nullptr;
	return PlayerState ? PlayerState->FindComponentByClass<UVestigeWalletComponent>() : nullptr;
}

void UVestigeWalletComponent::BeginPlay()
{
	Super::BeginPlay();

	// The registry hands the saved amount over at once, or has the wallet start a new game.
	Money = GetStartingMoney();
	UVestigeSaveRegistry::RegisterWorldOwner(GetWorld(), *this);
}

void UVestigeWalletComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UVestigeSaveRegistry::UnregisterWorldOwner(GetWorld(), *this);
	Super::EndPlay(EndPlayReason);
}

int32 UVestigeWalletComponent::GetStartingMoney() const
{
	return FMath::Clamp(StartingMoney, 0, VestigeWallet::MaxMoney);
}

bool UVestigeWalletComponent::AddMoney(int32 Amount)
{
	if (Amount <= 0)
	{
		return false;
	}

	const int64 Sum = static_cast<int64>(Money) + Amount;
	if (Sum > VestigeWallet::MaxMoney)
	{
		VESTIGE_LOG(this, LogVestigeInventory, Warning, "Adding {Amount} to {Money} passes the limit of {Max}, so the rest is lost.", Amount,
			Money, VestigeWallet::MaxMoney);
	}
	Money = static_cast<int32>(FMath::Min<int64>(Sum, VestigeWallet::MaxMoney));
	VESTIGE_LOG(this, LogVestigeInventory, Verbose, "Added {Amount}, so the wallet holds {Money}.", Amount, Money);
	OnMoneyChanged.Broadcast();
	return true;
}

bool UVestigeWalletComponent::TrySpendMoney(int32 Amount)
{
	if (Amount <= 0 || Amount > Money)
	{
		return false;
	}

	Money -= Amount;
	VESTIGE_LOG(this, LogVestigeInventory, Verbose, "Spent {Amount}, so the wallet holds {Money}.", Amount, Money);
	OnMoneyChanged.Broadcast();
	return true;
}

void UVestigeWalletComponent::StartNewGame()
{
	Money = GetStartingMoney();
	OnMoneyChanged.Broadcast();
}

FName UVestigeWalletComponent::GetSaveKey() const
{
	static const FName Key(TEXT("Base.Wallet"));
	return Key;
}

int32 UVestigeWalletComponent::GetSaveVersion() const
{
	return FVestigeWalletSaveData::Version;
}

FVestigeSaveRecord UVestigeWalletComponent::WriteSaveRecord() const
{
	FVestigeWalletSaveData Data;
	Data.Money = Money;
	return FVestigeSaveRecord::Write(Data);
}

bool UVestigeWalletComponent::ReadSaveRecord(const FVestigeSaveRecord& Record, int32& OutLeftOutCount)
{
	FVestigeWalletSaveData Data;
	if (!Record.Read(Data))
	{
		return false;
	}

	// A hand-edited or modded save may hold an amount the game never makes.
	const int32 Restored = FMath::Clamp(Data.Money, 0, VestigeWallet::MaxMoney);
	if (Restored != Data.Money)
	{
		VESTIGE_LOG(this, LogVestigeInventory, Warning, "The saved amount {Saved} is outside 0 to {Max}, so {Restored} is used.", Data.Money,
			VestigeWallet::MaxMoney, Restored);
	}
	Money = Restored;
	OnMoneyChanged.Broadcast();
	return true;
}
