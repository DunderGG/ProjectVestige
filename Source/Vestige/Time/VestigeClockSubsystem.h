// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Content/VestigeCalendar.h"
#include "Core/VestigeSaveable.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "VestigeClockSubsystem.generated.h"

struct FVestigeClockSaveData;
class UVestigeSaveRegistry;

namespace VestigeClock
{
	/** Game minutes in a game hour. */
	constexpr int32 MinutesPerHour = 60;

	/**
	 * The whole game minutes in Hours (GetTimeOfDay's hours), worked out in double: a float times 60 is exact there, so a
	 * minute count is a whole hour exactly when FloorToInt32(Hours) is that hour.
	 */
	inline int32 ToWholeMinutes(float Hours)
	{
		return FMath::FloorToInt32(static_cast<double>(Hours) * MinutesPerHour);
	}
}

DECLARE_MULTICAST_DELEGATE_OneParam(FOnVestigeNewDay, int32 /* Day */);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVestigeNewDayBlueprint, int32, Day);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnVestigeHourStarted, int32 /* Hour */);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnVestigeMinuteStarted, int32 /* Minute */);

/** How the last day ended, kept by UVestigeClockSubsystem from that day's end until the next day ends. */
struct FVestigeDayEnd
{
	/** The day that ended. */
	int32 Day = 0;

	/** When it ended, in GetTimeOfDay's hours (25 is 01:00 after midnight). */
	float TimeOfDay = 0.f;

	/** True when it ended before the day's end hour, because the player went to bed; false when the clock reached it. */
	bool bEndedEarly = false;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnVestigeDayEnded, const FVestigeDayEnd& /* DayEnd */);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnVestigeSeasonStarted, const FVestigeDate& /* Date */);

/**
 * The game's time: the day counter and the clock, the one owner of game time (Systems.md, section 4). A game-instance
 * subsystem, so the clock keeps its time across map changes and runs in whichever game map is open. It advances with that
 * world's time, so pausing stops it and time dilation (slomo) speeds it up, and it stands still while a map loads. Its
 * speed and hours, and the hours of sunrise and sunset, come from UVestigeClockSettings. Presentation (the sun, the HUD) reads it; nothing else keeps a clock.
 * The day count is also the calendar: the date is worked out from it with the game data's calendar, copied at Initialize.
 * Saves the day and the clock under Base.Clock through UVestigeSaveRegistry, for the whole game.
 */
UCLASS(BlueprintType, meta = (DisplayName = "Vestige Clock"))
class UVestigeClockSubsystem : public UGameInstanceSubsystem, public FTickableGameObject, public IVestigeSaveable
{
	GENERATED_BODY()

public:
	/** Ticking starts in Initialize, once the subsystem is set up, as FTickableGameObject requires of UObjects. */
	UVestigeClockSubsystem();

	/** The clock of World's game instance, or nullptr in a world without one (editor and preview worlds). */
	static UVestigeClockSubsystem* Get(const UWorld* World);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * Ends the day now, before its end hour, as when the player goes to bed: the clock stops where it is and OnDayEnded is
	 * broadcast, as when the clock reaches the day's end. Returns false, and changes nothing, when the day has already ended.
	 */
	bool EndDay();

	/**
	 * Sets the clock to the next morning, then broadcasts OnNewDay, OnNewDayBlueprint and OnNewDayFinished. The game mode
	 * calls it once the day has ended (OnDayEnded); a day that hasn't ended counts as ended now, without OnDayEnded.
	 */
	void StartNextDay();

	/** Puts the clock back to the morning of day 1 for a new game, without broadcasting anything. */
	virtual void StartNewGame() override;

	virtual FName GetSaveKey() const override;
	virtual int32 GetSaveVersion() const override;
	virtual FVestigeSaveRecord WriteSaveRecord() const override;
	virtual bool ReadSaveRecord(const FVestigeSaveRecord& Record, int32& OutLeftOutCount) override;

	/** The day and clock, for saving. */
	FVestigeClockSaveData GetSaveData() const;

	/**
	 * Sets the day and clock from a save without broadcasting anything. The day becomes at least 1 and the clock is
	 * clamped between the day's start and end; a clock at the day's end still broadcasts OnDayEnded on its next tick.
	 */
	void RestoreFromSaveData(const FVestigeClockSaveData& Data);

	/**
	 * Runs the clock forward to NewTimeOfDay at once, as if the time had passed: OnMinuteStarted and OnHourStarted are broadcast
	 * for every whole minute and hour on the way, and OnDayEnded if it reaches the day's end, where it stops. For developer commands.
	 * NewTimeOfDay is in GetTimeOfDay's hours (25 is 01:00 after midnight). Returns false, and changes nothing, when
	 * NewTimeOfDay isn't later than the clock or the clock has stopped at the day's end.
	 */
	bool AdvanceTo(float NewTimeOfDay);

	/**
	 * Sets the day count to Day and keeps the time, broadcasting nothing, as a restore does: the days between don't happen (no
	 * crop grows), and the next new day saves the new count. For developer commands. Returns false, and changes nothing, for a
	 * day before 1.
	 */
	bool JumpToDay(int32 Day);

	/** The day count, from 1: the only calendar state. GetDate works out the date from it. */
	int32 GetCurrentDay() const { return CurrentDay; }

	/** Today's date: the year, the season, the day of the season and the days left. */
	FVestigeDate GetDate() const { return Calendar.GetDate(CurrentDay); }

	/** The game's calendar, copied from the game data at Initialize: the seasons, their names and the days in each. */
	const FVestigeCalendar& GetCalendar() const { return Calendar; }

	/** Hours since midnight at the start of the current day. Goes past 24 after midnight, up to GetDayEndHour. */
	float GetTimeOfDay() const { return TimeOfDay; }

	/** Hour each day starts at after sleeping. */
	float GetDayStartHour() const { return DayStartHour; }

	/** Hour the day ends and the clock stops. Past 24 means after midnight. */
	float GetDayEndHour() const { return DayEndHour; }

	float GetRealSecondsPerGameHour() const { return RealSecondsPerGameHour; }

	/** Hour the sun rises, the same on every map. The sky lighting follows it; game rules may ask whether it is day. */
	float GetSunriseHour() const { return SunriseHour; }

	/** Hour the sun sets, later than GetSunriseHour. */
	float GetSunsetHour() const { return SunsetHour; }

	/** False once the day has ended and the clock has stopped, until the next day starts. */
	bool IsRunning() const { return !bDayEndReached; }

	/**
	 * How the last day ended: its number, when, and whether the player went to bed early. Read on OnNewDay to know how
	 * the night before went. Day is 0 before any day has ended in this run of the game.
	 */
	const FVestigeDayEnd& GetLastDayEnd() const { return LastDayEnd; }

	/** Runs the clock forward by DeltaSeconds of the world's time. The engine calls it every frame while IsTickable. */
	virtual void Tick(float DeltaSeconds) override;

	/** True while the clock runs and its game instance has a game world. */
	virtual bool IsTickable() const override;

	/** The game instance's current world, so the clock ticks with it: paused when it is, dilated when it is. */
	virtual UWorld* GetTickableGameObjectWorld() const override;

	virtual TStatId GetStatId() const override;

	/** Broadcast with the new day's number every time a new day starts. The farm reacts here, e.g. crops grow. */
	FOnVestigeNewDay OnNewDay;

	/**
	 * Extension point: OnNewDay for Blueprints, with the new day's number. Broadcast after OnNewDay, so the farm has
	 * reacted (crops have grown), and before OnNewDayFinished, so what a listener changes is saved. The same contract as
	 * OnNewDay: finish every change to saved state in the handler (Architecture.md, section 15). A Blueprint finds the
	 * clock with the Get Vestige Clock node.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Time", meta = (DisplayName = "On New Day"))
	FOnVestigeNewDayBlueprint OnNewDayBlueprint;

	/** Broadcast after OnNewDay and OnNewDayBlueprint, once everything has reacted to the new day. Saving listens here. */
	FOnVestigeNewDay OnNewDayFinished;

	/**
	 * Broadcast once per day when the day ends: when the clock reaches the day's end, or earlier through EndDay (the player
	 * went to bed). The clock then stops until StartNextDay. AVestigeGameMode starts the next day from here, so a listener
	 * after it may already be in the new day: react to the night on OnNewDay with GetLastDayEnd instead. In a world whose
	 * game mode isn't ours nothing starts the next day, so the clock stays stopped (logged as a warning).
	 */
	FOnVestigeDayEnded OnDayEnded;

	/**
	 * Broadcast with the new day's date when StartNextDay starts the first day of a season: after the day count moves and
	 * before OnNewDay, so the season's rules (crops that wither) apply before the day's. The same contract as OnNewDay: finish
	 * every change to saved state in the handler. Not broadcast for a new game, a restore or JumpToDay, nor by a calendar with
	 * no seasons. Telling the player is the game mode's job.
	 */
	FOnVestigeSeasonStarted OnSeasonStarted;

	/**
	 * Broadcast each time the running clock reaches a whole hour, with the hour since midnight at the start of the day: 22
	 * is 22:00, 24 is the next midnight. Once per hour even when a long frame passes several, after that hour's
	 * OnMinuteStarted. Not broadcast for the hour a new day starts at, nor when the clock is restored from a save. When a
	 * listener ends the day, the later hours and minutes aren't broadcast, but the other listeners of that hour still hear it,
	 * possibly in the new day. Listeners must not run the clock forward (AdvanceTo).
	 */
	FOnVestigeHourStarted OnHourStarted;

	/**
	 * Broadcast each time the running clock reaches a whole game minute, with the minutes since midnight at the start of the
	 * day: 1441 is 00:01 after midnight. Once per minute, in order, even when a long frame or AdvanceTo passes several; a
	 * minute that is also a whole hour comes before that hour's OnHourStarted. Something that goes on steadily over game time
	 * (the late-night drain) listens here. Not broadcast for the minute a new day starts at, nor when the clock is restored
	 * from a save. When a listener ends the day, the later minutes and hours aren't broadcast, but the other listeners of that
	 * minute still hear it, possibly in the new day. Listeners must not run the clock forward (AdvanceTo).
	 */
	FOnVestigeMinuteStarted OnMinuteStarted;

private:
	/** Sets the clock to NewTimeOfDay, no later than the day's end, and broadcasts the minutes and hours passed and the day's end. */
	void RunTo(float NewTimeOfDay);

	/**
	 * Broadcasts OnMinuteStarted for every whole minute after PreviousTimeOfDay, up to and including TimeOfDay, and
	 * OnHourStarted after each minute that is a whole hour. Stops early when a listener ends the day, so the new day never
	 * hears the old day's minutes and hours.
	 */
	void BroadcastTimeStartedSince(float PreviousTimeOfDay);

	/** Stops the clock at the current time, keeps how the day ended in LastDayEnd, and broadcasts OnDayEnded. */
	void EndDayNow();

	/** Keeps the current day and time in LastDayEnd, as the day that ended. */
	void RecordDayEnd();

	/** Logs and broadcasts OnSeasonStarted when today is the first day of a season. */
	void StartSeasonIfFirstDay();

	UPROPERTY()
	TObjectPtr<UVestigeSaveRegistry> SaveRegistry;

	// Read from UVestigeClockSettings in Initialize and kept for the game instance's lifetime.
	float RealSecondsPerGameHour = 40.f;
	float DayStartHour = 6.f;
	float DayEndHour = 30.f;
	float SunriseHour = 6.f;
	float SunsetHour = 20.f;

	/** Copied from UVestigeGameData at Initialize and kept for the game instance's lifetime. */
	FVestigeCalendar Calendar;

	int32 CurrentDay = 1;
	float TimeOfDay = 6.f;

	/** Set when the day ends, so OnDayEnded is broadcast once per day and the clock stops. */
	bool bDayEndReached = false;

	/** How the last day ended. Not saved: the next day starts in the same frame the day ends. */
	FVestigeDayEnd LastDayEnd;
};
