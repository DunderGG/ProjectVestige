// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"

/**
 * The rule of the farm's clean water store, kept apart from the subsystem so it is tested without a world: a daily supply,
 * the day water was last drawn on and how much was drawn then. What is left on a day is the supply minus what was drawn on
 * that day, and the whole supply on any other day, earlier or later, so a new day starts full and nothing carries over. It is
 * never refilled by an event, so no listener's order matters. Systems.md, section 24.
 */
struct FFarmWaterStore
{
public:
	FFarmWaterStore() = default;
	FFarmWaterStore(int32 InDay, int32 InDrawn) : Day(InDay), Drawn(FMath::Max(0, InDrawn)) {}

	/** How much is left on Today with DailySupply a day, never below 0. */
	int32 GetLeft(int32 Today, int32 DailySupply) const;

	/**
	 * Takes up to Amount from Today's water and returns how much it took: nothing for an Amount of 0 or less, never more than
	 * is left. The first draw of a day starts that day at nothing drawn.
	 */
	int32 Draw(int32 Today, int32 DailySupply, int32 Amount);

	/** The day water was last drawn on, or INDEX_NONE before anything was drawn. */
	int32 GetDay() const { return Day; }

	/** How much was drawn on GetDay. */
	int32 GetDrawn() const { return Drawn; }

private:
	int32 Day = INDEX_NONE;
	int32 Drawn = 0;
};
