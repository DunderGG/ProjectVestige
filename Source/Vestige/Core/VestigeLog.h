// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Logging/StructuredLog.h"

/** Level setup that belongs to no one system, and Epic's template code. Each system has its own category (Systems.md, section 21). */
DECLARE_LOG_CATEGORY_EXTERN(LogVestige, Log, All);

/** Every message for the player, as UVestigeFeedbackSubsystem::Notify sends it, so the log keeps what the HUD fades. */
DECLARE_LOG_CATEGORY_EXTERN(LogVestigeFeedback, Log, All);

namespace VestigeLog
{
	/**
	 * How a log line names its sender: an actor's or subsystem's own name, a component's owner and name
	 * (Owner.Component, since a component's name alone isn't unique), or None for nullptr.
	 */
	FString GetSenderName(const UObject* Sender);
}

/**
 * Logs a line that starts with its sender's name and the function it came from, through UE_LOGFMT:
 * VESTIGE_LOG(this, LogFarm, Warning, "Slot {Slot} is empty.", SlotIndex) prints
 * "LogFarm: Warning: [FarmGrid_1] AFarmGrid::SpawnTiles: Slot 3 is empty."
 * Sender is any UObject, or nullptr in a function with no object to name. Format is a plain string literal whose fields
 * in braces are given in order after it; Sender and Function are taken. Nothing is evaluated when the category is off.
 */
#define VESTIGE_LOG(Sender, CategoryName, Verbosity, Format, ...)                                                                          \
	UE_LOGFMT(CategoryName, Verbosity, "[{Sender}] {Function}: " Format, ::VestigeLog::GetSenderName(Sender), __FUNCTION__, ##__VA_ARGS__)
