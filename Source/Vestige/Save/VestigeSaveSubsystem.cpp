// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Save/VestigeSaveSubsystem.h"

#include "AbilitySystem/VestigeStaminaSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Content/FarmCropData.h"
#include "Content/VestigeContentData.h"
#include "Core/VestigeFeedbackSubsystem.h"
#include "Engine/World.h"
#include "Farming/FarmFieldSaveData.h"
#include "Farming/FarmFieldSubsystem.h"
#include "Farming/FarmingComponent.h"
#include "Farming/VestigeRestComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Inventory/VestigeInventoryComponent.h"
#include "Kismet/GameplayStatics.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "Save/VestigeSaveGame.h"
#include "Save/VestigeSaveLog.h"
#include "Time/VestigeClockSubsystem.h"

#define LOCTEXT_NAMESPACE "VestigeSaveSubsystem"

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

	/** Loads each crop the saved fields' tiles name, once, with its meshes. Crops that no longer exist are left out. */
	TMap<FName, const UFarmCropData*> LoadSavedCrops(const TArray<FFarmFieldSaveData>& SavedFields)
	{
		TMap<FName, const UFarmCropData*> Crops;
		TSet<FName> Missing;
		for (const FFarmFieldSaveData& Field : SavedFields)
		{
			for (const FFarmTileSaveData& Tile : Field.Tiles)
			{
				if (Tile.CropId.IsNone() || Crops.Contains(Tile.CropId) || Missing.Contains(Tile.CropId))
				{
					continue;
				}

				// Loading at play start, before the player can act, is when waiting for a load is allowed.
				const FPrimaryAssetId CropAssetId(UFarmCropData::AssetType, Tile.CropId);
				if (const UFarmCropData* Crop = Cast<UFarmCropData>(UVestigeContentData::LoadWithVisuals(CropAssetId)))
				{
					Crops.Add(Tile.CropId, Crop);
				}
				else
				{
					Missing.Add(Tile.CropId);
				}
			}
		}
		return Crops;
	}
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

bool UVestigeSaveSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UVestigeSaveSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	// This runs before the actors' BeginPlay. The world broadcasts OnWorldBeginPlay once all of them have run, so the
	// grid has its field and the farmer's toolbar is ready by then.
	WorldBeginPlayHandle = InWorld.OnWorldBeginPlay.AddUObject(this, &UVestigeSaveSubsystem::BindAndLoad);
}

void UVestigeSaveSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
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

	// The clock and the fields belong to the game instance, which survives opening the map.
	if (UVestigeClockSubsystem* GameClock = UVestigeClockSubsystem::Get(GetWorld()))
	{
		GameClock->StartNewGame();
	}
	if (UFarmFieldSubsystem* GameFields = UFarmFieldSubsystem::Get(GetWorld()))
	{
		GameFields->StartNewGame();
	}
	UGameplayStatics::OpenLevel(GetWorld(), FName(*UGameplayStatics::GetCurrentLevelName(GetWorld())));
}

void UVestigeSaveSubsystem::BindAndLoad()
{
	GetWorld()->OnWorldBeginPlay.Remove(WorldBeginPlayHandle);

	Clock = UVestigeClockSubsystem::Get(GetWorld());
	FarmFields = UFarmFieldSubsystem::Get(GetWorld());
	if (!Clock || !FarmFields)
	{
		VESTIGE_LOG(this, LogVestigeSave, Warning, "Saving is off in {Level}: it has no game instance with a clock and farm fields.",
			GetWorld()->GetMapName());
		return;
	}

	Clock->OnNewDayFinished.AddUObject(this, &UVestigeSaveSubsystem::SaveFarm);
	LoadFarm();
}

void UVestigeSaveSubsystem::SaveFarm(int32 NewDay)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UVestigeSaveSubsystem::SaveFarm);

	const UFarmingComponent* Farming = GetPlayerFarming();
	const UVestigeInventoryComponent* Inventory = GetPlayerInventory();
	if (!Farming || !Inventory)
	{
		return;
	}
	if (!bSavingAllowed)
	{
		VESTIGE_LOG(this, LogVestigeSave, Warning, "Not saving day {Day}: the save couldn't be loaded, so it is kept as it is.", NewDay);
		return;
	}

	UVestigeSaveGame* Save = Cast<UVestigeSaveGame>(UGameplayStatics::CreateSaveGameObject(UVestigeSaveGame::StaticClass()));
	Save->SaveVersion = UVestigeSaveGame::CurrentVersion;
	Save->SaveSequence = NewestSequence + 1;
	Save->Clock = Clock->GetSaveData();
	Save->Farming = Farming->GetSaveData();
	Save->Inventory = Inventory->GetSaveData();
	if (const UAbilitySystemComponent* AbilitySystem = GetPlayerAbilitySystem())
	{
		Save->Stamina = UVestigeStaminaSet::GetSaveData(*AbilitySystem);
	}
	if (const UVestigeRestComponent* Rest = GetPlayerRest())
	{
		Save->Rest = Rest->GetSaveData();
	}
	Save->Fields = FarmFields->GetSaveData();

	const int32 Slot = GetSlotToWrite(NewestSlot);
	if (!UGameplayStatics::SaveGameToSlot(Save, UVestigeSaveGame::SlotNames[Slot], UVestigeSaveGame::UserIndex))
	{
		VESTIGE_LOG(this, LogVestigeSave, Error, "Couldn't write the save to slot {Slot}. The previous save is kept.",
			UVestigeSaveGame::SlotNames[Slot]);
		UVestigeFeedbackSubsystem::Notify(GetWorld(), LOCTEXT("SaveFailed", "Couldn't save the game. The previous save is kept."));
		return;
	}

	NewestSlot = Slot;
	NewestSequence = Save->SaveSequence;
	UVestigeFeedbackSubsystem::Notify(GetWorld(), LOCTEXT("Saved", "Game saved."));
}

void UVestigeSaveSubsystem::LoadFarm()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UVestigeSaveSubsystem::LoadFarm);

	UFarmingComponent* Farming = GetPlayerFarming();
	UVestigeInventoryComponent* Inventory = GetPlayerInventory();
	if (!Farming || !Inventory)
	{
		VESTIGE_LOG(this, LogVestigeSave, Warning, "Not loading the save: the player has no farming component or no inventory.");
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
		RefuseToSave(LOCTEXT(
			"SaveUnreadable", "The save couldn't be read. It is kept as it is, and the game won't save until you start a new game."));
		return;

	case EVestigeLoadOutcome::RefuseNewer:
		VESTIGE_LOG(this, LogVestigeSave, Error,
			"Slot {Slot} holds a save from a newer version ({Version}, this build writes {CurrentVersion}). Saving is off.",
			UVestigeSaveGame::SlotNames[Decision.Slot], Slots[Decision.Slot].Save->SaveVersion, UVestigeSaveGame::CurrentVersion);
		RefuseToSave(LOCTEXT("SaveTooNew",
			"The save is from a newer version of the game. It is kept as it is, and the game won't save until you start a new game."));
		return;

	case EVestigeLoadOutcome::Load:
		break;
	}

	const UVestigeSaveGame& Save = *Slots[Decision.Slot].Save;
	VESTIGE_LOG(this, LogVestigeSave, Log, "Loading day {Day} from save slot {Slot} (version {Version}, save {Sequence}).", Save.Clock.Day,
		UVestigeSaveGame::SlotNames[Decision.Slot], Save.SaveVersion, Save.SaveSequence);
	NewestSlot = Decision.Slot;
	NewestSequence = Save.SaveSequence;
	if (ApplySave(Save, *Farming, *Inventory) > 0)
	{
		UVestigeFeedbackSubsystem::Notify(
			GetWorld(), LOCTEXT("ContentMissing", "Some saved things no longer exist in the game and were left out."));
	}
	UVestigeFeedbackSubsystem::Notify(
		GetWorld(), FText::FormatNamed(LOCTEXT("Loaded", "Welcome back. Day {Day}."), TEXT("Day"), Save.Clock.Day));
}

int32 UVestigeSaveSubsystem::ApplySave(const UVestigeSaveGame& Save, UFarmingComponent& Farming, UVestigeInventoryComponent& Inventory)
{
	int32 SkippedCount = 0;
	Clock->RestoreFromSaveData(Save.Clock);
	if (!Farming.RestoreFromSaveData(Save.Farming))
	{
		++SkippedCount;
	}
	SkippedCount += Inventory.RestoreFromSaveData(Save.Inventory);
	if (UAbilitySystemComponent* AbilitySystem = GetPlayerAbilitySystem())
	{
		UVestigeStaminaSet::RestoreFromSaveData(*AbilitySystem, Save.Stamina);
	}
	if (UVestigeRestComponent* Rest = GetPlayerRest())
	{
		Rest->RestoreFromSaveData(Save.Rest);
	}
	SkippedCount += FarmFields->RestoreFromSaveData(Save.Fields, VestigeSaveSlots::LoadSavedCrops(Save.Fields));
	return SkippedCount;
}

void UVestigeSaveSubsystem::RefuseToSave(const FText& Reason)
{
	bSavingAllowed = false;
	UVestigeFeedbackSubsystem::Notify(GetWorld(), Reason);
}

UFarmingComponent* UVestigeSaveSubsystem::GetPlayerFarming() const
{
	const APawn* Pawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	return Pawn ? Pawn->FindComponentByClass<UFarmingComponent>() : nullptr;
}

UVestigeInventoryComponent* UVestigeSaveSubsystem::GetPlayerInventory() const
{
	const APlayerState* PlayerState = UGameplayStatics::GetPlayerState(GetWorld(), 0);
	return PlayerState ? PlayerState->FindComponentByClass<UVestigeInventoryComponent>() : nullptr;
}

UVestigeRestComponent* UVestigeSaveSubsystem::GetPlayerRest() const
{
	const APawn* Pawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	return Pawn ? Pawn->FindComponentByClass<UVestigeRestComponent>() : nullptr;
}

UAbilitySystemComponent* UVestigeSaveSubsystem::GetPlayerAbilitySystem() const
{
	return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(UGameplayStatics::GetPlayerState(GetWorld(), 0));
}

#undef LOCTEXT_NAMESPACE
