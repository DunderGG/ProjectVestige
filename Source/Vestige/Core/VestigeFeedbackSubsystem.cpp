// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Core/VestigeFeedbackSubsystem.h"

#include "Core/Vestige.h"
#include "Engine/World.h"

void UVestigeFeedbackSubsystem::Notify(const UWorld* World, const FText& Message)
{
	// The HUD lets messages fade, so the log keeps the full history (Output Log, filtered to LogVestige).
	UE_LOG(LogVestige, Log, TEXT("Feedback: %s"), *Message.ToString());

	if (UVestigeFeedbackSubsystem* Feedback = UWorld::GetSubsystem<UVestigeFeedbackSubsystem>(World))
	{
		Feedback->OnFeedback.Broadcast(Message);
	}
}
