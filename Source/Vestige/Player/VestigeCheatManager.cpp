// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Player/VestigeCheatManager.h"

#include "Core/VestigeLog.h"
#include "Engine/World.h"
#include "Farming/VestigeRestComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Save/VestigeSaveSubsystem.h"

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
