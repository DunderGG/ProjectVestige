// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "UI/FarmHUD.h"

#include "Content/FarmItemData.h"
#include "Content/FarmToolData.h"
#include "Core/FarmFeedbackSubsystem.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Farming/FarmingComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Inventory/VestigeInventoryComponent.h"
#include "Player/FarmerCharacter.h"
#include "Time/FarmDayCycle.h"
#include "TimerManager.h"
#include "UI/FarmHudSettings.h"
#include "UI/FarmHudWidget.h"

#define LOCTEXT_NAMESPACE "FarmHUD"

namespace FarmHudClock
{
	constexpr int32 MinutesPerHour = 60;
	constexpr int32 HoursPerDay = 24;

	/** The clock moves in steps of this many minutes, so it doesn't flicker every game minute. */
	constexpr int32 StepMinutes = 10;

	/** Hours since midnight, rounded down to the clock's step, in minutes. */
	int32 ToShownMinutes(float Hours)
	{
		return FMath::FloorToInt(Hours * MinutesPerHour / StepMinutes) * StepMinutes;
	}

	/** A 24-hour clock, like "18:40". Minutes past midnight of the next day wrap around to 00:00. */
	FText Format(int32 Minutes)
	{
		FNumberFormattingOptions TwoDigits;
		TwoDigits.SetMinimumIntegralDigits(2).SetUseGrouping(false);
		return FText::FormatNamed(LOCTEXT("Clock", "{Hours}:{Minutes}"), TEXT("Hours"),
			FText::AsNumber((Minutes / MinutesPerHour) % HoursPerDay, &TwoDigits), TEXT("Minutes"),
			FText::AsNumber(Minutes % MinutesPerHour, &TwoDigits));
	}
}

namespace FarmHudHints
{
	FText Format(const FFarmKeyHint& Hint)
	{
		return FText::FormatNamed(LOCTEXT("KeyHint", "{Key} {Action}"), TEXT("Key"), Hint.Key, TEXT("Action"), Hint.Action);
	}
}

void AFarmHUD::BeginPlay()
{
	Super::BeginPlay();

	const UFarmHudSettings& Settings = UFarmHudSettings::Get();
	MaxMessages = Settings.GetMaxMessages();
	MessageSeconds = Settings.GetMessageSeconds();

	UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
	if (!PlayerOwner || !PlayerOwner->IsLocalController() || !Viewport)
	{
		return;
	}

	DayCycle = AFarmDayCycle::Get(GetWorld());
	PlayerOwner->OnPossessedPawnChanged.AddDynamic(this, &AFarmHUD::FollowPawn);
	FollowPawn(nullptr, PlayerOwner->GetPawn());
	FollowInventory();
	if (UFarmFeedbackSubsystem* Feedback = GetWorld()->GetSubsystem<UFarmFeedbackSubsystem>())
	{
		Feedback->OnFeedback.AddUObject(this, &AFarmHUD::ShowMessage);
	}

	HudWidget = SNew(SFarmHudWidget).Hud(this);
	Viewport->AddViewportWidgetContent(HudWidget.ToSharedRef());
}

void AFarmHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (PlayerOwner)
	{
		PlayerOwner->OnPossessedPawnChanged.RemoveDynamic(this, &AFarmHUD::FollowPawn);
	}
	StopFollowingFarming();
	if (UVestigeInventoryComponent* CurrentInventory = Inventory.Get())
	{
		CurrentInventory->OnInventoryChanged.RemoveAll(this);
	}
	if (UFarmFeedbackSubsystem* Feedback = UWorld::GetSubsystem<UFarmFeedbackSubsystem>(GetWorld()))
	{
		Feedback->OnFeedback.RemoveAll(this);
	}
	if (HudWidget)
	{
		if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
		{
			Viewport->RemoveViewportWidgetContent(HudWidget.ToSharedRef());
		}
		HudWidget.Reset();
	}
	Super::EndPlay(EndPlayReason);
}

FText AFarmHUD::GetStatusText() const
{
	const AFarmDayCycle* Cycle = DayCycle.Get();
	if (!Cycle)
	{
		return FText::GetEmpty();
	}

	const int32 Day = Cycle->GetCurrentDay();
	const int32 ClockMinutes = FarmHudClock::ToShownMinutes(Cycle->GetTimeOfDay());
	if (Day != StatusDay || ClockMinutes != StatusClockMinutes)
	{
		StatusDay = Day;
		StatusClockMinutes = ClockMinutes;
		StatusText = FText::FormatNamed(LOCTEXT("Status", "Day {Day}   {Clock}   |   {Tool}"), TEXT("Day"), Day, TEXT("Clock"),
			FarmHudClock::Format(ClockMinutes), TEXT("Tool"), ToolText);
	}
	return StatusText;
}

void AFarmHUD::FollowPawn(APawn* OldPawn, APawn* NewPawn)
{
	StopFollowingFarming();
	Farmer = Cast<AFarmerCharacter>(NewPawn);
	Farming = NewPawn ? NewPawn->FindComponentByClass<UFarmingComponent>() : nullptr;

	UFarmingComponent* NewFarming = Farming.Get();
	if (NewFarming)
	{
		NewFarming->OnToolChanged.AddUObject(this, &AFarmHUD::RefreshToolTexts);
	}
	RefreshToolTexts(NewFarming ? NewFarming->GetCurrentTool() : nullptr);
}

void AFarmHUD::StopFollowingFarming()
{
	if (UFarmingComponent* OldFarming = Farming.Get())
	{
		OldFarming->OnToolChanged.RemoveAll(this);
	}
}

void AFarmHUD::FollowInventory()
{
	const APlayerState* PlayerState = PlayerOwner ? PlayerOwner->PlayerState.Get() : nullptr;
	Inventory = PlayerState ? PlayerState->FindComponentByClass<UVestigeInventoryComponent>() : nullptr;
	if (UVestigeInventoryComponent* CurrentInventory = Inventory.Get())
	{
		CurrentInventory->OnInventoryChanged.AddUObject(this, &AFarmHUD::RefreshInventoryText);
	}
	RefreshInventoryText();
}

void AFarmHUD::RefreshToolTexts(const UFarmToolData* Tool)
{
	ToolText = Tool ? Tool->GetDisplayName() : LOCTEXT("NoTool", "No tool");

	// The status line shows the tool, so it is built again on its next read.
	StatusDay = INDEX_NONE;
	RefreshControlsText();
}

void AFarmHUD::RefreshInventoryText()
{
	const UVestigeInventoryComponent* CurrentInventory = Inventory.Get();
	if (!CurrentInventory)
	{
		InventoryText = FText::GetEmpty();
		return;
	}

	TArray<FText> Stacks;
	for (const FVestigeInventorySlot& Slot : CurrentInventory->GetSlots())
	{
		if (!Slot.IsEmpty())
		{
			Stacks.Add(FText::FormatNamed(
				LOCTEXT("Stack", "{Item} {Count}"), TEXT("Item"), Slot.Item->GetDisplayName(), TEXT("Count"), Slot.Count));
		}
	}
	const FText Contents = Stacks.IsEmpty() ? LOCTEXT("BagEmpty", "empty") : FText::Join(FText::AsCultureInvariant(TEXT("   ")), Stacks);
	InventoryText = FText::FormatNamed(LOCTEXT("Inventory", "Bag: {Contents}"), TEXT("Contents"), Contents);
}

void AFarmHUD::RefreshControlsText()
{
	const AFarmerCharacter* CurrentFarmer = Farmer.Get();
	if (!CurrentFarmer)
	{
		ControlsText = FText::GetEmpty();
		return;
	}

	TArray<FText> ToolbarHints;
	for (const FFarmKeyHint& Hint : CurrentFarmer->GetToolbarKeyHints())
	{
		ToolbarHints.Add(FarmHudHints::Format(Hint));
	}

	// The toolbar hints belong together; the other hints are set apart from them and from each other.
	TArray<FText> Groups;
	if (!ToolbarHints.IsEmpty())
	{
		Groups.Add(FText::Join(FText::AsCultureInvariant(TEXT("   ")), ToolbarHints));
	}
	for (const FFarmKeyHint& Hint : CurrentFarmer->GetActionKeyHints())
	{
		Groups.Add(FarmHudHints::Format(Hint));
	}
	ControlsText = FText::Join(FText::AsCultureInvariant(TEXT("   |   ")), Groups);
}

void AFarmHUD::ShowMessage(const FText& Message)
{
	Messages.Add(Message, GetWorld()->GetTimeSeconds(), MaxMessages);
	MessagesChanged();
}

void AFarmHUD::RemoveExpiredMessages()
{
	Messages.RemoveExpired(GetWorld()->GetTimeSeconds(), MessageSeconds);
	MessagesChanged();
}

void AFarmHUD::MessagesChanged()
{
	if (HudWidget)
	{
		HudWidget->RefreshMessages();
	}

	// Every message lives as long, so one timer for the oldest is enough; it is set again each time the list changes.
	const TOptional<double> SecondsLeft = Messages.GetSecondsUntilNextExpiry(GetWorld()->GetTimeSeconds(), MessageSeconds);
	if (!SecondsLeft.IsSet())
	{
		GetWorldTimerManager().ClearTimer(ExpireTimer);
		return;
	}

	// A timer with no time left would never fire, so it waits at least a frame.
	constexpr float MinimumWaitSeconds = 0.001f;
	GetWorldTimerManager().SetTimer(
		ExpireTimer, this, &AFarmHUD::RemoveExpiredMessages, FMath::Max(MinimumWaitSeconds, static_cast<float>(SecondsLeft.GetValue())));
}

float AFarmHUD::GetMessageOpacity(int32 Index) const
{
	const TArray<FFarmHudMessage>& Shown = Messages.GetMessages();
	if (!Shown.IsValidIndex(Index) || !GetWorld())
	{
		return 0.f;
	}
	return FFarmHudMessages::GetOpacity(Shown[Index].StartTime, GetWorld()->GetTimeSeconds(), MessageSeconds);
}

#undef LOCTEXT_NAMESPACE
