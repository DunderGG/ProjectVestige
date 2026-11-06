// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "VestigeSaveSubsystem.generated.h"

class UVestigeClockSubsystem;
class UVestigeSaveGame;
class UVestigeSaveRegistry;

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

	/** No slot could be read, or each holds a save from an older version. Leave the files alone and turn saving off. */
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
 * The save files, following the rules in Architecture.md, section 6 and Systems.md, section 1. Reads the save once per
 * game, when the game's first map is initialised and before any of its actors begins play, and hands its records to
 * UVestigeSaveRegistry, which gives each owner its part; tells the player how loading went once that map has begun play.
 * Writes every record the registry holds each time a new day starts. Saves alternate between two slots, so a failed write
 * keeps the last good save. A save that can't be loaded is left untouched, and saving stays off until the player starts a
 * new game. Knows no owner of saved state.
 * A game-instance subsystem, so the save is read once and travelling to another map doesn't load it again.
 */
UCLASS()
class UVestigeSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** The save subsystem of World's game instance, or nullptr in a world without one (editor and preview worlds). */
	static UVestigeSaveSubsystem* Get(const UWorld* World);

	/**
	 * Decides what loading does with what each slot holds: refuse if any slot holds a newer version, load the readable save of
	 * the current version with the highest sequence number, refuse if files exist but none of them is that, or start a new game.
	 * An older version counts as unreadable: there are no upgrades before the first public build.
	 * Static, so the rules are tested without touching the player's save files.
	 */
	static FVestigeLoadDecision DecideLoad(TConstArrayView<FVestigeSaveSlotRead> Slots);

	/** The slot the next save goes to: whichever doesn't hold the newest good save (NewestSlot, or INDEX_NONE for none). */
	static int32 GetSlotToWrite(int32 NewestSlot);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * Deletes both save slots, starts a new game in every saved owner and opens the map again. This is the player's
	 * explicit choice, so it also replaces a save that couldn't be loaded, and saving is allowed again.
	 */
	void StartNewGame();

	/** False once a save couldn't be loaded, so no new day can overwrite it. */
	bool IsSavingAllowed() const { return bSavingAllowed; }

private:
	/** Loads the save when the game's first map is initialised. Bound to FWorldDelegates::OnPostWorldInitialization. */
	void LoadOnFirstMap(UWorld* World, const UWorld::InitializationValues InitializationValues);

	/** Reads the slots and hands the newest good save to the registry, or turns saving off if it can't be loaded. */
	void LoadGame();

	/** Tells the player how loading went. Bound to the first map's OnWorldBeginPlay, once the HUD listens. */
	void TellPlayerAboutLoad();

	/** Writes every record to the slot that doesn't hold the newest good save. Bound to the clock's OnNewDayFinished. */
	void SaveGame(int32 NewDay);

	/** Turns saving off for the rest of the session and keeps Reason to tell the player. */
	void RefuseToSave(const FText& Reason);

	/** Raises Message for the player in the open map. */
	void Notify(const FText& Message) const;

	UPROPERTY()
	TObjectPtr<UVestigeSaveRegistry> Registry;

	UPROPERTY()
	TObjectPtr<UVestigeClockSubsystem> Clock;

	/** The first map, until the player has been told how loading went. */
	TWeakObjectPtr<UWorld> LoadedWorld;

	FDelegateHandle WorldInitializedHandle;
	FDelegateHandle WorldBeginPlayHandle;

	/** What the player is told once the first map has begun play. */
	TArray<FText> MessagesForPlayer;

	/** True once the save has been loaded, or a new game started. Nothing is saved before. */
	bool bGameStarted = false;

	bool bSavingAllowed = true;

	/** The slot holding the newest good save, which the next save must not overwrite. INDEX_NONE before there is one. */
	int32 NewestSlot = INDEX_NONE;

	/** The newest good save's sequence number. The next save gets one more. */
	int32 NewestSequence = 0;
};
