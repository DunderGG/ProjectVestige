// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VestigeSkyLighting.generated.h"

class ADirectionalLight;
class ASkyLight;
class UPostProcessComponent;
class UVestigeClockSubsystem;

/**
 * The lighting of a level whose sky follows the time of day: turns the sun (the moon at night), dims the sky light and
 * darkens the exposure at night, to match the game's clock (UVestigeClockSubsystem). Presentation only: it reads the clock
 * and never changes it. Place one in each such level; a level without one (an interior) keeps its lights as placed. The sky
 * atmosphere and clouds are ordinary placed actors that follow the sun on their own.
 */
UCLASS()
class AVestigeSkyLighting : public AActor
{
	GENERATED_BODY()

public:
	AVestigeSkyLighting();

	/**
	 * Where the clock puts the sun now, or the moon at night. The sun itself turns to it in steps of SunStepDegrees, so
	 * it can be up to a step behind.
	 */
	FRotator GetSunRotation() const;

	/** Smallest turn of the sun, in degrees. */
	float GetSunStepDegrees() const { return SunStepDegrees; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	/** The clock's time of day, or the day's start hour without a clock (outside a game). */
	float GetTimeOfDay() const;

	void FindLights();
	void UpdateLighting();

	/** Turns the sun to GetSunRotation once it is at least SunStepDegrees away from it. */
	void TurnSunInSteps();

	/** Hour the sun rises above the horizon. Before it, the moon lights the farm. */
	UPROPERTY(EditAnywhere, Category = "Time", meta = (AllowPrivateAccess = "true", ClampMin = 0, ClampMax = 23))
	float SunriseHour = 6.f;

	/** Hour the sun sets. Must be later than SunriseHour. */
	UPROPERTY(EditAnywhere, Category = "Time", meta = (AllowPrivateAccess = "true", ClampMin = 1, ClampMax = 24))
	float SunsetHour = 20.f;

	/** Light moved with the time of day. Left empty, the first DirectionalLight in the level is used. */
	UPROPERTY(EditAnywhere, Category = "Lighting", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ADirectionalLight> Sun;

	/** Dimmed at night. Left empty, the first SkyLight in the level is used. */
	UPROPERTY(EditAnywhere, Category = "Lighting", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ASkyLight> SkyLight;

	/** Highest angle of the sun above the horizon at midday, in degrees. */
	UPROPERTY(EditAnywhere, Category = "Lighting", meta = (AllowPrivateAccess = "true", ClampMin = 10, ClampMax = 90))
	float MaxSunElevation = 65.f;

	/**
	 * Smallest turn of the sun or moon, in degrees: it waits until the clock has moved it this far. Virtual shadow maps
	 * keep the sun's shadows cached until its direction changes, then draw them all again, so a sun that turns every
	 * frame redraws them every frame. Brightness, color and exposure still change smoothly. 0 turns it every frame.
	 * 0.05 was tuned by eye at 40 real seconds per game hour: 0.1 and 0.25 show as small jumps of the shadows.
	 */
	UPROPERTY(EditAnywhere, Category = "Lighting", meta = (AllowPrivateAccess = "true", ClampMin = 0, ClampMax = 5))
	float SunStepDegrees = 0.05f;

	/** Share of the daytime light intensity left at night, for moonlight. */
	UPROPERTY(EditAnywhere, Category = "Lighting", meta = (AllowPrivateAccess = "true", ClampMin = 0, ClampMax = 1))
	float NightLightFraction = 0.03f;

	/**
	 * Exposure change at night, in stops. Auto-exposure brightens dark scenes back up,
	 * so this pulls it down again. More negative is darker; 0 turns it off.
	 */
	UPROPERTY(EditAnywhere, Category = "Lighting", meta = (AllowPrivateAccess = "true", ClampMin = -8, ClampMax = 0))
	float NightExposureBias = -2.5f;

	/** Sun color near the horizon. It blends to the sun's color as placed in the level as the sun rises. */
	UPROPERTY(EditAnywhere, Category = "Lighting", meta = (AllowPrivateAccess = "true"))
	FLinearColor SunriseColor = FLinearColor(1.f, 0.55f, 0.3f);

	/** Color of the directional light at night, when it stands in for the moon. */
	UPROPERTY(EditAnywhere, Category = "Lighting", meta = (AllowPrivateAccess = "true"))
	FLinearColor MoonColor = FLinearColor(0.5f, 0.6f, 1.f);

	/** Covers the whole level and darkens the exposure at night. */
	UPROPERTY(VisibleAnywhere, Category = "Lighting")
	TObjectPtr<UPostProcessComponent> NightPostProcess;

	/** The clock the lighting follows, found at BeginPlay. */
	TWeakObjectPtr<const UVestigeClockSubsystem> Clock;

	// The lights' settings as placed in the level, used as the daytime values.
	float SunYaw = 0.f;
	float DaySunIntensity = 0.f;
	FLinearColor DaySunColor = FLinearColor::White;
	float DaySkyIntensity = 0.f;
};
