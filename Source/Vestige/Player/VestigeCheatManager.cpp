// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Player/VestigeCheatManager.h"

#include "Content/FarmCropData.h"
#include "Content/VestigeContentCatalog.h"
#include "Algo/AllOf.h"
#include "Core/VestigeLog.h"
#include "Engine/World.h"
#include "Farming/FarmField.h"
#include "Farming/FarmGrid.h"
#include "Farming/FarmTileState.h"
#include "Farming/VestigeRestComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "Save/VestigeSaveSubsystem.h"
#include "Time/VestigeClockSubsystem.h"

namespace VestigeCheats
{
	/** The largest benchmark field side, so a typo can't spawn a million tiles. */
	constexpr int32 MaxBenchmarkFieldSize = 100;

	constexpr int32 HoursPerDay = 24;

	/** A clock time typed as digits: up to two are the hour (19), three or four the hour and minutes (930, 1930). */
	constexpr int32 MaxHourDigits = 2;
	constexpr int32 MaxTimeDigits = 4;
	constexpr int32 HourMinuteSplit = 100;

	/** Half a game minute, in hours. */
	constexpr float HalfMinuteHours = 0.5f / VestigeClock::MinutesPerHour;

	/** Reads "1930" (19:30), "930" (09:30) or "19" (19:00) as hours since midnight, up to 24:00. False for anything else. */
	bool ParseClockTime(const FString& Text, float& OutHours)
	{
		const bool bAllDigits = Algo::AllOf(Text,
			[](TCHAR Character)
			{
				return FChar::IsDigit(Character);
			});
		if (Text.IsEmpty() || Text.Len() > MaxTimeDigits || !bAllDigits)
		{
			return false;
		}

		const int32 Number = FCString::Atoi(*Text);
		const bool bHasMinutes = Text.Len() > MaxHourDigits;
		const int32 Hours = bHasMinutes ? Number / HourMinuteSplit : Number;
		const int32 Minutes = bHasMinutes ? Number % HourMinuteSplit : 0;
		const int32 TotalMinutes = Hours * VestigeClock::MinutesPerHour + Minutes;
		if (Minutes >= VestigeClock::MinutesPerHour || TotalMinutes > HoursPerDay * VestigeClock::MinutesPerHour)
		{
			return false;
		}
		OutHours = static_cast<float>(TotalMinutes) / VestigeClock::MinutesPerHour;
		return true;
	}
}

void UVestigeCheatManager::NewGame()
{
	UVestigeSaveSubsystem* Saves = UVestigeSaveSubsystem::Get(GetWorld());
	if (!Saves)
	{
		VESTIGE_LOG(this, LogVestige, Warning, "There is no save subsystem to start a new game with.");
		return;
	}
	Saves->StartNewGame();
}

void UVestigeCheatManager::SkipDay()
{
	UVestigeRestComponent* Rest = UVestigeRestComponent::Find(GetOuterAPlayerController()->PlayerState);
	if (!Rest)
	{
		VESTIGE_LOG(this, LogVestige, Warning, "The player has no rest component to sleep.");
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
		VESTIGE_LOG(this, LogVestige, Warning, "Give the time as digits, 1930 for 19:30, not {Time}.", Time);
		return;
	}

	// The clock counts on past midnight (25 is 01:00), and the day starts in the morning, so earlier hours are tonight's.
	const float TimeOfDay = Hours < Clock->GetDayStartHour() ? Hours + VestigeCheats::HoursPerDay : Hours;

	// Land inside the typed minute: in float hours a time such as 00:10 falls just short of it, and the clock would stop a
	// minute early, before that minute's events.
	if (!Clock->AdvanceTo(TimeOfDay + VestigeCheats::HalfMinuteHours))
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

	const UVestigeContentCatalog* Catalog = UVestigeContentCatalog::Get(GetWorld());
	const UFarmCropData* Crop = Catalog ? Catalog->Find<UFarmCropData>(FPrimaryAssetId(UFarmCropData::AssetType, CropId)) : nullptr;
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
