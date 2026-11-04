// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Player/VestigeCheatManager.h"

#include "Content/FarmCropData.h"
#include "Content/VestigeContentData.h"
#include "Core/VestigeLog.h"
#include "Engine/World.h"
#include "Farming/FarmGrid.h"
#include "Farming/FarmTileState.h"
#include "Farming/VestigeRestComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "Save/VestigeSaveSubsystem.h"

namespace VestigeCheats
{
	/** The largest benchmark field side, so a typo can't spawn a million tiles. */
	constexpr int32 MaxBenchmarkFieldSize = 100;
}

void UVestigeCheatManager::NewGame()
{
	UVestigeSaveSubsystem* Saves = GetWorld() ? GetWorld()->GetSubsystem<UVestigeSaveSubsystem>() : nullptr;
	if (!Saves)
	{
		VESTIGE_LOG(this, LogVestige, Warning, "There is no save subsystem to start a new game with.");
		return;
	}
	Saves->StartNewGame();
}

void UVestigeCheatManager::SkipDay()
{
	const APawn* Pawn = GetOuterAPlayerController()->GetPawn();
	UVestigeRestComponent* Rest = Pawn ? Pawn->FindComponentByClass<UVestigeRestComponent>() : nullptr;
	if (!Rest)
	{
		VESTIGE_LOG(this, LogVestige, Warning, "The player has no pawn with a rest component to sleep.");
		return;
	}
	Rest->Sleep();
}

void UVestigeCheatManager::BenchmarkField(int32 FieldSize, FName CropId)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UVestigeCheatManager::BenchmarkField);

	if (FieldSize < 1 || FieldSize > VestigeCheats::MaxBenchmarkFieldSize)
	{
		VESTIGE_LOG(this, LogVestige, Warning, "The field size must be from 1 to {Max}, not {Size}.", VestigeCheats::MaxBenchmarkFieldSize,
			FieldSize);
		return;
	}

	AFarmGrid* Grid = AFarmGrid::Find(GetWorld());
	if (!Grid)
	{
		VESTIGE_LOG(this, LogVestige, Warning, "There is no farm grid to fill.");
		return;
	}

	// A developer command typed before measuring, not play, so waiting for the crop and its meshes to load is fine.
	const FPrimaryAssetId CropAssetId(UFarmCropData::AssetType, CropId);
	const UFarmCropData* Crop = Cast<UFarmCropData>(UVestigeContentData::LoadWithVisuals(CropAssetId));
	if (!Crop)
	{
		VESTIGE_LOG(this, LogVestige, Warning, "There is no crop {CropId}.", CropId);
		return;
	}

	Grid->Resize(FIntPoint(FieldSize));

	// The grid puts each tile into this state the way it restores a save, so the field follows the tiles' own rules.
	TArray<FFarmTileSaveData> RipeTiles;
	RipeTiles.Reserve(FieldSize * FieldSize);
	for (int32 Y = 0; Y < FieldSize; ++Y)
	{
		for (int32 X = 0; X < FieldSize; ++X)
		{
			FFarmTileSaveData& Tile = RipeTiles.AddDefaulted_GetRef();
			Tile.Coordinates = FIntPoint(X, Y);
			Tile.State = EFarmTileState::Planted;
			Tile.GrowthDays = Crop->GetDaysToMature();
			Tile.CropId = CropId;
		}
	}

	TMap<FName, const UFarmCropData*> Crops;
	Crops.Add(CropId, Crop);
	Grid->RestoreTilesFromSaveData(RipeTiles, Crops);
}
