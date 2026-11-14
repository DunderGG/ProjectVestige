// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "VestigeGameMode.generated.h"

class UVestigeClockSubsystem;
struct FVestigeDayEnd;

/**
 * The rules of a game map (Architecture.md, section 2): the classes a player is made of, and the end of the day. When
 * the clock says the day has ended (the player went to bed, or stayed up until the day's end), it tells the player and
 * starts the next day, whether or not the player has a body, and it tells the player when a season begins. A sleep fade or
 * a rule such as "not during a cutscene" will live here. BP_GameMode sets the player's Blueprint classes;
 * GlobalDefaultGameMode names it, so every map uses it unless its World Settings say otherwise.
 */
UCLASS()
class AVestigeGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	/** Our HUD, player state and player controller, as fallbacks for the Blueprint's classes. */
	AVestigeGameMode();

protected:
	/** Follows the clock's OnDayEnded. */
	virtual void BeginPlay() override;

	/** Stops following the clock, which outlives this world. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/**
	 * Tells the player how the day ended and which day begins, in one day report whose tag says whether the new day starts a
	 * season or is a season's last (FVestigeCalendar::GetDayKind), and starts the next one. Bound to
	 * UVestigeClockSubsystem::OnDayEnded; does nothing when DayEnd is no longer the clock's current, stopped day.
	 */
	void StartNextDay(const FVestigeDayEnd& DayEnd);

	/** The game's clock, found at BeginPlay. It lives in the game instance, so this world doesn't own it. */
	TWeakObjectPtr<UVestigeClockSubsystem> Clock;
};
