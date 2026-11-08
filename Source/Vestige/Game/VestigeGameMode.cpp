// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Game/VestigeGameMode.h"

#include "AbilitySystem/VestigeGameplayTags.h"
#include "Core/VestigeFeedbackSubsystem.h"
#include "Engine/World.h"
#include "Game/VestigePlayerController.h"
#include "Player/VestigePlayerState.h"
#include "Time/VestigeClockSubsystem.h"
#include "UI/VestigeHUD.h"

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

	// Reported first, so the reports of everything reacting to the new day, like waking exhausted and saving, follow it.
	const FGameplayTag DayTag = DayEnd.bEndedEarly ? VestigeGameplayTags::Feedback_Day_Slept : VestigeGameplayTags::Feedback_Day_StayedUp;
	UVestigeFeedbackSubsystem::Report(GetWorld(), DayTag, {{TEXT("Day"), DayEnd.Day + 1}});
	GameClock->StartNextDay();
}
