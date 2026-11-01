// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"

class AActor;
class UWorld;

/** How the game finds the actors it expects exactly one of in a world. Each such class wraps this in its own static Find. */
namespace VestigeActorLookup
{
	/**
	 * The first actor of ActorClass in World, or nullptr if there is none or World is null.
	 * Logs an error, and still returns the first one, if the world has more than one: that is a level setup mistake.
	 * Searches the world, so call it once and keep the result, never every frame.
	 */
	AActor* FindSingle(const UWorld* World, TSubclassOf<AActor> ActorClass);
}
