// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"

/** How the game finds a component that lives on the player state, from the player's pawn or the player state itself. */
namespace VestigePlayerComponents
{
	/**
	 * The T component of PlayerActor: its own, or its player state's if it is a pawn. Nullptr if there is none. Searches the
	 * actor's components, so call it on a player action or once, never every frame.
	 */
	template <typename T> T* Find(const AActor* PlayerActor)
	{
		if (!PlayerActor)
		{
			return nullptr;
		}
		if (T* Own = PlayerActor->FindComponentByClass<T>())
		{
			return Own;
		}
		const APawn* Pawn = Cast<APawn>(PlayerActor);
		const APlayerState* PlayerState = Pawn ? Pawn->GetPlayerState() : nullptr;
		return PlayerState ? PlayerState->FindComponentByClass<T>() : nullptr;
	}
}
