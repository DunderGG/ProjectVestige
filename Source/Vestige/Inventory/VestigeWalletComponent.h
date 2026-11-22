// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/VestigeSaveable.h"
#include "VestigeWalletComponent.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnVestigeMoneyChanged);

namespace VestigeWallet
{
	/** The most money a wallet holds: far beyond any season, and low enough that two amounts add up in int64 without overflow. */
	constexpr int32 MaxMoney = 999999999;
}

/**
 * The player's money: one currency, a whole number between 0 and VestigeWallet::MaxMoney. Lives on the player state beside
 * the inventory, so it outlives the pawn; it is not the inventory's, which is slots and their rules. Broadcasts
 * OnMoneyChanged after every change, including a new game and a restore. Saves its amount under Base.Wallet through
 * UVestigeSaveRegistry while it is in play; a game with no saved amount (new, or from before coins) starts with StartingMoney.
 * Spending is SpendMoney, called by the shop rule (VestigeShop::Buy). Systems.md, sections 25 and 26.
 */
UCLASS(ClassGroup = (Inventory), meta = (BlueprintSpawnableComponent))
class UVestigeWalletComponent : public UActorComponent, public IVestigeSaveable
{
	GENERATED_BODY()

public:
	UVestigeWalletComponent();

	/** The wallet on the player state of World's first player controller, or nullptr. Single-player: co-op decides this. */
	static UVestigeWalletComponent* FindForFirstPlayer(const UWorld* World);

	/** The wallet on Pawn's player state, or nullptr: Pawn isn't a pawn, has no player state, or that has no wallet. */
	static UVestigeWalletComponent* FindForPawn(const AActor* Pawn);

	int32 GetMoney() const { return Money; }

	/** The purse a new game starts with: StartingMoney, within 0 to MaxMoney. */
	int32 GetStartingMoney() const;

	/**
	 * Adds Amount and broadcasts OnMoneyChanged. Returns false, changing nothing, for 0 or less. What would take the
	 * wallet above MaxMoney is lost, with a warning.
	 */
	bool AddMoney(int32 Amount);

	/**
	 * Takes Amount and broadcasts OnMoneyChanged. Returns false, changing nothing, for 0 or less or more than the wallet holds:
	 * there is no debt.
	 */
	bool SpendMoney(int32 Amount);

	/** Starts with the starting purse and broadcasts. */
	virtual void StartNewGame() override;

	virtual FName GetSaveKey() const override;
	virtual int32 GetSaveVersion() const override;
	virtual FVestigeSaveRecord WriteSaveRecord() const override;
	virtual bool ReadSaveRecord(const FVestigeSaveRecord& Record, int32& OutLeftOutCount) override;

	/** Broadcast after every change, including a new game and a restore. It carries no value: listeners read GetMoney. */
	FOnVestigeMoneyChanged OnMoneyChanged;

protected:
	/** Takes its saved amount, or the starting purse when there is none. */
	virtual void BeginPlay() override;

	/** Keeps its amount for the next map before it goes. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** What a new game starts with, in coins. Tuned on BP_PlayerState without a build. */
	UPROPERTY(EditDefaultsOnly, Category = "Wallet", meta = (AllowPrivateAccess = "true", ClampMin = 0))
	int32 StartingMoney = 100;

	int32 Money = 0;
};
