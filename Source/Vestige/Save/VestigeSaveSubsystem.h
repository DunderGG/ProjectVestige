// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "VestigeSaveSubsystem.generated.h"

class AVestigeDayCycle;
class AFarmGrid;
class UAbilitySystemComponent;
class UFarmingComponent;
class UVestigeSaveGame;
class UVestigeInventoryComponent;

/** What reading one save slot found. */
struct FVestigeSaveSlotRead
{
	/** True if the slot has a file, readable or not. */
	bool bExists = false;

	/** The save the file holds, or nullptr if there is no file or it couldn't be read as a farm save. Only used during loading. */
	UVestigeSaveGame* Save = nullptr;
};

/** What loading does with what the save slots hold. */
enum class EVestigeLoadOutcome : uint8
{
	/** No slot has a file: start a new game. */
	NewGame,

	/** Load the newest readable save. */
	Load,

	/** No slot could be read. Leave the files alone and turn saving off. */
	RefuseUnreadable,

	/** A slot holds a save from a newer version of the game. Leave the files alone and turn saving off. */
	RefuseNewer
};

/** What loading should do, and with which slot. */
struct FVestigeLoadDecision
{
	EVestigeLoadOutcome Outcome = EVestigeLoadOutcome::NewGame;

	/** With Load, the index of the slot to load. */
	int32 Slot = INDEX_NONE;
};

/**
 * Saves the farm every time a new day starts and loads it when play begins, following the rules in Architecture.md,
 * section 6. Each owner writes and restores its own part of the save; this subsystem collects, versions and writes them.
 * Saves alternate between two slots, so a failed write keeps the last good save. A save that can't be loaded is left
 * untouched, and saving stays off for the session, until the player starts a new game.
 * A world subsystem because everything it saves lives in the world, and a fresh one comes with every new world,
 * so starting a new game is just deleting the saves and opening the map again.
 */
UCLASS()
class UVestigeSaveSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * Decides what loading does with what each slot holds: refuse if any slot holds a newer version, load the readable save
	 * with the highest sequence number, refuse if files exist but none is readable, or start a new game.
	 * Static, so the rules are tested without touching the player's save files.
	 */
	static FVestigeLoadDecision DecideLoad(TConstArrayView<FVestigeSaveSlotRead> Slots);

	/** The slot the next save goes to: whichever doesn't hold the newest good save (NewestSlot, or INDEX_NONE for none). */
	static int32 GetSlotToWrite(int32 NewestSlot);

	/**
	 * Brings a save from an older version up to UVestigeSaveGame::CurrentVersion one step at a time. Returns false, leaving the
	 * save unusable, for a version it has no upgrade for. GridWidth is the width of the grid the save will be loaded into:
	 * version 0 stored tiles in grid order.
	 */
	static bool UpgradeToCurrentVersion(UVestigeSaveGame& Save, int32 GridWidth);

	/**
	 * Deletes both save slots and opens the map again, starting the farm from day 1. This is the player's explicit choice,
	 * so it also replaces a save that couldn't be loaded. Backups of upgraded saves are kept.
	 */
	void StartNewGame();

	/** False once a save couldn't be loaded, so no new day can overwrite it. */
	bool IsSavingAllowed() const { return bSavingAllowed; }

	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void Deinitialize() override;

private:
	/** Binds to the new day and loads the save. Bound to UWorld::OnWorldBeginPlay, which follows every actor's BeginPlay. */
	void BindAndLoad();

	void SaveFarm(int32 NewDay);
	void LoadFarm();

	/** Hands each owner its part of Save. Returns how many saved entries named content that no longer exists. */
	int32 ApplySave(const UVestigeSaveGame& Save, UFarmingComponent& Farming, UVestigeInventoryComponent& Inventory);

	/** Turns saving off for the rest of the session and tells the player why. */
	void RefuseToSave(const FText& Reason);

	/** Copies the save file in Slot, byte for byte, before an upgraded save can replace it. Done once per version. */
	void BackUpBeforeUpgrade(int32 Slot, int32 Version) const;

	UFarmingComponent* GetPlayerFarming() const;
	UVestigeInventoryComponent* GetPlayerInventory() const;

	/** The ability system on the player's player state, which holds the stamina. nullptr if there is none. */
	UAbilitySystemComponent* GetPlayerAbilitySystem() const;

	UPROPERTY()
	TObjectPtr<AVestigeDayCycle> DayCycle;

	UPROPERTY()
	TObjectPtr<AFarmGrid> Grid;

	FDelegateHandle WorldBeginPlayHandle;

	bool bSavingAllowed = true;

	/** The slot holding the newest good save, which the next save must not overwrite. INDEX_NONE before there is one. */
	int32 NewestSlot = INDEX_NONE;

	/** The newest good save's sequence number. The next save gets one more. */
	int32 NewestSequence = 0;
};
