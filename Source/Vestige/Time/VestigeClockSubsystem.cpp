// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Time/VestigeClockSubsystem.h"

#include "Core/VestigeSaveRegistry.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "Stats/Stats.h"
#include "Time/VestigeClockSaveData.h"
#include "Time/VestigeClockSettings.h"

UVestigeClockSubsystem::UVestigeClockSubsystem() : FTickableGameObject(ETickableTickType::Never)
{
}

UVestigeClockSubsystem* UVestigeClockSubsystem::Get(const UWorld* World)
{
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UVestigeClockSubsystem>() : nullptr;
}

void UVestigeClockSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	const UVestigeClockSettings& Settings = UVestigeClockSettings::Get();
	RealSecondsPerGameHour = Settings.GetRealSecondsPerGameHour();
	DayStartHour = Settings.GetDayStartHour();
	DayEndHour = Settings.GetDayEndHour();
	SunriseHour = Settings.GetSunriseHour();
	SunsetHour = Settings.GetSunsetHour();

	// Registering restores the saved day and clock, or starts day 1.
	SaveRegistry = Collection.InitializeDependency<UVestigeSaveRegistry>();
	if (SaveRegistry)
	{
		SaveRegistry->RegisterGameOwner(*this);
	}
	else
	{
		StartNewGame();
	}

	// IsTickable decides each frame whether the clock runs.
	SetTickableTickType(ETickableTickType::Conditional);
}

void UVestigeClockSubsystem::Deinitialize()
{
	SetTickableTickType(ETickableTickType::Never);
	if (SaveRegistry)
	{
		SaveRegistry->UnregisterGameOwner(*this);
	}
	Super::Deinitialize();
}

void UVestigeClockSubsystem::StartNextDay()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UVestigeClockSubsystem::StartNextDay);

	++CurrentDay;
	TimeOfDay = DayStartHour;
	bDayEndReached = false;
	OnNewDay.Broadcast(CurrentDay);
	OnNewDayBlueprint.Broadcast(CurrentDay);
	OnNewDayFinished.Broadcast(CurrentDay);
}

void UVestigeClockSubsystem::StartNewGame()
{
	CurrentDay = 1;
	TimeOfDay = DayStartHour;
	bDayEndReached = false;
}

FName UVestigeClockSubsystem::GetSaveKey() const
{
	static const FName Key(TEXT("Base.Clock"));
	return Key;
}

int32 UVestigeClockSubsystem::GetSaveVersion() const
{
	return FVestigeClockSaveData::Version;
}

FVestigeSaveRecord UVestigeClockSubsystem::WriteSaveRecord() const
{
	return FVestigeSaveRecord::Write(GetSaveData());
}

bool UVestigeClockSubsystem::ReadSaveRecord(const FVestigeSaveRecord& Record, int32& OutLeftOutCount)
{
	FVestigeClockSaveData Data;
	if (!Record.Read(Data))
	{
		return false;
	}
	RestoreFromSaveData(Data);
	return true;
}

FVestigeClockSaveData UVestigeClockSubsystem::GetSaveData() const
{
	FVestigeClockSaveData Data;
	Data.Day = CurrentDay;
	Data.TimeOfDay = TimeOfDay;
	return Data;
}

void UVestigeClockSubsystem::RestoreFromSaveData(const FVestigeClockSaveData& Data)
{
	CurrentDay = FMath::Max(1, Data.Day);
	TimeOfDay = FMath::Clamp(Data.TimeOfDay, DayStartHour, DayEndHour);

	// A clock restored at the day's end reaches it on the next tick, so OnDayEndReached isn't lost.
	bDayEndReached = false;
}

bool UVestigeClockSubsystem::AdvanceTo(float NewTimeOfDay)
{
	if (bDayEndReached || NewTimeOfDay <= TimeOfDay)
	{
		return false;
	}
	RunTo(NewTimeOfDay);
	return true;
}

void UVestigeClockSubsystem::Tick(float DeltaSeconds)
{
	RunTo(TimeOfDay + DeltaSeconds / RealSecondsPerGameHour);
}

void UVestigeClockSubsystem::RunTo(float NewTimeOfDay)
{
	const float PreviousTimeOfDay = TimeOfDay;
	TimeOfDay = FMath::Min(NewTimeOfDay, DayEndHour);
	BroadcastHoursStartedSince(PreviousTimeOfDay);
	if (TimeOfDay < DayEndHour || bDayEndReached)
	{
		return;
	}

	// The clock stops at the day's end until the next day starts. Stop first, so a listener that starts the next day
	// starts the clock again.
	bDayEndReached = true;
	OnDayEndReached.Broadcast(CurrentDay);
}

bool UVestigeClockSubsystem::IsTickable() const
{
	const UWorld* World = GetTickableGameObjectWorld();
	return !bDayEndReached && World && World->IsGameWorld();
}

UWorld* UVestigeClockSubsystem::GetTickableGameObjectWorld() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetWorld() : nullptr;
}

TStatId UVestigeClockSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UVestigeClockSubsystem, STATGROUP_Tickables);
}

void UVestigeClockSubsystem::BroadcastHoursStartedSince(float PreviousTimeOfDay)
{
	const int32 LastHour = FMath::FloorToInt32(TimeOfDay);
	for (int32 Hour = FMath::FloorToInt32(PreviousTimeOfDay) + 1; Hour <= LastHour; ++Hour)
	{
		OnHourStarted.Broadcast(Hour);
	}
}
