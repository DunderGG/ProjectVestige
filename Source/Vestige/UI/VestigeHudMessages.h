// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"

/** How the feedback message boxes fade, in seconds. */
namespace VestigeHudMessageFade
{
	constexpr double FadeInSeconds = 0.15;
	constexpr double FadeOutSeconds = 0.4;
}

/** One feedback message on screen: its text and the time it appeared. */
struct FVestigeHudMessage
{
	FText Text;

	/** World time, in seconds, when the message appeared. */
	double StartTime = 0.0;
};

/**
 * The feedback messages on screen, oldest first. Each shows in its own box for its own lifetime, fading in and out, and
 * the oldest goes when the list is full. Plain C++, so the rules are tested without UMG or a world: AVestigeHUD keeps one
 * and UVestigeHudWidget shows it, each message in a UVestigeHudMessageWidget.
 */
class FVestigeHudMessages
{
public:
	/** Adds Text as a new message at Now. With MaxMessages already showing, the oldest goes to make room. */
	void Add(const FText& Text, double Now, int32 MaxMessages);

	/** Drops the messages that have been showing for Lifetime seconds or longer at Now. Returns true if any went. */
	bool RemoveExpired(double Now, double Lifetime);

	/** Seconds from Now until the oldest message expires, or no value when there are none. */
	TOptional<double> GetSecondsUntilNextExpiry(double Now, double Lifetime) const;

	/**
	 * How visible a message that appeared at StartTime is at Now, from 0 to 1. It fades in over FadeInSeconds after it
	 * appears and out over the last FadeOutSeconds of its Lifetime, and is fully visible in between.
	 */
	static float GetOpacity(double StartTime, double Now, double Lifetime);

	const TArray<FVestigeHudMessage>& GetMessages() const { return Messages; }
	bool IsEmpty() const { return Messages.IsEmpty(); }

private:
	TArray<FVestigeHudMessage> Messages;
};
