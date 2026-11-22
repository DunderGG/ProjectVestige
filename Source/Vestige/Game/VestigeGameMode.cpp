// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Game/VestigeGameMode.h"

#include "AbilitySystem/VestigeGameplayTags.h"
#include "Content/VestigeCalendar.h"
#include "Core/VestigeFeedbackSubsystem.h"
#include "Engine/World.h"
#include "Farming/FarmLog.h"
#include "Farming/FarmShippingSubsystem.h"
#include "Game/VestigePlayerController.h"
#include "Inventory/VestigeWalletComponent.h"
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

	/** The day report's Paid clause: what the night's sale paid, or an empty one (no tag) when it paid nothing. */
	FVestigeFeedbackClause MakePaidClause(int32 Paid)
	{
		FVestigeFeedbackClause Clause;
		Clause.Argument = TEXT("Paid");
		if (Paid > 0)
		{
			Clause.Tag = VestigeGameplayTags::Feedback_Shipping_Paid;
			Clause.Arguments.Add(TEXT("Amount"), Paid);
		}
		return Clause;
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

	// The night's sale comes first, so the day report can tell what the bin paid and the coins are in the morning's save.
	const int32 Paid = SellShipment();

	// One report per morning: a new season is told inside it, not in a second message. Reported first, so the reports of
	// everything reacting to the new day, like waking exhausted and saving, follow it.
	const FVestigeCalendar& Calendar = GameClock->GetCalendar();
	const int32 NextDay = DayEnd.Day + 1;
	const EVestigeDayKind Kind = Calendar.GetDayKind(NextDay);
	FFormatNamedArguments Values =
		Kind == EVestigeDayKind::NewSeason ? Calendar.GetSeasonStartValues(NextDay) : Calendar.GetReportValues(NextDay);
	FVestigeFeedbackReport DayReport{VestigeGameModeReports::GetDayTag(DayEnd, Kind), MoveTemp(Values), {}};
	DayReport.Clauses.Add(VestigeGameModeReports::MakePaidClause(Paid));
	UVestigeFeedbackSubsystem::Report(GetWorld(), MoveTemp(DayReport));
	GameClock->StartNextDay();
}

int32 AVestigeGameMode::SellShipment() const
{
	UFarmShippingSubsystem* Shipping = UFarmShippingSubsystem::Get(GetWorld());
	if (!Shipping)
	{
		return 0;
	}

	// Without a wallet (no player state, a test world) the shipment waits for the next night rather than selling to nobody.
	UVestigeWalletComponent* Wallet = UVestigeWalletComponent::FindForFirstPlayer(GetWorld());
	if (!Wallet)
	{
		if (!Shipping->GetShippedStacks().IsEmpty())
		{
			VESTIGE_LOG(this, LogFarm, Warning, "There is no wallet to pay, so the shipment waits for the next night.");
		}
		return 0;
	}
	return Shipping->SellShipment(*Wallet);
}
