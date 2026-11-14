// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Game/VestigeGameMode.h"

#include "AbilitySystem/VestigeGameplayTags.h"
#include "Content/VestigeCalendar.h"
#include "Core/VestigeFeedbackSubsystem.h"
#include "Engine/World.h"
#include "Game/VestigePlayerController.h"
#include "Player/VestigePlayerState.h"
#include "Time/VestigeClockSubsystem.h"
#include "UI/VestigeHUD.h"

namespace VestigeGameModeReports
{
	/** The day report's tag for a night that ended as DayEnd did, before a day of Kind. */
	FGameplayTag GetDayTag(const FVestigeDayEnd& DayEnd, EVestigeDayKind Kind)
	{
		const bool bSlept = DayEnd.bEndedEarly;
		switch (Kind)
		{
		case EVestigeDayKind::NewSeason:
			return bSlept ? VestigeGameplayTags::Feedback_Day_Slept_NewSeason : VestigeGameplayTags::Feedback_Day_StayedUp_NewSeason;
		case EVestigeDayKind::LastDay:
			return bSlept ? VestigeGameplayTags::Feedback_Day_Slept_LastDay : VestigeGameplayTags::Feedback_Day_StayedUp_LastDay;
		case EVestigeDayKind::Usual:
			break;
		}
		return bSlept ? VestigeGameplayTags::Feedback_Day_Slept : VestigeGameplayTags::Feedback_Day_StayedUp;
	}
}

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

	// One report per morning: a new season is told inside it, not in a second message. Reported first, so the reports of
	// everything reacting to the new day, like waking exhausted and saving, follow it.
	const FVestigeCalendar& Calendar = GameClock->GetCalendar();
	const int32 NextDay = DayEnd.Day + 1;
	const EVestigeDayKind Kind = Calendar.GetDayKind(NextDay);
	FFormatNamedArguments Values =
		Kind == EVestigeDayKind::NewSeason ? Calendar.GetSeasonStartValues(NextDay) : Calendar.GetReportValues(NextDay);
	UVestigeFeedbackSubsystem::Report(GetWorld(), VestigeGameModeReports::GetDayTag(DayEnd, Kind), MoveTemp(Values));
	GameClock->StartNextDay();
}
