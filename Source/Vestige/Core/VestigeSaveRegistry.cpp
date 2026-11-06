// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Core/VestigeSaveRegistry.h"

#include "Core/VestigeSaveLog.h"
#include "Core/VestigeSaveable.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

UVestigeSaveRegistry* UVestigeSaveRegistry::Get(const UWorld* World)
{
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UVestigeSaveRegistry>() : nullptr;
}

void UVestigeSaveRegistry::RegisterWorldOwner(const UWorld* World, IVestigeSaveable& Saveable)
{
	if (UVestigeSaveRegistry* Registry = Get(World))
	{
		Registry->Register(Saveable, ELifetime::World);
		return;
	}

	// Without a game instance nothing is saved, so the owner is in a game that just started.
	Saveable.StartNewGame();
}

void UVestigeSaveRegistry::UnregisterWorldOwner(const UWorld* World, IVestigeSaveable& Saveable)
{
	if (UVestigeSaveRegistry* Registry = Get(World))
	{
		Registry->KeepPartAndForget(Saveable);
	}
}

void UVestigeSaveRegistry::RegisterGameOwner(IVestigeSaveable& Saveable)
{
	Register(Saveable, ELifetime::Game);
}

void UVestigeSaveRegistry::UnregisterGameOwner(IVestigeSaveable& Saveable)
{
	Forget(Saveable);
}

void UVestigeSaveRegistry::Load(TMap<FName, FVestigeSaveRecord> InRecords)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UVestigeSaveRegistry::Load);

	Records = MoveTemp(InRecords);
	LeftOutCount = 0;
	bHasNewerParts = false;

	// A copy, so an owner that registers or leaves while restoring can't change the list being walked.
	const TArray<FRegistration> Current = Registrations;
	for (const FRegistration& Registration : Current)
	{
		if (IVestigeSaveable* Saveable = Registration.Saveable.Get())
		{
			Restore(*Saveable);
		}
	}
}

const TMap<FName, FVestigeSaveRecord>& UVestigeSaveRegistry::CollectRecords()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UVestigeSaveRegistry::CollectRecords);

	for (const FRegistration& Registration : Registrations)
	{
		if (const IVestigeSaveable* Saveable = Registration.Saveable.Get())
		{
			Records.Add(Saveable->GetSaveKey(), Saveable->WriteSaveRecord());
		}
	}
	return Records;
}

void UVestigeSaveRegistry::StartNewGame()
{
	Records.Reset();
	LeftOutCount = 0;
	bHasNewerParts = false;

	// The open map's owners are destroyed when it opens again, and what they hold belongs to the old game.
	Registrations.RemoveAll(
		[](const FRegistration& Registration)
		{
			return Registration.Lifetime == ELifetime::World;
		});

	const TArray<FRegistration> GameOwners = Registrations;
	for (const FRegistration& Registration : GameOwners)
	{
		if (IVestigeSaveable* Saveable = Registration.Saveable.Get())
		{
			Saveable->StartNewGame();
		}
	}
}

void UVestigeSaveRegistry::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	WorldTearDownHandle = FWorldDelegates::OnWorldBeginTearDown.AddUObject(this, &UVestigeSaveRegistry::KeepWorldOwnersParts);
}

void UVestigeSaveRegistry::Deinitialize()
{
	FWorldDelegates::OnWorldBeginTearDown.Remove(WorldTearDownHandle);
	Registrations.Reset();
	Super::Deinitialize();
}

void UVestigeSaveRegistry::KeepWorldOwnersParts(UWorld* World)
{
	if (!World || World->GetGameInstance() != GetGameInstance())
	{
		return;
	}

	TRACE_CPUPROFILER_EVENT_SCOPE(UVestigeSaveRegistry::KeepWorldOwnersParts);

	// Now, while the pawn is still possessed: the engine unpossesses it before any actor's EndPlay runs.
	for (const FRegistration& Registration : Registrations)
	{
		const IVestigeSaveable* Saveable = Registration.Saveable.Get();
		if (Saveable && Registration.Lifetime == ELifetime::World)
		{
			Records.Add(Saveable->GetSaveKey(), Saveable->WriteSaveRecord());
		}
	}
	Registrations.RemoveAll(
		[](const FRegistration& Registration)
		{
			return Registration.Lifetime == ELifetime::World;
		});
}

void UVestigeSaveRegistry::Register(IVestigeSaveable& Saveable, ELifetime Lifetime)
{
	const FName Key = Saveable.GetSaveKey();
	if (IsKeyRegistered(Key))
	{
		VESTIGE_LOG(this, LogVestigeSave, Warning, "Two owners save under the key {Key}, so only the first one is saved.", Key);
		Saveable.StartNewGame();
		return;
	}

	Registrations.Add({&Saveable, Lifetime});
	Restore(Saveable);
}

void UVestigeSaveRegistry::KeepPartAndForget(IVestigeSaveable& Saveable)
{
	if (FindRegistration(Saveable) == INDEX_NONE)
	{
		return;
	}
	Records.Add(Saveable.GetSaveKey(), Saveable.WriteSaveRecord());
	Forget(Saveable);
}

void UVestigeSaveRegistry::Forget(const IVestigeSaveable& Saveable)
{
	const int32 Index = FindRegistration(Saveable);
	if (Index != INDEX_NONE)
	{
		Registrations.RemoveAt(Index);
	}
}

void UVestigeSaveRegistry::Restore(IVestigeSaveable& Saveable)
{
	const FName Key = Saveable.GetSaveKey();
	const FVestigeSaveRecord* Record = Records.Find(Key);
	if (!Record)
	{
		Saveable.StartNewGame();
		return;
	}

	const int32 OwnerVersion = Saveable.GetSaveVersion();
	if (Record->GetVersion() > OwnerVersion || Record->IsFromNewerEngine())
	{
		VESTIGE_LOG(this, LogVestigeSave, Error,
			"The saved {Key} (version {Version}) is from a newer build than this one (version {OwnerVersion}), so it wasn't read.", Key,
			Record->GetVersion(), OwnerVersion);
		bHasNewerParts = true;
		Saveable.StartNewGame();
		return;
	}

	// There are no upgrades before the first public build, so an older part is left out like content that no longer exists.
	if (Record->GetVersion() < OwnerVersion)
	{
		VESTIGE_LOG(this, LogVestigeSave, Warning,
			"The saved {Key} is version {Version}, older than this build's {OwnerVersion}, so it was left out.", Key, Record->GetVersion(),
			OwnerVersion);
		++LeftOutCount;
		Saveable.StartNewGame();
		return;
	}

	if (!Saveable.ReadSaveRecord(*Record, LeftOutCount))
	{
		VESTIGE_LOG(this, LogVestigeSave, Warning, "The saved {Key} couldn't be read, so it was left out.", Key);
		++LeftOutCount;
		Saveable.StartNewGame();
	}
}

int32 UVestigeSaveRegistry::FindRegistration(const IVestigeSaveable& Saveable) const
{
	return Registrations.IndexOfByPredicate(
		[&Saveable](const FRegistration& Registration)
		{
			return Registration.Saveable.Get() == &Saveable;
		});
}

bool UVestigeSaveRegistry::IsKeyRegistered(FName Key) const
{
	return Registrations.ContainsByPredicate(
		[Key](const FRegistration& Registration)
		{
			const IVestigeSaveable* Saveable = Registration.Saveable.Get();
			return Saveable && Saveable->GetSaveKey() == Key;
		});
}
