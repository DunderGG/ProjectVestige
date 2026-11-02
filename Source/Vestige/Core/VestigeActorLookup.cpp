// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Core/VestigeActorLookup.h"

#include "Core/VestigeLog.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"

AActor* VestigeActorLookup::FindSingle(const UWorld* World, TSubclassOf<AActor> ActorClass)
{
	if (!World || !ActorClass)
	{
		return nullptr;
	}

	TActorIterator<AActor> It(World, ActorClass);
	AActor* First = It ? *It : nullptr;
	if (!First)
	{
		return nullptr;
	}

	++It;
	if (It)
	{
		VESTIGE_LOG(World, LogVestige, Error, "The level has more than one {Class}. Only {Used} is used; delete the others.",
			GetNameSafe(ActorClass), First->GetName());
	}
	return First;
}
