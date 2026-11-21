// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Farming/FarmWaterSubsystem.h"

#include "Core/VestigeSaveRegistry.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Farming/FarmLog.h"
#include "Farming/FarmWaterSaveData.h"
#include "Farming/FarmWaterSettings.h"
#include "Time/VestigeClockSubsystem.h"

UFarmWaterSubsystem* UFarmWaterSubsystem::Get(const UWorld* World)
{
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UFarmWaterSubsystem>() : nullptr;
}

void UFarmWaterSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// The clock's day is what the store's refill reads; the registry holds the save part.
	Clock = Collection.InitializeDependency<UVestigeClockSubsystem>();
	SaveRegistry = Collection.InitializeDependency<UVestigeSaveRegistry>();
	if (SaveRegistry)
	{
		SaveRegistry->RegisterGameOwner(*this);
	}
}

void UFarmWaterSubsystem::Deinitialize()
{
	if (SaveRegistry)
	{
		SaveRegistry->UnregisterGameOwner(*this);
	}
	Super::Deinitialize();
}

int32 UFarmWaterSubsystem::GetCleanWaterLeft() const
{
	return Clock ? Store.GetLeft(Clock->GetCurrentDay(), UFarmWaterSettings::Get().GetWellWaterPerDay()) : 0;
}

int32 UFarmWaterSubsystem::DrawCleanWater(int32 Amount)
{
	if (!Clock)
	{
		return 0;
	}
	const int32 Taken = Store.Draw(Clock->GetCurrentDay(), UFarmWaterSettings::Get().GetWellWaterPerDay(), Amount);
	VESTIGE_LOG(this, LogFarm, Verbose, "Drew {Taken} of the {Amount} clean water asked for, {Left} left today.", Taken, Amount,
		GetCleanWaterLeft());
	return Taken;
}

void UFarmWaterSubsystem::StartNewGame()
{
	Store = FFarmWaterStore();
}

FName UFarmWaterSubsystem::GetSaveKey() const
{
	static const FName Key(TEXT("Base.FarmWater"));
	return Key;
}

int32 UFarmWaterSubsystem::GetSaveVersion() const
{
	return FFarmWaterSaveData::Version;
}

FVestigeSaveRecord UFarmWaterSubsystem::WriteSaveRecord() const
{
	FFarmWaterSaveData Data;
	Data.Day = Store.GetDay();
	Data.CleanWaterDrawn = Store.GetDrawn();
	return FVestigeSaveRecord::Write(Data);
}

bool UFarmWaterSubsystem::ReadSaveRecord(const FVestigeSaveRecord& Record, int32& OutLeftOutCount)
{
	FFarmWaterSaveData Data;
	if (!Record.Read(Data))
	{
		return false;
	}
	Store = FFarmWaterStore(Data.Day, Data.CleanWaterDrawn);
	return true;
}
