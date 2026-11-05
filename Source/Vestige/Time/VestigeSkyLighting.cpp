// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Time/VestigeSkyLighting.h"

#include "Components/LightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Time/VestigeClockSettings.h"
#include "Time/VestigeClockSubsystem.h"

namespace VestigeSkyLightingMath
{
	constexpr float HoursPerDay = 24.f;

	/** Sun elevation at sunrise and sunset, in degrees: a little above the horizon, for long shadows rather than darkness. */
	constexpr float MinSunElevation = 5.f;

	/** How far the sun turns between sunrise and sunset, and the moon between sunset and sunrise, in degrees of yaw. */
	constexpr float SunArcYaw = 180.f;

	// How soon each effect reaches full strength as the sun or moon rises: 3 means a third of the way to its highest point.
	constexpr float DaylightFadeRate = 3.f;
	constexpr float WarmColorFadeRate = 2.f;
	constexpr float NightExposureFadeRate = 6.f;

	/** 0 at the horizon (Arc 0), rising to 1 once Arc reaches 1 / Rate. */
	float FadeIn(float Arc, float Rate)
	{
		return FMath::Clamp(Arc * Rate, 0.f, 1.f);
	}

	/** Where the sun is on its path across the sky by day, or the moon by night. */
	struct FSkyPosition
	{
		bool bDaytime = true;

		/** 0 when it rises to 1 when it sets. */
		float Progress = 0.f;

		/** 0 at the horizon to 1 at its highest point. */
		float Arc = 0.f;
	};

	/** The sun arcs across the sky from sunrise to sunset, and the moon takes the same path from sunset to sunrise. */
	FSkyPosition GetSkyPosition(float TimeOfDay, float SunriseHour, float SunsetHour)
	{
		const float Hour = FMath::Fmod(TimeOfDay, HoursPerDay);
		const float DayLength = SunsetHour - SunriseHour;
		const float HoursSinceSunset = FMath::Fmod(Hour - SunsetHour + HoursPerDay, HoursPerDay);

		FSkyPosition Position;
		Position.bDaytime = Hour >= SunriseHour && Hour < SunsetHour;
		Position.Progress = Position.bDaytime ? (Hour - SunriseHour) / DayLength : HoursSinceSunset / (HoursPerDay - DayLength);
		Position.Arc = FMath::Sin(Position.Progress * PI);
		return Position;
	}

	/** Where the sun or moon is at the clock's time, or at the start of the day without a clock (outside a game). */
	FSkyPosition GetSkyPosition(const UVestigeClockSubsystem* Clock)
	{
		if (!Clock)
		{
			const UVestigeClockSettings& Settings = UVestigeClockSettings::Get();
			return GetSkyPosition(Settings.GetDayStartHour(), Settings.GetSunriseHour(), Settings.GetSunsetHour());
		}
		return GetSkyPosition(Clock->GetTimeOfDay(), Clock->GetSunriseHour(), Clock->GetSunsetHour());
	}
}

AVestigeSkyLighting::AVestigeSkyLighting()
{
	// Ticks to follow the clock: brightness, colour and exposure change every frame, the sun turns in steps.
	PrimaryActorTick.bCanEverTick = true;

	NightPostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("NightPostProcess"));
	RootComponent = NightPostProcess;
	NightPostProcess->bUnbound = true;
	NightPostProcess->Settings.bOverride_AutoExposureBias = true;
	NightPostProcess->Settings.AutoExposureBias = 0.f;
}

void AVestigeSkyLighting::BeginPlay()
{
	Super::BeginPlay();

	Clock = UVestigeClockSubsystem::Get(GetWorld());
	FindLights();
	UpdateLighting();
}

void AVestigeSkyLighting::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateLighting();
}

void AVestigeSkyLighting::FindLights()
{
	if (!Sun)
	{
		TActorIterator<ADirectionalLight> It(GetWorld());
		Sun = It ? *It : nullptr;
	}
	if (!SkyLight)
	{
		TActorIterator<ASkyLight> It(GetWorld());
		SkyLight = It ? *It : nullptr;
	}

	if (Sun)
	{
		// Static or stationary lights can't be turned at runtime. Setting the light to Movable in the level avoids this.
		ULightComponent* SunLight = Sun->GetLightComponent();
		if (SunLight->Mobility != EComponentMobility::Movable)
		{
			SunLight->SetMobility(EComponentMobility::Movable);
		}
		SunYaw = Sun->GetActorRotation().Yaw;
		DaySunIntensity = SunLight->Intensity;
		DaySunColor = SunLight->GetLightColor();
	}
	if (SkyLight)
	{
		DaySkyIntensity = SkyLight->GetLightComponent()->Intensity;
	}
}

void AVestigeSkyLighting::UpdateLighting()
{
	if (!Sun)
	{
		return;
	}

	using namespace VestigeSkyLightingMath;

	TurnSunInSteps();

	// Light fades in after sunrise and out before sunset, warm near the horizon.
	const FSkyPosition Sky = GetSkyPosition(Clock.Get());
	const float Daylight = Sky.bDaytime ? FadeIn(Sky.Arc, DaylightFadeRate) : 0.f;
	const float Brightness = FMath::Lerp(NightLightFraction, 1.f, Daylight);
	ULightComponent* SunLight = Sun->GetLightComponent();
	SunLight->SetIntensity(DaySunIntensity * Brightness);
	SunLight->SetLightColor(Sky.bDaytime ? FMath::Lerp(SunriseColor, DaySunColor, FadeIn(Sky.Arc, WarmColorFadeRate)) : MoonColor);

	if (SkyLight)
	{
		SkyLight->GetLightComponent()->SetIntensity(DaySkyIntensity * Brightness);
	}

	// Darken the exposure over the first part of the night and ease it off again toward sunrise,
	// so daytime, mornings and evenings keep the normal exposure.
	const float Darkness = Sky.bDaytime ? 0.f : FadeIn(Sky.Arc, NightExposureFadeRate);
	NightPostProcess->Settings.AutoExposureBias = NightExposureBias * Darkness;
}

void AVestigeSkyLighting::TurnSunInSteps()
{
	// After a jump in the clock (a new day, a loaded save) the sun is usually far behind, so it follows on the next frame.
	const FQuat Target = GetSunRotation().Quaternion();
	const double DegreesBehind = FMath::RadiansToDegrees(Sun->GetActorQuat().AngularDistance(Target));
	if (DegreesBehind >= SunStepDegrees)
	{
		Sun->SetActorRotation(Target);
	}
}

FRotator AVestigeSkyLighting::GetSunRotation() const
{
	using namespace VestigeSkyLightingMath;

	const FSkyPosition Sky = GetSkyPosition(Clock.Get());
	const float Elevation = FMath::Lerp(MinSunElevation, MaxSunElevation, Sky.Arc);
	return FRotator(-Elevation, SunYaw + Sky.Progress * SunArcYaw, 0.f);
}
