// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Time/VestigeDayCycle.h"

#include "Components/LightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SkyLightComponent.h"
#include "Core/VestigeActorLookup.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Engine.h"
#include "Engine/SkyLight.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "Time/VestigeClockSaveData.h"
#include "Time/VestigeTimeLog.h"

namespace VestigeDayCycleLighting
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
}

AVestigeDayCycle::AVestigeDayCycle()
{
	// Ticks to run the clock and move the sun. Stops while the clock is stopped at the end of the day.
	PrimaryActorTick.bCanEverTick = true;

	NightPostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("NightPostProcess"));
	RootComponent = NightPostProcess;
	NightPostProcess->bUnbound = true;
	NightPostProcess->Settings.bOverride_AutoExposureBias = true;
	NightPostProcess->Settings.AutoExposureBias = 0.f;
}

AVestigeDayCycle* AVestigeDayCycle::Get(UWorld* World)
{
	check(IsInGameThread());
	if (!World)
	{
		VESTIGE_LOG(nullptr, LogVestigeTime, Error, "Called without a world.");
		return nullptr;
	}

	if (AVestigeDayCycle* Existing = Find(World))
	{
		return Existing;
	}

	// Actors' BeginPlay runs before the world counts as begun, so "running" means initialized for play and not ending.
	const bool bRunningGameWorld = World->IsGameWorld() && World->AreActorsInitialized() && !World->bIsTearingDown;
	if (!bRunningGameWorld)
	{
		return nullptr;
	}

	VESTIGE_LOG(World, LogVestigeTime, Warning,
		"The level has no VestigeDayCycle, so one was spawned with default settings. Place one to tune it.");
	return World->SpawnActor<AVestigeDayCycle>();
}

AVestigeDayCycle* AVestigeDayCycle::Find(const UWorld* World)
{
	return Cast<AVestigeDayCycle>(VestigeActorLookup::FindSingle(World, StaticClass()));
}

AVestigeDayCycle* AVestigeDayCycle::GetDayCycle(const UObject* WorldContextObject)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull) : nullptr;
	return Get(World);
}

void AVestigeDayCycle::BeginPlay()
{
	Super::BeginPlay();

	TimeOfDay = DayStartHour;
	FindLights();
	UpdateLighting();
}

void AVestigeDayCycle::StartNextDay()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(AVestigeDayCycle::StartNextDay);

	++CurrentDay;
	TimeOfDay = DayStartHour;
	bDayEndReached = false;
	SetActorTickEnabled(true);
	UpdateLighting();
	OnNewDay.Broadcast(CurrentDay);
	OnNewDayBlueprint.Broadcast(CurrentDay);
	OnNewDayFinished.Broadcast(CurrentDay);
}

FVestigeClockSaveData AVestigeDayCycle::GetSaveData() const
{
	FVestigeClockSaveData Data;
	Data.Day = CurrentDay;
	Data.TimeOfDay = TimeOfDay;
	return Data;
}

void AVestigeDayCycle::RestoreFromSaveData(const FVestigeClockSaveData& Data)
{
	CurrentDay = FMath::Max(1, Data.Day);
	TimeOfDay = FMath::Clamp(Data.TimeOfDay, DayStartHour, DayEndHour);
	bDayEndReached = false;

	// A clock restored at DayEndHour reaches the end on the next tick, so OnDayEndReached isn't lost.
	SetActorTickEnabled(true);
	UpdateLighting();
}

void AVestigeDayCycle::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	TimeOfDay = FMath::Min(TimeOfDay + DeltaSeconds / RealSecondsPerGameHour, DayEndHour);
	UpdateLighting();
	if (TimeOfDay < DayEndHour || bDayEndReached)
	{
		return;
	}

	// The clock stops at DayEndHour until the next day starts, and nothing changes meanwhile. Stop ticking first,
	// so a listener that starts the next day turns it back on.
	bDayEndReached = true;
	SetActorTickEnabled(false);
	OnDayEndReached.Broadcast(CurrentDay);
}

void AVestigeDayCycle::FindLights()
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

void AVestigeDayCycle::UpdateLighting()
{
	if (!Sun)
	{
		return;
	}

	using namespace VestigeDayCycleLighting;

	// Progress runs 0 to 1 from sunrise to sunset by day, and from sunset to the next sunrise by night.
	// The sun arcs across the sky by day and the moon takes the same path by night.
	const float Hour = FMath::Fmod(TimeOfDay, HoursPerDay);
	const float DayLength = SunsetHour - SunriseHour;
	const bool bDaytime = Hour >= SunriseHour && Hour < SunsetHour;
	const float HoursSinceSunset = FMath::Fmod(Hour - SunsetHour + HoursPerDay, HoursPerDay);
	const float Progress = bDaytime ? (Hour - SunriseHour) / DayLength : HoursSinceSunset / (HoursPerDay - DayLength);
	const float Arc = FMath::Sin(Progress * PI);

	const float Elevation = FMath::Lerp(MinSunElevation, MaxSunElevation, Arc);
	Sun->SetActorRotation(FRotator(-Elevation, SunYaw + Progress * SunArcYaw, 0.f));

	// Light fades in after sunrise and out before sunset, warm near the horizon.
	const float Daylight = bDaytime ? FadeIn(Arc, DaylightFadeRate) : 0.f;
	const float Brightness = FMath::Lerp(NightLightFraction, 1.f, Daylight);
	ULightComponent* SunLight = Sun->GetLightComponent();
	SunLight->SetIntensity(DaySunIntensity * Brightness);
	SunLight->SetLightColor(bDaytime ? FMath::Lerp(SunriseColor, DaySunColor, FadeIn(Arc, WarmColorFadeRate)) : MoonColor);

	if (SkyLight)
	{
		SkyLight->GetLightComponent()->SetIntensity(DaySkyIntensity * Brightness);
	}

	// Darken the exposure over the first part of the night and ease it off again toward sunrise,
	// so daytime, mornings and evenings keep the normal exposure.
	const float Darkness = bDaytime ? 0.f : FadeIn(Arc, NightExposureFadeRate);
	NightPostProcess->Settings.AutoExposureBias = NightExposureBias * Darkness;
}
