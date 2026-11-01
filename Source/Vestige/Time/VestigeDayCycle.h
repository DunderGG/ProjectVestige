// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VestigeDayCycle.generated.h"

class ADirectionalLight;
class ASkyLight;
class UPostProcessComponent;
struct FVestigeClockSaveData;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnVestigeNewDay, int32 /* Day */);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVestigeNewDayBlueprint, int32, Day);

/**
 * Keeps the day counter and the clock, and moves the sun, sky light and exposure to match the time.
 * Place one in the level to tweak its settings. If there is none, one is spawned with default settings.
 */
UCLASS()
class AVestigeDayCycle : public AActor
{
	GENERATED_BODY()

public:
	AVestigeDayCycle();

	/**
	 * Returns the world's day cycle, spawning one with a warning if a running game world has none.
	 * The only code that spawns a day cycle. Returns nullptr without spawning in editor and preview worlds,
	 * before the world is initialized for play and while it is torn down. Game thread only.
	 */
	static AVestigeDayCycle* Get(UWorld* World);

	/**
	 * Returns the world's day cycle, or nullptr if there is none yet. Never spawns one.
	 * Searches the world, so call it once and keep the result.
	 */
	static AVestigeDayCycle* Find(const UWorld* World);

	/**
	 * Extension point: the world's day cycle for Blueprints, from Get, so a level that has none still gets one. Bind to
	 * its On New Day event to react to a new day.
	 */
	UFUNCTION(BlueprintCallable, Category = "Farming|Time", meta = (WorldContext = "WorldContextObject", DisplayName = "Get Day Cycle"))
	static AVestigeDayCycle* GetDayCycle(const UObject* WorldContextObject);

	/** Ends the current day and sets the clock to the next morning, then broadcasts OnNewDay and OnNewDayBlueprint. */
	void StartNextDay();

	/** The day and clock, for saving. */
	FVestigeClockSaveData GetSaveData() const;

	/**
	 * Sets the day and clock from a save without broadcasting anything. The day becomes at least 1 and the clock is
	 * clamped between DayStartHour and DayEndHour; a clock at DayEndHour still broadcasts OnDayEndReached on its next tick.
	 */
	void RestoreFromSaveData(const FVestigeClockSaveData& Data);

	int32 GetCurrentDay() const { return CurrentDay; }

	/** Hours since midnight at the start of the current day. Goes past 24 after midnight, up to DayEndHour. */
	float GetTimeOfDay() const { return TimeOfDay; }

	/** Hour each day starts at after sleeping. */
	float GetDayStartHour() const { return DayStartHour; }

	/** Hour the day ends and the clock stops. Past 24 means after midnight. */
	float GetDayEndHour() const { return DayEndHour; }

	float GetRealSecondsPerGameHour() const { return RealSecondsPerGameHour; }

	/** Broadcast with the new day's number every time a new day starts. The farm reacts here, e.g. crops grow. */
	FOnVestigeNewDay OnNewDay;

	/**
	 * Extension point: OnNewDay for Blueprints, with the new day's number. Broadcast after OnNewDay, so the farm has
	 * reacted (crops have grown), and before OnNewDayFinished, so what a listener changes is saved. The same contract as
	 * OnNewDay: finish every change to saved state in the handler (Architecture.md, section 15).
	 */
	UPROPERTY(BlueprintAssignable, Category = "Farming|Time", meta = (DisplayName = "On New Day"))
	FOnVestigeNewDayBlueprint OnNewDayBlueprint;

	/** Broadcast after OnNewDay and OnNewDayBlueprint, once everything has reacted to the new day. Saving listens here. */
	FOnVestigeNewDay OnNewDayFinished;

	/**
	 * Broadcast once per day, with the day's number, when the clock reaches DayEndHour. The clock then stops until
	 * StartNextDay. Whoever owns the end of the player's day (UVestigeRestComponent) reacts here.
	 */
	FOnVestigeNewDay OnDayEndReached;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void FindLights();
	void UpdateLighting();

	/** Real seconds per in-game hour. */
	UPROPERTY(EditAnywhere, Category = "Farming|Time", meta = (AllowPrivateAccess = "true", ClampMin = 0.1))
	float RealSecondsPerGameHour = 40.f;

	/** Hour each day starts at after sleeping. */
	UPROPERTY(EditAnywhere, Category = "Farming|Time", meta = (AllowPrivateAccess = "true", ClampMin = 0, ClampMax = 23))
	float DayStartHour = 6.f;

	/** Hour the day ends and the clock stops: OnDayEndReached is broadcast. 26 is 02:00 the next morning. */
	UPROPERTY(EditAnywhere, Category = "Farming|Time", meta = (AllowPrivateAccess = "true", ClampMin = 1, ClampMax = 30))
	float DayEndHour = 26.f;

	/** Hour the sun rises above the horizon. Before it, the moon lights the farm. */
	UPROPERTY(EditAnywhere, Category = "Farming|Time", meta = (AllowPrivateAccess = "true", ClampMin = 0, ClampMax = 23))
	float SunriseHour = 6.f;

	/** Hour the sun sets. Must be later than SunriseHour. */
	UPROPERTY(EditAnywhere, Category = "Farming|Time", meta = (AllowPrivateAccess = "true", ClampMin = 1, ClampMax = 24))
	float SunsetHour = 20.f;

	/** Light moved with the time of day. Left empty, the first DirectionalLight in the level is used. */
	UPROPERTY(EditAnywhere, Category = "Farming|Lighting", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ADirectionalLight> Sun;

	/** Dimmed at night. Left empty, the first SkyLight in the level is used. */
	UPROPERTY(EditAnywhere, Category = "Farming|Lighting", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ASkyLight> SkyLight;

	/** Highest angle of the sun above the horizon at midday, in degrees. */
	UPROPERTY(EditAnywhere, Category = "Farming|Lighting", meta = (AllowPrivateAccess = "true", ClampMin = 10, ClampMax = 90))
	float MaxSunElevation = 65.f;

	/** Share of the daytime light intensity left at night, for moonlight. */
	UPROPERTY(EditAnywhere, Category = "Farming|Lighting", meta = (AllowPrivateAccess = "true", ClampMin = 0, ClampMax = 1))
	float NightLightFraction = 0.03f;

	/**
	 * Exposure change at night, in stops. Auto-exposure brightens dark scenes back up,
	 * so this pulls it down again. More negative is darker; 0 turns it off.
	 */
	UPROPERTY(EditAnywhere, Category = "Farming|Lighting", meta = (AllowPrivateAccess = "true", ClampMin = -8, ClampMax = 0))
	float NightExposureBias = -2.5f;

	/** Sun color near the horizon. It blends to the sun's color as placed in the level as the sun rises. */
	UPROPERTY(EditAnywhere, Category = "Farming|Lighting", meta = (AllowPrivateAccess = "true"))
	FLinearColor SunriseColor = FLinearColor(1.f, 0.55f, 0.3f);

	/** Color of the directional light at night, when it stands in for the moon. */
	UPROPERTY(EditAnywhere, Category = "Farming|Lighting", meta = (AllowPrivateAccess = "true"))
	FLinearColor MoonColor = FLinearColor(0.5f, 0.6f, 1.f);

	/** Covers the whole level and darkens the exposure at night. */
	UPROPERTY(VisibleAnywhere, Category = "Farming")
	TObjectPtr<UPostProcessComponent> NightPostProcess;

	int32 CurrentDay = 1;
	float TimeOfDay = DayStartHour;

	/** Set when OnDayEndReached is broadcast, so it is broadcast once per day. */
	bool bDayEndReached = false;

	// The lights' settings as placed in the level, used as the daytime values.
	float SunYaw = 0.f;
	float DaySunIntensity = 0.f;
	FLinearColor DaySunColor = FLinearColor::White;
	float DaySkyIntensity = 0.f;
};
