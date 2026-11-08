// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Save/VestigeSaveSubsystem.h"

#include "AbilitySystem/VestigeGameplayTags.h"
#include "Core/VestigeFeedbackSubsystem.h"
#include "Core/VestigeSaveLog.h"
#include "Core/VestigeSaveRegistry.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "Save/VestigeSaveGame.h"
#include "Time/VestigeClockSubsystem.h"

namespace VestigeSaveSlots
{
	FVestigeSaveSlotRead ReadSlot(const TCHAR* SlotName)
	{
		FVestigeSaveSlotRead Read;
		Read.bExists = UGameplayStatics::DoesSaveGameExist(SlotName, UVestigeSaveGame::UserIndex);
		if (Read.bExists)
		{
			Read.Save = Cast<UVestigeSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, UVestigeSaveGame::UserIndex));
		}
		return Read;
	}
}

UVestigeSaveSubsystem* UVestigeSaveSubsystem::Get(const UWorld* World)
{
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UVestigeSaveSubsystem>() : nullptr;
}

FVestigeLoadDecision UVestigeSaveSubsystem::DecideLoad(TConstArrayView<FVestigeSaveSlotRead> Slots)
{
	FVestigeLoadDecision Decision;
	bool bAnyFile = false;
	for (int32 Slot = 0; Slot < Slots.Num(); ++Slot)
	{
		const FVestigeSaveSlotRead& Read = Slots[Slot];
		bAnyFile |= Read.bExists;
		if (!Read.Save)
		{
			continue;
		}

		// Loading an older slot and then writing over the newer one would lose the newer game's progress.
		if (Read.Save->SaveVersion > UVestigeSaveGame::CurrentVersion)
		{
			return {EVestigeLoadOutcome::RefuseNewer, Slot};
		}
		// There are no upgrades before the first public build, so an older save is as unreadable as a corrupt one.
		if (Read.Save->SaveVersion < UVestigeSaveGame::CurrentVersion)
		{
			continue;
		}
		if (Decision.Slot == INDEX_NONE || Read.Save->SaveSequence > Slots[Decision.Slot].Save->SaveSequence)
		{
			Decision = {EVestigeLoadOutcome::Load, Slot};
		}
	}

	if (Decision.Slot == INDEX_NONE && bAnyFile)
	{
		Decision.Outcome = EVestigeLoadOutcome::RefuseUnreadable;
	}
	return Decision;
}

int32 UVestigeSaveSubsystem::GetSlotToWrite(int32 NewestSlot)
{
	return NewestSlot == 0 ? 1 : 0;
}

void UVestigeSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Registry = Collection.InitializeDependency<UVestigeSaveRegistry>();
	Clock = Collection.InitializeDependency<UVestigeClockSubsystem>();
	if (Clock)
	{
		Clock->OnNewDayFinished.AddUObject(this, &UVestigeSaveSubsystem::SaveGame);
	}

	// The game's first map is initialised after the game instance, and before any of its actors begins play. A test's
	// game instance is made for a map that already exists, so it never loads the player's save. Once there is a main
	// menu, the menu's Continue loads instead (Systems.md, section 1).
	WorldInitializedHandle = FWorldDelegates::OnPostWorldInitialization.AddUObject(this, &UVestigeSaveSubsystem::LoadOnFirstMap);
}

void UVestigeSaveSubsystem::Deinitialize()
{
	FWorldDelegates::OnPostWorldInitialization.Remove(WorldInitializedHandle);
	if (UWorld* World = LoadedWorld.Get())
	{
		World->OnWorldBeginPlay.Remove(WorldBeginPlayHandle);
	}
	if (Clock)
	{
		Clock->OnNewDayFinished.RemoveAll(this);
	}
	Super::Deinitialize();
}

void UVestigeSaveSubsystem::StartNewGame()
{
	for (const TCHAR* SlotName : UVestigeSaveGame::SlotNames)
	{
		UGameplayStatics::DeleteGameInSlot(SlotName, UVestigeSaveGame::UserIndex);
	}

	if (Registry)
	{
		Registry->StartNewGame();
	}
	bGameStarted = true;
	bSavingAllowed = true;
	NewestSlot = INDEX_NONE;
	NewestSequence = 0;

	const UWorld* World = GetGameInstance()->GetWorld();
	UGameplayStatics::OpenLevel(World, FName(*UGameplayStatics::GetCurrentLevelName(World)));
}

void UVestigeSaveSubsystem::LoadOnFirstMap(UWorld* World, const UWorld::InitializationValues InitializationValues)
{
	if (bGameStarted || !World || !World->IsGameWorld() || World->GetGameInstance() != GetGameInstance())
	{
		return;
	}

	// A test that plays a map gets a real game instance; it must neither read nor overwrite the player's save. Without a
	// load the game never starts here, so nothing is saved either.
	if (GIsAutomationTesting)
	{
		VESTIGE_LOG(this, LogVestigeSave, Log, "Automation tests are running, so the save isn't loaded and nothing is saved.");
		return;
	}

	FWorldDelegates::OnPostWorldInitialization.Remove(WorldInitializedHandle);
	LoadGame();
	LoadedWorld = World;
	WorldBeginPlayHandle = World->OnWorldBeginPlay.AddUObject(this, &UVestigeSaveSubsystem::TellPlayerAboutLoad);
}

void UVestigeSaveSubsystem::LoadGame()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UVestigeSaveSubsystem::LoadGame);

	bGameStarted = true;
	if (!Registry || !Clock)
	{
		VESTIGE_LOG(this, LogVestigeSave, Warning, "Saving is off: the game instance has no save registry or no clock.");
		bSavingAllowed = false;
		return;
	}

	TArray<FVestigeSaveSlotRead, TInlineAllocator<UE_ARRAY_COUNT(UVestigeSaveGame::SlotNames)>> Slots;
	for (const TCHAR* SlotName : UVestigeSaveGame::SlotNames)
	{
		Slots.Add(VestigeSaveSlots::ReadSlot(SlotName));
	}

	for (int32 Slot = 0; Slot < Slots.Num(); ++Slot)
	{
		if (Slots[Slot].bExists && !Slots[Slot].Save)
		{
			VESTIGE_LOG(
				this, LogVestigeSave, Warning, "Save slot {Slot} couldn't be read as a farm save.", UVestigeSaveGame::SlotNames[Slot]);
		}
		else if (Slots[Slot].Save && Slots[Slot].Save->SaveVersion < UVestigeSaveGame::CurrentVersion)
		{
			VESTIGE_LOG(this, LogVestigeSave, Warning,
				"Save slot {Slot} holds an older save, version {Version}. This build only loads version {CurrentVersion}.",
				UVestigeSaveGame::SlotNames[Slot], Slots[Slot].Save->SaveVersion, UVestigeSaveGame::CurrentVersion);
		}
	}

	const FVestigeLoadDecision Decision = DecideLoad(Slots);
	switch (Decision.Outcome)
	{
	case EVestigeLoadOutcome::NewGame:
		return;

	case EVestigeLoadOutcome::RefuseUnreadable:
		VESTIGE_LOG(this, LogVestigeSave, Error, "No save slot could be read. The files are left as they are and saving is off.");
		RefuseToSave(VestigeGameplayTags::Feedback_Save_Unreadable);
		return;

	case EVestigeLoadOutcome::RefuseNewer:
		VESTIGE_LOG(this, LogVestigeSave, Error,
			"Slot {Slot} holds a save from a newer version ({Version}, this build writes {CurrentVersion}). Saving is off.",
			UVestigeSaveGame::SlotNames[Decision.Slot], Slots[Decision.Slot].Save->SaveVersion, UVestigeSaveGame::CurrentVersion);
		RefuseToSave(VestigeGameplayTags::Feedback_Save_TooNew);
		return;

	case EVestigeLoadOutcome::Load:
		break;
	}

	const UVestigeSaveGame& Save = *Slots[Decision.Slot].Save;
	NewestSlot = Decision.Slot;
	NewestSequence = Save.SaveSequence;
	Registry->Load(Save.Records);
	VESTIGE_LOG(this, LogVestigeSave, Log, "Loaded day {Day} from save slot {Slot} (version {Version}, save {Sequence}, {Count} parts).",
		Clock->GetCurrentDay(), UVestigeSaveGame::SlotNames[Decision.Slot], Save.SaveVersion, Save.SaveSequence, Save.Records.Num());
	PendingReports.Add({VestigeGameplayTags::Feedback_Save_Loaded, FFormatNamedArguments{{TEXT("Day"), Clock->GetCurrentDay()}}});
}

void UVestigeSaveSubsystem::TellPlayerAboutLoad()
{
	if (UWorld* World = LoadedWorld.Get())
	{
		World->OnWorldBeginPlay.Remove(WorldBeginPlayHandle);
	}
	LoadedWorld.Reset();

	// The map's owners have read their parts in their BeginPlay by now, so the registry knows what was left out.
	if (Registry && Registry->GetLeftOutCount() > 0)
	{
		ReportToPlayer(VestigeGameplayTags::Feedback_Save_ContentMissing);
	}
	if (Registry && Registry->HasNewerParts() && bSavingAllowed)
	{
		RefuseToSave(VestigeGameplayTags::Feedback_Save_PartTooNew);
	}
	for (FVestigeFeedbackReport& Pending : PendingReports)
	{
		ReportToPlayer(Pending.Tag, MoveTemp(Pending.Arguments));
	}
	PendingReports.Reset();
}

void UVestigeSaveSubsystem::SaveGame(int32 NewDay)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UVestigeSaveSubsystem::SaveGame);

	// A game instance whose first map was never initialised (a test's) has no save to keep, and must not touch the files.
	if (!bGameStarted || !Registry)
	{
		return;
	}
	if (!bSavingAllowed || Registry->HasNewerParts())
	{
		VESTIGE_LOG(this, LogVestigeSave, Warning, "Not saving day {Day}: the save couldn't be loaded, so it is kept as it is.", NewDay);
		return;
	}

	UVestigeSaveGame* Save = Cast<UVestigeSaveGame>(UGameplayStatics::CreateSaveGameObject(UVestigeSaveGame::StaticClass()));
	Save->SaveVersion = UVestigeSaveGame::CurrentVersion;
	Save->SaveSequence = NewestSequence + 1;
	Save->Records = Registry->CollectRecords();

	const int32 Slot = GetSlotToWrite(NewestSlot);
	if (!UGameplayStatics::SaveGameToSlot(Save, UVestigeSaveGame::SlotNames[Slot], UVestigeSaveGame::UserIndex))
	{
		VESTIGE_LOG(this, LogVestigeSave, Error, "Couldn't write the save to slot {Slot}. The previous save is kept.",
			UVestigeSaveGame::SlotNames[Slot]);
		ReportToPlayer(VestigeGameplayTags::Feedback_Save_Failed);
		return;
	}

	NewestSlot = Slot;
	NewestSequence = Save->SaveSequence;
	ReportToPlayer(VestigeGameplayTags::Feedback_Save_Saved);
}

void UVestigeSaveSubsystem::RefuseToSave(const FGameplayTag& Reason)
{
	bSavingAllowed = false;
	PendingReports.Add({Reason, {}});
}

void UVestigeSaveSubsystem::ReportToPlayer(const FGameplayTag& Tag, FFormatNamedArguments Arguments) const
{
	UVestigeFeedbackSubsystem::Report(GetGameInstance()->GetWorld(), Tag, MoveTemp(Arguments));
}
