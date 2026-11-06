// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Player/VestigeCheatManager.h"

#include "Content/FarmCropData.h"
#include "Content/VestigeContentData.h"
#include "Core/VestigeLog.h"
#include "Engine/World.h"
#include "Farming/FarmField.h"
#include "Farming/FarmGrid.h"
#include "Farming/FarmTileState.h"
#include "Farming/VestigeRestComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "Save/VestigeSaveSubsystem.h"
#include "Time/VestigeClockSubsystem.h"

namespace VestigeCheats
{
	/** The largest benchmark field side, so a typo can't spawn a million tiles. */
	constexpr int32 MaxBenchmarkFieldSize = 100;

	constexpr float HoursPerDay = 24.f;
	constexpr float MinutesPerHour = 60.f;

	/** Reads "19:30" or "19.5" as hours since midnight, from 0 up to 24. Returns false for anything else. */
	bool ParseClockTime(const FString& Text, float& OutHours)
	{
		FString HoursText = Text;
		FString MinutesText;
		const bool bHasMinutes = Text.Split(TEXT(":"), &HoursText, &MinutesText);
		if (!HoursText.IsNumeric() || (bHasMinutes && !MinutesText.IsNumeric()))
		{
			return false;
		}

		const float Minutes = bHasMinutes ? FCString::Atof(*MinutesText) : 0.f;
		OutHours = FCString::Atof(*HoursText) + Minutes / MinutesPerHour;
		return Minutes >= 0.f && Minutes < MinutesPerHour && OutHours >= 0.f && OutHours <= HoursPerDay;
	}
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

void UVestigeCheatManager::SkipToTime(const FString& Time)
{
	UVestigeClockSubsystem* Clock = UVestigeClockSubsystem::Get(GetWorld());
	if (!Clock)
	{
		VESTIGE_LOG(this, LogVestige, Warning, "There is no clock to run forward.");
		return;
	}

	float Hours = 0.f;
	if (!VestigeCheats::ParseClockTime(Time, Hours))
	{
		VESTIGE_LOG(this, LogVestige, Warning, "Give the time as 19:30 or 19.5, not {Time}.", Time);
		return;
	}

	// The clock counts on past midnight (25 is 01:00), and the day starts in the morning, so earlier hours are tonight's.
	const float TimeOfDay = Hours < Clock->GetDayStartHour() ? Hours + VestigeCheats::HoursPerDay : Hours;
	if (!Clock->AdvanceTo(TimeOfDay))
	{
		VESTIGE_LOG(this, LogVestige, Warning, "The clock only runs forward and stops at the day's end; {Time} isn't later today.", Time);
	}
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
	if (!Grid || !Grid->GetField())
	{
		VESTIGE_LOG(this, LogVestige, Warning, "There is no farm grid with a field to fill.");
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

	// The field puts each tile into this state the way it restores a save, so the field follows the tiles' own rules.
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
	Grid->GetField()->RestoreTilesFromSaveData(RipeTiles, Crops);
}
