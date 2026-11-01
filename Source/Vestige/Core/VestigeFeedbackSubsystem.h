// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "VestigeFeedbackSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnVestigeFeedback, const FText& /* Message */);

/**
 * Carries short messages for the player ("Tilled the soil.", "Game saved.") from game code to whatever shows them.
 * Game code raises a message and never knows who listens; AVestigeHUD listens and displays it.
 * The game is single-player, so the world's player is the one the messages are for.
 */
UCLASS()
class UVestigeFeedbackSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Raises Message for the player in World. Does nothing without a world or before the subsystem exists. */
	static void Notify(const UWorld* World, const FText& Message);

	/** Broadcast with each message as it is raised. Presentation listens; game code doesn't. */
	FOnVestigeFeedback OnFeedback;
};
