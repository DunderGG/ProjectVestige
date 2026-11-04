// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "VestigeClockSettings.generated.h"

namespace VestigeClockLimits
{
	/** The fastest clock the settings allow: real seconds per game hour. */
	constexpr float FewestRealSecondsPerGameHour = 0.1f;

	/** The shortest day the settings allow, in game hours from the start of the day to its end. */
	constexpr float ShortestDayHours = 1.f;
}

/**
 * How the game's clock runs: its speed and when each day starts and ends. One clock for the whole game, whichever map is
 * open, so these are project settings rather than a level's. Project Settings > Game > Vestige Clock, stored in
 * Config/DefaultGame.ini. UVestigeClockSubsystem reads it once when the game starts.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Vestige Clock"))
class UVestigeClockSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UVestigeClockSettings& Get() { return *GetDefault<UVestigeClockSettings>(); }

	/** Real seconds per game hour, at least VestigeClockLimits::FewestRealSecondsPerGameHour. */
	float GetRealSecondsPerGameHour() const { return FMath::Max(RealSecondsPerGameHour, VestigeClockLimits::FewestRealSecondsPerGameHour); }

	float GetDayStartHour() const { return DayStartHour; }

	/** The hour the day ends, at least VestigeClockLimits::ShortestDayHours after GetDayStartHour. */
	float GetDayEndHour() const { return FMath::Max(DayEndHour, DayStartHour + VestigeClockLimits::ShortestDayHours); }

private:
	/** Real seconds per in-game hour. */
	UPROPERTY(Config, EditAnywhere, Category = "Time", meta = (AllowPrivateAccess = "true", ClampMin = 0.1))
	float RealSecondsPerGameHour = 40.f;

	/** Hour each day starts at after sleeping, and the first day at the start of a new game. */
	UPROPERTY(Config, EditAnywhere, Category = "Time", meta = (AllowPrivateAccess = "true", ClampMin = 0, ClampMax = 23))
	float DayStartHour = 6.f;

	/**
	 * Hour the day ends and the clock stops: OnDayEndReached is broadcast. 30 is 06:00 the next morning, so a player who
	 * never sleeps stays up the whole night and the next day starts at sunrise.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Time", meta = (AllowPrivateAccess = "true", ClampMin = 1, ClampMax = 30))
	float DayEndHour = 30.f;
};
