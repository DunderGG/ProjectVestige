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
};
