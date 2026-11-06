// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Game/VestigeGameMode.h"

#include "Core/VestigeFeedbackSubsystem.h"
#include "Engine/World.h"
#include "Game/VestigePlayerController.h"
#include "Player/VestigePlayerState.h"
#include "Time/VestigeClockSubsystem.h"
#include "UI/VestigeHUD.h"

#define LOCTEXT_NAMESPACE "VestigeGameMode"

AVestigeGameMode::AVestigeGameMode()
{
	HUDClass = AVestigeHUD::StaticClass();
	PlayerStateClass = AVestigePlayerState::StaticClass();
	PlayerControllerClass = AVestigePlayerController::StaticClass();
}

void AVestigeGameMode::BeginPlay()
{
	Super::BeginPlay();

	Clock = UVestigeClockSubsystem::Get(GetWorld());
	if (UVestigeClockSubsystem* GameClock = Clock.Get())
	{
		GameClock->OnDayEnded.AddUObject(this, &AVestigeGameMode::StartNextDay);
	}
}

void AVestigeGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UVestigeClockSubsystem* GameClock = Clock.Get())
	{
		GameClock->OnDayEnded.RemoveAll(this);
	}
	Clock.Reset();
	Super::EndPlay(EndPlayReason);
}

void AVestigeGameMode::StartNextDay(const FVestigeDayEnd& DayEnd)
{
	UVestigeClockSubsystem* GameClock = Clock.Get();
	if (!GameClock || GameClock->IsRunning() || DayEnd.Day != GameClock->GetCurrentDay())
	{
		return;
	}

	// Raised first, so the messages of everything reacting to the new day, like waking exhausted and saving, follow it.
	const FText Format = DayEnd.bEndedEarly ? LOCTEXT("Slept", "You slept. Day {Day} begins.")
											: LOCTEXT("StayedUp", "You stayed up all night. Day {Day} begins.");
	UVestigeFeedbackSubsystem::Notify(GetWorld(), FText::FormatNamed(Format, TEXT("Day"), DayEnd.Day + 1));
	GameClock->StartNextDay();
}

#undef LOCTEXT_NAMESPACE
