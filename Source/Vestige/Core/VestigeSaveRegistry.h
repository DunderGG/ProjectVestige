// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Core/VestigeSaveRecord.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UObject/WeakInterfacePtr.h"
#include "VestigeSaveRegistry.generated.h"

class IVestigeSaveable;

/**
 * The game's saved state in memory, one record per key, and the owners it belongs to (Systems.md, section 1). Owners
 * register with it and get their part at once: game-instance owners (the clock, the fields) for the whole game, world
 * owners (the player's components) while their map is open. When the map starts tearing down, before any actor is
 * destroyed, every world owner writes its part and is forgotten, so the player's data crosses a map change; a world owner
 * destroyed while its map stays open writes its part as it leaves. Records nobody has registered for are kept unchanged,
 * so a mod's data survives a game without the mod. Knows no system and no file: UVestigeSaveSubsystem reads and writes
 * the files.
 */
UCLASS()
class UVestigeSaveRegistry : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** The registry of World's game instance, or nullptr in a world without one (editor and preview worlds). */
	static UVestigeSaveRegistry* Get(const UWorld* World);

	/**
	 * For an owner in a map, from its BeginPlay, once it can restore: registers it in World's game instance and hands it
	 * its part, or has it start as in a new game when there is no part or no game instance.
	 */
	static void RegisterWorldOwner(const UWorld* World, IVestigeSaveable& Saveable);

	/**
	 * For an owner in a map, at the start of its EndPlay: keeps its part in the snapshot and forgets it. An owner whose
	 * map is closing was already asked for its part when the map started tearing down, and isn't asked again: by now the
	 * pawn has been unpossessed and may no longer reach the player state.
	 */
	static void UnregisterWorldOwner(const UWorld* World, IVestigeSaveable& Saveable);

	/** For a game-instance subsystem, from its Initialize: registers it for the whole game and hands it its part. */
	void RegisterGameOwner(IVestigeSaveable& Saveable);

	/** For a game-instance subsystem, from its Deinitialize. */
	void UnregisterGameOwner(IVestigeSaveable& Saveable);

	/** Replaces the snapshot with records read from a save, and hands every registered owner its part. */
	void Load(TMap<FName, FVestigeSaveRecord> InRecords);

	/** Every record, for writing a save: each registered owner writes its part now; the others stay as they were read. */
	const TMap<FName, FVestigeSaveRecord>& CollectRecords();

	/**
	 * Empties the snapshot for a new game: every game-instance owner starts a new game, and the open map's owners are
	 * forgotten without writing their part, since the map is opened again for the new game.
	 */
	void StartNewGame();

	/** How many saved entries were left out since the last Load or StartNewGame: content that no longer exists, outdated or unreadable parts. */
	int32 GetLeftOutCount() const { return LeftOutCount; }

	/**
	 * True once a part from a newer version of the game, or written by a newer engine, was found. It wasn't read, so
	 * saving over it would lose it.
	 */
	bool HasNewerParts() const { return bHasNewerParts; }

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

#if WITH_DEV_AUTOMATION_TESTS
	/** For tests only: what happens when World starts tearing down, without tearing it down. */
	void KeepWorldOwnersPartsForTests(UWorld& World) { KeepWorldOwnersParts(&World); }
#endif

private:
	/** Whether an owner lives as long as the game instance or as long as its map. */
	enum class ELifetime : uint8
	{
		Game,
		World
	};

	struct FRegistration
	{
		TWeakInterfacePtr<IVestigeSaveable> Saveable;
		ELifetime Lifetime = ELifetime::Game;
	};

	void Register(IVestigeSaveable& Saveable, ELifetime Lifetime);

	/** Writes a registered world owner's part into the snapshot, then forgets it. Does nothing for an owner it forgot. */
	void KeepPartAndForget(IVestigeSaveable& Saveable);

	/**
	 * Writes every world owner's part and forgets them, when their map starts tearing down and before any of its actors
	 * is destroyed. Bound to FWorldDelegates::OnWorldBeginTearDown.
	 */
	void KeepWorldOwnersParts(UWorld* World);

	void Forget(const IVestigeSaveable& Saveable);

	/** Hands Saveable its part, or has it start as in a new game when there is none it can read. */
	void Restore(IVestigeSaveable& Saveable);

	int32 FindRegistration(const IVestigeSaveable& Saveable) const;
	bool IsKeyRegistered(FName Key) const;

	UPROPERTY()
	TMap<FName, FVestigeSaveRecord> Records;

	TArray<FRegistration> Registrations;

	FDelegateHandle WorldTearDownHandle;

	int32 LeftOutCount = 0;

	bool bHasNewerParts = false;
};
