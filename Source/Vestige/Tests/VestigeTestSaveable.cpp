// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Tests/VestigeTestSaveable.h"

#if WITH_TESTS

void UVestigeTestSaveable::Setup(FName Key, int32 OwnerVersion)
{
	SaveKey = Key;
	SaveVersion = OwnerVersion;
}

FVestigeSaveRecord UVestigeTestSaveable::WriteSaveRecord() const
{
	return FVestigeSaveRecord::Write(State);
}

bool UVestigeTestSaveable::ReadSaveRecord(const FVestigeSaveRecord& Record, int32& OutLeftOutCount)
{
	FVestigeTestSaveData Data;
	if (!Record.Read(Data))
	{
		return false;
	}
	State = Data;
	return true;
}

void UVestigeTestSaveable::StartNewGame()
{
	State = FVestigeTestSaveData();
	++NewGameCount;
}

#endif
