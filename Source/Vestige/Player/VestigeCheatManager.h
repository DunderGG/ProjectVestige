// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "VestigeCheatManager.generated.h"

/**
 * The developer commands, typed in the console while playing (Systems.md, section 19). Each one asks the class that owns
 * the action to do it, so a cheat never has rules of its own. The player controller creates it; the engine never
 * creates a cheat manager in Shipping builds, so players can't reach these commands.
 */
UCLASS()
class UVestigeCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	/** Console: NewGame. Deletes both save slots and opens the map again on day 1, as the save subsystem's StartNewGame does. */
	UFUNCTION(Exec)
	void NewGame();

	/** Console: SkipDay. The player sleeps where they stand, as in the bed, to start the next day. */
	UFUNCTION(Exec)
	void SkipDay();

	/**
	 * Console: SkipToTime <Time>, with the time as digits: SkipToTime 1930 for 19:30, SkipToTime 19 for 19:00. Runs the
	 * clock forward to Time today, as if the hours had passed, so the hourly rules (the late-night warnings and drain)
	 * still happen. A time before the day's start hour means after midnight (100 is 01:00). The clock only runs forward;
	 * to start a day again, use SkipDay.
	 */
	UFUNCTION(Exec)
	void SkipToTime(const FString& Time);

	/**
	 * Console: BenchmarkField <Size> <CropId>, for example BenchmarkField 20 Base.Turnip. Makes the farm a Size x Size field
	 * with a ripe CropId on every tile, the busy farm the performance baseline measures (Architecture.md, section 10).
	 * The next save keeps that field, so use it in a build made for measuring.
	 */
	UFUNCTION(Exec)
	void BenchmarkField(int32 FieldSize, FName CropId);
};
