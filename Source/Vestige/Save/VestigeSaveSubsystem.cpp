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
#include "Farming/FarmGrid.h"
#include "Farming/FarmingComponent.h"
#include "Farming/VestigeRestComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Inventory/VestigeInventoryComponent.h"
#include "Kismet/GameplayStatics.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "Save/VestigeSaveGame.h"
#include "Save/VestigeSaveLog.h"
#include "Time/VestigeDayCycle.h"

#define LOCTEXT_NAMESPACE "VestigeSaveSubsystem"

namespace VestigeSaveUpgrade
{
	/** Version 0 saves could only plant the seed crop, and the seed crop was always the turnip. */
	const FName Version0CropId(TEXT("Base.Turnip"));

	/** The ContentId of the tool in hand in a version 0 save, which may only have the old tool enum. */
	FName GetVersion0ToolId(const UVestigeSaveGame& Save)
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
	bool UpgradeFromVersion0(UVestigeSaveGame& Save, int32 GridWidth)
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
	void UpgradeFromVersion1(UVestigeSaveGame& Save)
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

	/** Version 2 had no stamina. Its Stamina part reads as unsaved, so the player starts with full stamina. */
	void UpgradeFromVersion2(UVestigeSaveGame& Save)
	{
		Save.SaveVersion = 3;
	}

	/** Version 3 had no exhaustion. Its Rest part reads as not exhausted. */
	void UpgradeFromVersion3(UVestigeSaveGame& Save)
	{
		Save.SaveVersion = 4;
	}
}

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
			if (const UFarmCropData* Crop = Cast<UFarmCropData>(UVestigeContentData::LoadWithVisuals(CropAssetId)))
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

bool UVestigeSaveSubsystem::UpgradeToCurrentVersion(UVestigeSaveGame& Save, int32 GridWidth)
{
	if (Save.SaveVersion == 0 && !VestigeSaveUpgrade::UpgradeFromVersion0(Save, GridWidth))
	{
		return false;
	}
	if (Save.SaveVersion == 1)
	{
		VestigeSaveUpgrade::UpgradeFromVersion1(Save);
	}
	if (Save.SaveVersion == 2)
	{
		VestigeSaveUpgrade::UpgradeFromVersion2(Save);
	}
	if (Save.SaveVersion == 3)
	{
		VestigeSaveUpgrade::UpgradeFromVersion3(Save);
	}
	return Save.SaveVersion == UVestigeSaveGame::CurrentVersion;
}

bool UVestigeSaveSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UVestigeSaveSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	// This runs before the actors' BeginPlay. The world broadcasts OnWorldBeginPlay once all of them have run, so the
	// grid's tiles and the farmer's toolbar are ready by then.
	WorldBeginPlayHandle = InWorld.OnWorldBeginPlay.AddUObject(this, &UVestigeSaveSubsystem::BindAndLoad);
}

void UVestigeSaveSubsystem::Deinitialize()
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

void UVestigeSaveSubsystem::StartNewGame()
{
	for (const TCHAR* SlotName : UVestigeSaveGame::SlotNames)
	{
		UGameplayStatics::DeleteGameInSlot(SlotName, UVestigeSaveGame::UserIndex);
	}
	UGameplayStatics::OpenLevel(GetWorld(), FName(*UGameplayStatics::GetCurrentLevelName(GetWorld())));
}

void UVestigeSaveSubsystem::BindAndLoad()
{
	GetWorld()->OnWorldBeginPlay.Remove(WorldBeginPlayHandle);

	Grid = AFarmGrid::Find(GetWorld());
	DayCycle = AVestigeDayCycle::Find(GetWorld());
	if (!Grid || !DayCycle)
	{
		VESTIGE_LOG(this, LogVestigeSave, Warning, "Saving is off in {Level}: it needs a FarmGrid and a VestigeDayCycle.",
			GetWorld()->GetMapName());
		return;
	}

	DayCycle->OnNewDayFinished.AddUObject(this, &UVestigeSaveSubsystem::SaveFarm);
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
	Save->Clock = DayCycle->GetSaveData();
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
	Save->Tiles = Grid->GetTilesSaveData();

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

	UVestigeSaveGame& Save = *Slots[Decision.Slot].Save;
	const int32 SavedVersion = Save.SaveVersion;
	if (SavedVersion < UVestigeSaveGame::CurrentVersion)
	{
		BackUpBeforeUpgrade(Decision.Slot, SavedVersion);
		if (!UpgradeToCurrentVersion(Save, Grid->GetWidth()))
		{
			VESTIGE_LOG(this, LogVestigeSave, Error, "The save in slot {Slot} (version {Version}) couldn't be upgraded. Saving is off.",
				UVestigeSaveGame::SlotNames[Decision.Slot], SavedVersion);
			RefuseToSave(LOCTEXT(
				"SaveNotUpgraded", "The save couldn't be read. It is kept as it is, and the game won't save until you start a new game."));
			return;
		}
	}

	VESTIGE_LOG(this, LogVestigeSave, Log, "Loading day {Day} from save slot {Slot} (version {Version}, save {Sequence}).", Save.Clock.Day,
		UVestigeSaveGame::SlotNames[Decision.Slot], SavedVersion, Save.SaveSequence);
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
	DayCycle->RestoreFromSaveData(Save.Clock);
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
	SkippedCount += Grid->RestoreTilesFromSaveData(Save.Tiles, VestigeSaveSlots::LoadSavedCrops(Save.Tiles));
	return SkippedCount;
}

void UVestigeSaveSubsystem::RefuseToSave(const FText& Reason)
{
	bSavingAllowed = false;
	UVestigeFeedbackSubsystem::Notify(GetWorld(), Reason);
}

void UVestigeSaveSubsystem::BackUpBeforeUpgrade(int32 Slot, int32 Version) const
{
	const FString BackupSlot = FString::Printf(TEXT("%s_Version%d"), UVestigeSaveGame::SlotNames[Slot], Version);
	if (UGameplayStatics::DoesSaveGameExist(BackupSlot, UVestigeSaveGame::UserIndex))
	{
		return;
	}

	TArray<uint8> Bytes;
	if (!UGameplayStatics::LoadDataFromSlot(Bytes, UVestigeSaveGame::SlotNames[Slot], UVestigeSaveGame::UserIndex)
		|| !UGameplayStatics::SaveDataToSlot(Bytes, BackupSlot, UVestigeSaveGame::UserIndex))
	{
		VESTIGE_LOG(this, LogVestigeSave, Warning, "Couldn't back up the version {Version} save in slot {Slot} before upgrading it.",
			Version, UVestigeSaveGame::SlotNames[Slot]);
		return;
	}
	VESTIGE_LOG(this, LogVestigeSave, Log, "Backed up the version {Version} save in slot {Slot} to {Backup} before upgrading it.", Version,
		UVestigeSaveGame::SlotNames[Slot], BackupSlot);
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
