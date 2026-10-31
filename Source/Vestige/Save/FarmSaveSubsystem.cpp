// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Save/FarmSaveSubsystem.h"

#include "Content/FarmContentData.h"
#include "Content/FarmCropData.h"
#include "Core/FarmFeedbackSubsystem.h"
#include "Core/Vestige.h"
#include "Engine/World.h"
#include "Farming/FarmGrid.h"
#include "Farming/FarmingComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Inventory/VestigeInventoryComponent.h"
#include "Kismet/GameplayStatics.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "Save/FarmSaveGame.h"
#include "Time/FarmDayCycle.h"

#define LOCTEXT_NAMESPACE "FarmSaveSubsystem"

namespace FarmSaveUpgrade
{
	/** Version 0 saves could only plant the seed crop, and the seed crop was always the turnip. */
	const FName Version0CropId(TEXT("Base.Turnip"));

	/** The ContentId of the tool in hand in a version 0 save, which may only have the old tool enum. */
	FName GetVersion0ToolId(const UFarmSaveGame& Save)
	{
		if (!Save.CurrentToolId.IsNone())
		{
			return Save.CurrentToolId;
		}

		switch (Save.CurrentTool)
		{
		case EFarmTool::Hoe:
			return TEXT("Base.Hoe");
		case EFarmTool::Seeds:
			return TEXT("Base.Seeds");
		case EFarmTool::WateringCan:
			return TEXT("Base.WateringCan");
		case EFarmTool::Hand:
			return TEXT("Base.Hand");
		}
		return NAME_None;
	}

	/** Moves the flat version 0 fields into the owners' structs and gives tiles their coordinates and crop. */
	bool UpgradeFromVersion0(UFarmSaveGame& Save, int32 GridWidth)
	{
		if (GridWidth <= 0)
		{
			return false;
		}

		Save.Clock.Day = Save.Day;
		Save.Clock.TimeOfDay = Save.TimeOfDay;
		Save.Farming.Seeds = Save.Seeds;
		Save.Farming.HarvestedCrops = Save.HarvestedCrops;
		Save.Farming.CurrentToolId = GetVersion0ToolId(Save);

		// Version 0 wrote every tile in grid order, row by row, into a grid of the same width.
		for (int32 Index = 0; Index < Save.Tiles.Num(); ++Index)
		{
			FFarmTileSaveData& Tile = Save.Tiles[Index];
			Tile.Coordinates = FIntPoint(Index % GridWidth, Index / GridWidth);
			Tile.CropId = Tile.State == EFarmTileState::Planted ? Version0CropId : NAME_None;
		}
		Save.SaveVersion = 1;
		return true;
	}

	/** Version 1 counted seeds and harvested crops, and the turnip was the only seed and crop. */
	const FName Version1SeedItemId(TEXT("Base.TurnipSeeds"));
	const FName Version1HarvestItemId(TEXT("Base.Turnip"));

	/** Turns the seed and harvest counts into stacks in the first two inventory slots. */
	void UpgradeFromVersion1(UFarmSaveGame& Save)
	{
		const TPair<FName, int32> Stacks[] = {
			{Version1SeedItemId, Save.Farming.Seeds}, {Version1HarvestItemId, Save.Farming.HarvestedCrops}};
		for (const TPair<FName, int32>& Stack : Stacks)
		{
			if (Stack.Value > 0)
			{
				Save.Inventory.Slots.Add({Save.Inventory.Slots.Num(), Stack.Key, Stack.Value});
			}
		}
		Save.SaveVersion = 2;
	}
}

namespace FarmSaveSlots
{
	FFarmSaveSlotRead ReadSlot(const TCHAR* SlotName)
	{
		FFarmSaveSlotRead Read;
		Read.bExists = UGameplayStatics::DoesSaveGameExist(SlotName, UFarmSaveGame::UserIndex);
		if (Read.bExists)
		{
			Read.Save = Cast<UFarmSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, UFarmSaveGame::UserIndex));
		}
		return Read;
	}

	/** Loads each crop the saved tiles name, once, with its meshes. Crops that no longer exist are left out. */
	TMap<FName, const UFarmCropData*> LoadSavedCrops(const TArray<FFarmTileSaveData>& SavedTiles)
	{
		TMap<FName, const UFarmCropData*> Crops;
		TSet<FName> Missing;
		for (const FFarmTileSaveData& Tile : SavedTiles)
		{
			if (Tile.CropId.IsNone() || Crops.Contains(Tile.CropId) || Missing.Contains(Tile.CropId))
			{
				continue;
			}

			// Loading at play start, before the player can act, is when waiting for a load is allowed.
			const FPrimaryAssetId CropAssetId(UFarmCropData::AssetType, Tile.CropId);
			if (const UFarmCropData* Crop = Cast<UFarmCropData>(UFarmContentData::LoadWithVisuals(CropAssetId)))
			{
				Crops.Add(Tile.CropId, Crop);
			}
			else
			{
				Missing.Add(Tile.CropId);
			}
		}
		return Crops;
	}
}

FFarmLoadDecision UFarmSaveSubsystem::DecideLoad(TConstArrayView<FFarmSaveSlotRead> Slots)
{
	FFarmLoadDecision Decision;
	bool bAnyFile = false;
	for (int32 Slot = 0; Slot < Slots.Num(); ++Slot)
	{
		const FFarmSaveSlotRead& Read = Slots[Slot];
		bAnyFile |= Read.bExists;
		if (!Read.Save)
		{
			continue;
		}

		// Loading an older slot and then writing over the newer one would lose the newer game's progress.
		if (Read.Save->SaveVersion > UFarmSaveGame::CurrentVersion)
		{
			return {EFarmLoadOutcome::RefuseNewer, Slot};
		}
		if (Decision.Slot == INDEX_NONE || Read.Save->SaveSequence > Slots[Decision.Slot].Save->SaveSequence)
		{
			Decision = {EFarmLoadOutcome::Load, Slot};
		}
	}

	if (Decision.Slot == INDEX_NONE && bAnyFile)
	{
		Decision.Outcome = EFarmLoadOutcome::RefuseUnreadable;
	}
	return Decision;
}

int32 UFarmSaveSubsystem::GetSlotToWrite(int32 NewestSlot)
{
	return NewestSlot == 0 ? 1 : 0;
}

bool UFarmSaveSubsystem::UpgradeToCurrentVersion(UFarmSaveGame& Save, int32 GridWidth)
{
	if (Save.SaveVersion == 0 && !FarmSaveUpgrade::UpgradeFromVersion0(Save, GridWidth))
	{
		return false;
	}
	if (Save.SaveVersion == 1)
	{
		FarmSaveUpgrade::UpgradeFromVersion1(Save);
	}
	return Save.SaveVersion == UFarmSaveGame::CurrentVersion;
}

bool UFarmSaveSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UFarmSaveSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	// This runs before the actors' BeginPlay. The world broadcasts OnWorldBeginPlay once all of them have run, so the
	// grid's tiles and the farmer's toolbar are ready by then.
	WorldBeginPlayHandle = InWorld.OnWorldBeginPlay.AddUObject(this, &UFarmSaveSubsystem::BindAndLoad);
}

void UFarmSaveSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->OnWorldBeginPlay.Remove(WorldBeginPlayHandle);
	}
	if (DayCycle)
	{
		DayCycle->OnNewDayFinished.RemoveAll(this);
	}
	Super::Deinitialize();
}

void UFarmSaveSubsystem::StartNewGame()
{
	for (const TCHAR* SlotName : UFarmSaveGame::SlotNames)
	{
		UGameplayStatics::DeleteGameInSlot(SlotName, UFarmSaveGame::UserIndex);
	}
	UGameplayStatics::OpenLevel(GetWorld(), FName(*UGameplayStatics::GetCurrentLevelName(GetWorld())));
}

void UFarmSaveSubsystem::BindAndLoad()
{
	GetWorld()->OnWorldBeginPlay.Remove(WorldBeginPlayHandle);

	Grid = AFarmGrid::Find(GetWorld());
	DayCycle = AFarmDayCycle::Find(GetWorld());
	if (!Grid || !DayCycle)
	{
		UE_LOG(LogVestige, Warning, TEXT("Saving is off in %s: it needs a FarmGrid and a FarmDayCycle."), *GetWorld()->GetMapName());
		return;
	}

	DayCycle->OnNewDayFinished.AddUObject(this, &UFarmSaveSubsystem::SaveFarm);
	LoadFarm();
}

void UFarmSaveSubsystem::SaveFarm(int32 NewDay)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UFarmSaveSubsystem::SaveFarm);

	const UFarmingComponent* Farming = GetPlayerFarming();
	const UVestigeInventoryComponent* Inventory = GetPlayerInventory();
	if (!Farming || !Inventory)
	{
		return;
	}
	if (!bSavingAllowed)
	{
		UE_LOG(LogVestige, Warning, TEXT("Not saving day %d: the save couldn't be loaded, so it is kept as it is."), NewDay);
		return;
	}

	UFarmSaveGame* Save = Cast<UFarmSaveGame>(UGameplayStatics::CreateSaveGameObject(UFarmSaveGame::StaticClass()));
	Save->SaveVersion = UFarmSaveGame::CurrentVersion;
	Save->SaveSequence = NewestSequence + 1;
	Save->Clock = DayCycle->GetSaveData();
	Save->Farming = Farming->GetSaveData();
	Save->Inventory = Inventory->GetSaveData();
	Save->Tiles = Grid->GetTilesSaveData();

	const int32 Slot = GetSlotToWrite(NewestSlot);
	if (!UGameplayStatics::SaveGameToSlot(Save, UFarmSaveGame::SlotNames[Slot], UFarmSaveGame::UserIndex))
	{
		UE_LOG(LogVestige, Error, TEXT("Couldn't write the save to slot %s. The previous save is kept."), UFarmSaveGame::SlotNames[Slot]);
		UFarmFeedbackSubsystem::Notify(GetWorld(), LOCTEXT("SaveFailed", "Couldn't save the game. The previous save is kept."));
		return;
	}

	NewestSlot = Slot;
	NewestSequence = Save->SaveSequence;
	UFarmFeedbackSubsystem::Notify(GetWorld(), LOCTEXT("Saved", "Game saved."));
}

void UFarmSaveSubsystem::LoadFarm()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UFarmSaveSubsystem::LoadFarm);

	UFarmingComponent* Farming = GetPlayerFarming();
	UVestigeInventoryComponent* Inventory = GetPlayerInventory();
	if (!Farming || !Inventory)
	{
		UE_LOG(LogVestige, Warning, TEXT("Not loading the save: the player has no farming component or no inventory."));
		return;
	}

	TArray<FFarmSaveSlotRead, TInlineAllocator<UE_ARRAY_COUNT(UFarmSaveGame::SlotNames)>> Slots;
	for (const TCHAR* SlotName : UFarmSaveGame::SlotNames)
	{
		Slots.Add(FarmSaveSlots::ReadSlot(SlotName));
	}

	for (int32 Slot = 0; Slot < Slots.Num(); ++Slot)
	{
		if (Slots[Slot].bExists && !Slots[Slot].Save)
		{
			UE_LOG(LogVestige, Warning, TEXT("Save slot %s couldn't be read as a farm save."), UFarmSaveGame::SlotNames[Slot]);
		}
	}

	const FFarmLoadDecision Decision = DecideLoad(Slots);
	switch (Decision.Outcome)
	{
	case EFarmLoadOutcome::NewGame:
		return;

	case EFarmLoadOutcome::RefuseUnreadable:
		UE_LOG(LogVestige, Error, TEXT("No save slot could be read. The files are left as they are and saving is off."));
		RefuseToSave(LOCTEXT(
			"SaveUnreadable", "The save couldn't be read. It is kept as it is, and the game won't save until you start a new game."));
		return;

	case EFarmLoadOutcome::RefuseNewer:
		UE_LOG(LogVestige, Error, TEXT("Slot %s holds a save from a newer version (%d, this build writes %d). Saving is off."),
			UFarmSaveGame::SlotNames[Decision.Slot], Slots[Decision.Slot].Save->SaveVersion, UFarmSaveGame::CurrentVersion);
		RefuseToSave(LOCTEXT("SaveTooNew",
			"The save is from a newer version of the game. It is kept as it is, and the game won't save until you start a new game."));
		return;

	case EFarmLoadOutcome::Load:
		break;
	}

	UFarmSaveGame& Save = *Slots[Decision.Slot].Save;
	const int32 SavedVersion = Save.SaveVersion;
	if (SavedVersion < UFarmSaveGame::CurrentVersion)
	{
		BackUpBeforeUpgrade(Decision.Slot, SavedVersion);
		if (!UpgradeToCurrentVersion(Save, Grid->GetWidth()))
		{
			UE_LOG(LogVestige, Error, TEXT("The save in slot %s (version %d) couldn't be upgraded. Saving is off."),
				UFarmSaveGame::SlotNames[Decision.Slot], SavedVersion);
			RefuseToSave(LOCTEXT(
				"SaveNotUpgraded", "The save couldn't be read. It is kept as it is, and the game won't save until you start a new game."));
			return;
		}
	}

	UE_LOG(LogVestige, Log, TEXT("Loading day %d from save slot %s (version %d, save %d)."), Save.Clock.Day,
		UFarmSaveGame::SlotNames[Decision.Slot], SavedVersion, Save.SaveSequence);
	NewestSlot = Decision.Slot;
	NewestSequence = Save.SaveSequence;
	if (ApplySave(Save, *Farming, *Inventory) > 0)
	{
		UFarmFeedbackSubsystem::Notify(
			GetWorld(), LOCTEXT("ContentMissing", "Some saved things no longer exist in the game and were left out."));
	}
	UFarmFeedbackSubsystem::Notify(
		GetWorld(), FText::FormatNamed(LOCTEXT("Loaded", "Welcome back. Day {Day}."), TEXT("Day"), Save.Clock.Day));
}

int32 UFarmSaveSubsystem::ApplySave(const UFarmSaveGame& Save, UFarmingComponent& Farming, UVestigeInventoryComponent& Inventory)
{
	int32 SkippedCount = 0;
	DayCycle->RestoreFromSaveData(Save.Clock);
	if (!Farming.RestoreFromSaveData(Save.Farming))
	{
		++SkippedCount;
	}
	SkippedCount += Inventory.RestoreFromSaveData(Save.Inventory);
	SkippedCount += Grid->RestoreTilesFromSaveData(Save.Tiles, FarmSaveSlots::LoadSavedCrops(Save.Tiles));
	return SkippedCount;
}

void UFarmSaveSubsystem::RefuseToSave(const FText& Reason)
{
	bSavingAllowed = false;
	UFarmFeedbackSubsystem::Notify(GetWorld(), Reason);
}

void UFarmSaveSubsystem::BackUpBeforeUpgrade(int32 Slot, int32 Version) const
{
	const FString BackupSlot = FString::Printf(TEXT("%s_Version%d"), UFarmSaveGame::SlotNames[Slot], Version);
	if (UGameplayStatics::DoesSaveGameExist(BackupSlot, UFarmSaveGame::UserIndex))
	{
		return;
	}

	TArray<uint8> Bytes;
	if (!UGameplayStatics::LoadDataFromSlot(Bytes, UFarmSaveGame::SlotNames[Slot], UFarmSaveGame::UserIndex)
		|| !UGameplayStatics::SaveDataToSlot(Bytes, BackupSlot, UFarmSaveGame::UserIndex))
	{
		UE_LOG(LogVestige, Warning, TEXT("Couldn't back up the version %d save in slot %s before upgrading it."), Version,
			UFarmSaveGame::SlotNames[Slot]);
		return;
	}
	UE_LOG(LogVestige, Log, TEXT("Backed up the version %d save in slot %s to %s before upgrading it."), Version,
		UFarmSaveGame::SlotNames[Slot], *BackupSlot);
}

UFarmingComponent* UFarmSaveSubsystem::GetPlayerFarming() const
{
	const APawn* Pawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	return Pawn ? Pawn->FindComponentByClass<UFarmingComponent>() : nullptr;
}

UVestigeInventoryComponent* UFarmSaveSubsystem::GetPlayerInventory() const
{
	const APlayerState* PlayerState = UGameplayStatics::GetPlayerState(GetWorld(), 0);
	return PlayerState ? PlayerState->FindComponentByClass<UVestigeInventoryComponent>() : nullptr;
}

#undef LOCTEXT_NAMESPACE
