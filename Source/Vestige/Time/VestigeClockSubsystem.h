// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "VestigeClockSubsystem.generated.h"

struct FVestigeClockSaveData;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnVestigeNewDay, int32 /* Day */);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVestigeNewDayBlueprint, int32, Day);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnVestigeHourStarted, int32 /* Hour */);

/**
 * The game's time: the day counter and the clock, the one owner of game time (Systems.md, section 4). A game-instance
 * subsystem, so the clock keeps its time across map changes and runs in whichever game map is open. It advances with that
 * world's time, so pausing stops it and time dilation (slomo) speeds it up, and it stands still while a map loads. Its
 * speed and hours come from UVestigeClockSettings. Presentation (the sun, the HUD) reads it; nothing else keeps a clock.
 */
UCLASS(BlueprintType, meta = (DisplayName = "Vestige Clock"))
class UVestigeClockSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	/** Ticking starts in Initialize, once the subsystem is set up, as FTickableGameObject requires of UObjects. */
	UVestigeClockSubsystem();

	/** The clock of World's game instance, or nullptr in a world without one (editor and preview worlds). */
	static UVestigeClockSubsystem* Get(const UWorld* World);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Ends the current day and sets the clock to the next morning, then broadcasts OnNewDay, OnNewDayBlueprint and OnNewDayFinished. */
	void StartNextDay();

	/** Puts the clock back to the morning of day 1 for a new game, without broadcasting anything. */
	void StartNewGame();

	/** The day and clock, for saving. */
	FVestigeClockSaveData GetSaveData() const;

	/**
	 * Sets the day and clock from a save without broadcasting anything. The day becomes at least 1 and the clock is
	 * clamped between the day's start and end; a clock at the day's end still broadcasts OnDayEndReached on its next tick.
	 */
	void RestoreFromSaveData(const FVestigeClockSaveData& Data);

	int32 GetCurrentDay() const { return CurrentDay; }

	/** Hours since midnight at the start of the current day. Goes past 24 after midnight, up to GetDayEndHour. */
	float GetTimeOfDay() const { return TimeOfDay; }

	/** Hour each day starts at after sleeping. */
	float GetDayStartHour() const { return DayStartHour; }

	/** Hour the day ends and the clock stops. Past 24 means after midnight. */
	float GetDayEndHour() const { return DayEndHour; }

	float GetRealSecondsPerGameHour() const { return RealSecondsPerGameHour; }

	/** False once the clock has stopped at the end of the day, until the next day starts. */
	bool IsRunning() const { return !bDayEndReached; }

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
	 * Broadcast once per day, with the day's number, when the clock reaches the day's end. The clock then stops until
	 * StartNextDay. Whoever owns the end of the player's day (UVestigeRestComponent) reacts here.
	 */
	FOnVestigeNewDay OnDayEndReached;

	/**
	 * Broadcast each time the running clock reaches a whole hour, with the hour since midnight at the start of the day: 22
	 * is 22:00, 24 is the next midnight. Once per hour even when a long frame passes several. Not broadcast for the hour a
	 * new day starts at, nor when the clock is restored from a save.
	 */
	FOnVestigeHourStarted OnHourStarted;

private:
	/** Broadcasts OnHourStarted for every whole hour after PreviousTimeOfDay, up to and including TimeOfDay. */
	void BroadcastHoursStartedSince(float PreviousTimeOfDay);

	// Read from UVestigeClockSettings in Initialize and kept for the game instance's lifetime.
	float RealSecondsPerGameHour = 40.f;
	float DayStartHour = 6.f;
	float DayEndHour = 30.f;

	int32 CurrentDay = 1;
	float TimeOfDay = 6.f;

	/** Set when OnDayEndReached is broadcast, so it is broadcast once per day and the clock stops. */
	bool bDayEndReached = false;
};
