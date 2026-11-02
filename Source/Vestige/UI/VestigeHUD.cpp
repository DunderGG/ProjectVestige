// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "UI/VestigeHUD.h"

#include "AbilitySystem/VestigeGameplayTags.h"
#include "AbilitySystem/VestigeStaminaSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Content/VestigeItemData.h"
#include "Content/VestigeToolData.h"
#include "Core/VestigeFeedbackSubsystem.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Farming/FarmingComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameplayTagContainer.h"
#include "Interaction/VestigeInteractionComponent.h"
#include "Inventory/VestigeInventoryComponent.h"
#include "Player/VestigePlayerCharacter.h"
#include "Time/VestigeDayCycle.h"
#include "TimerManager.h"
#include "UI/VestigeHudSettings.h"
#include "UI/VestigeHudWidget.h"

#define LOCTEXT_NAMESPACE "VestigeHUD"

namespace VestigeHudClock
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

namespace VestigeHudHints
{
	FText Format(const FVestigeKeyHint& Hint)
	{
		return FText::FormatNamed(LOCTEXT("KeyHint", "{Key} {Action}"), TEXT("Key"), Hint.Key, TEXT("Action"), Hint.Action);
	}
}

void AVestigeHUD::BeginPlay()
{
	Super::BeginPlay();

	const UVestigeHudSettings& Settings = UVestigeHudSettings::Get();
	MaxMessages = Settings.GetMaxMessages();
	MessageSeconds = Settings.GetMessageSeconds();

	UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
	if (!PlayerOwner || !PlayerOwner->IsLocalController() || !Viewport)
	{
		return;
	}

	DayCycle = AVestigeDayCycle::Get(GetWorld());
	PlayerOwner->OnPossessedPawnChanged.AddDynamic(this, &AVestigeHUD::FollowPawn);
	FollowPawn(nullptr, PlayerOwner->GetPawn());
	FollowInventory();
	FollowAbilitySystem();
	if (UVestigeFeedbackSubsystem* Feedback = GetWorld()->GetSubsystem<UVestigeFeedbackSubsystem>())
	{
		Feedback->OnFeedback.AddUObject(this, &AVestigeHUD::ShowMessage);
	}

	HudWidget = SNew(SVestigeHudWidget).Hud(this);
	Viewport->AddViewportWidgetContent(HudWidget.ToSharedRef());
}

void AVestigeHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (PlayerOwner)
	{
		PlayerOwner->OnPossessedPawnChanged.RemoveDynamic(this, &AVestigeHUD::FollowPawn);
	}
	StopFollowingPawn();
	if (UVestigeInventoryComponent* CurrentInventory = Inventory.Get())
	{
		CurrentInventory->OnInventoryChanged.RemoveAll(this);
	}
	if (UAbilitySystemComponent* CurrentAbilitySystem = AbilitySystem.Get())
	{
		CurrentAbilitySystem->GetGameplayAttributeValueChangeDelegate(UVestigeStaminaSet::GetStaminaAttribute()).RemoveAll(this);
		CurrentAbilitySystem->GetGameplayAttributeValueChangeDelegate(UVestigeStaminaSet::GetMaxStaminaAttribute()).RemoveAll(this);
		CurrentAbilitySystem->RegisterGameplayTagEvent(VestigeGameplayTags::State_OutOfStamina).RemoveAll(this);
	}
	if (UVestigeFeedbackSubsystem* Feedback = UWorld::GetSubsystem<UVestigeFeedbackSubsystem>(GetWorld()))
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

FText AVestigeHUD::GetStatusText() const
{
	const AVestigeDayCycle* Cycle = DayCycle.Get();
	if (!Cycle)
	{
		return FText::GetEmpty();
	}

	const int32 Day = Cycle->GetCurrentDay();
	const int32 ClockMinutes = VestigeHudClock::ToShownMinutes(Cycle->GetTimeOfDay());
	if (Day != StatusDay || ClockMinutes != StatusClockMinutes)
	{
		StatusDay = Day;
		StatusClockMinutes = ClockMinutes;
		StatusText = FText::FormatNamed(LOCTEXT("Status", "Day {Day}   {Clock}   |   {Tool}"), TEXT("Day"), Day, TEXT("Clock"),
			VestigeHudClock::Format(ClockMinutes), TEXT("Tool"), ToolText);
	}
	return StatusText;
}

void AVestigeHUD::FollowPawn(APawn* OldPawn, APawn* NewPawn)
{
	StopFollowingPawn();
	Farmer = Cast<AVestigePlayerCharacter>(NewPawn);
	Farming = NewPawn ? NewPawn->FindComponentByClass<UFarmingComponent>() : nullptr;

	UFarmingComponent* NewFarming = Farming.Get();
	if (NewFarming)
	{
		NewFarming->OnToolChanged.AddUObject(this, &AVestigeHUD::RefreshToolTexts);
	}
	RefreshToolTexts(NewFarming ? NewFarming->GetCurrentTool() : nullptr);

	Interaction = NewPawn ? NewPawn->FindComponentByClass<UVestigeInteractionComponent>() : nullptr;
	if (UVestigeInteractionComponent* NewInteraction = Interaction.Get())
	{
		NewInteraction->OnFocusChanged.AddUObject(this, &AVestigeHUD::RefreshInteractionText);
	}
	RefreshInteractionText();
}

void AVestigeHUD::StopFollowingPawn()
{
	if (UFarmingComponent* OldFarming = Farming.Get())
	{
		OldFarming->OnToolChanged.RemoveAll(this);
	}
	if (UVestigeInteractionComponent* OldInteraction = Interaction.Get())
	{
		OldInteraction->OnFocusChanged.RemoveAll(this);
	}
}

void AVestigeHUD::RefreshInteractionText()
{
	const UVestigeInteractionComponent* CurrentInteraction = Interaction.Get();
	const AVestigePlayerCharacter* CurrentFarmer = Farmer.Get();
	if (!CurrentInteraction || !CurrentFarmer || !CurrentInteraction->GetFocusedActor())
	{
		InteractionText = FText::GetEmpty();
		return;
	}
	InteractionText = VestigeHudHints::Format({CurrentFarmer->GetInteractKey(), CurrentInteraction->GetFocusedOption().Text});
}

void AVestigeHUD::FollowInventory()
{
	const APlayerState* PlayerState = PlayerOwner ? PlayerOwner->PlayerState.Get() : nullptr;
	Inventory = PlayerState ? PlayerState->FindComponentByClass<UVestigeInventoryComponent>() : nullptr;
	if (UVestigeInventoryComponent* CurrentInventory = Inventory.Get())
	{
		CurrentInventory->OnInventoryChanged.AddUObject(this, &AVestigeHUD::RefreshInventoryText);
	}
	RefreshInventoryText();
}

void AVestigeHUD::FollowAbilitySystem()
{
	const APlayerState* PlayerState = PlayerOwner ? PlayerOwner->PlayerState.Get() : nullptr;
	AbilitySystem = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PlayerState);
	if (UAbilitySystemComponent* CurrentAbilitySystem = AbilitySystem.Get())
	{
		for (const FGameplayAttribute& Attribute :
			{UVestigeStaminaSet::GetStaminaAttribute(), UVestigeStaminaSet::GetMaxStaminaAttribute()})
		{
			CurrentAbilitySystem->GetGameplayAttributeValueChangeDelegate(Attribute).AddWeakLambda(this,
				[this](const FOnAttributeChangeData&)
				{
					RefreshStaminaText();
				});
		}

		// The set decides when the player is out of stamina; the HUD follows its tag rather than judging the number itself.
		CurrentAbilitySystem->RegisterGameplayTagEvent(VestigeGameplayTags::State_OutOfStamina)
			.AddUObject(this, &AVestigeHUD::RefreshOutOfStamina);
		bOutOfStamina = CurrentAbilitySystem->HasMatchingGameplayTag(VestigeGameplayTags::State_OutOfStamina);
	}
	RefreshStaminaText();
}

void AVestigeHUD::RefreshOutOfStamina(const FGameplayTag Tag, int32 NewCount)
{
	bOutOfStamina = NewCount > 0;
	if (HudWidget)
	{
		HudWidget->RefreshStaminaColor();
	}
}

void AVestigeHUD::RefreshStaminaText()
{
	const UAbilitySystemComponent* CurrentAbilitySystem = AbilitySystem.Get();
	if (!CurrentAbilitySystem || !CurrentAbilitySystem->HasAttributeSetForAttribute(UVestigeStaminaSet::GetStaminaAttribute()))
	{
		StaminaText = FText::GetEmpty();
		return;
	}

	// Rounded up, so the HUD only shows 0 when the player is really out of stamina.
	const int32 Stamina = FMath::CeilToInt(CurrentAbilitySystem->GetNumericAttribute(UVestigeStaminaSet::GetStaminaAttribute()));
	const int32 MaxStamina = FMath::RoundToInt(CurrentAbilitySystem->GetNumericAttribute(UVestigeStaminaSet::GetMaxStaminaAttribute()));
	StaminaText = FText::FormatNamed(
		LOCTEXT("Stamina", "Stamina {Stamina} / {MaxStamina}"), TEXT("Stamina"), Stamina, TEXT("MaxStamina"), MaxStamina);
}

void AVestigeHUD::RefreshToolTexts(const UVestigeToolData* Tool)
{
	ToolText = Tool ? Tool->GetDisplayName() : LOCTEXT("NoTool", "No tool");

	// The status line shows the tool, so it is built again on its next read.
	StatusDay = INDEX_NONE;
	RefreshControlsText();
}

void AVestigeHUD::RefreshInventoryText()
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

void AVestigeHUD::RefreshControlsText()
{
	const AVestigePlayerCharacter* CurrentFarmer = Farmer.Get();
	if (!CurrentFarmer)
	{
		ControlsText = FText::GetEmpty();
		return;
	}

	TArray<FText> ToolbarHints;
	for (const FVestigeKeyHint& Hint : CurrentFarmer->GetToolbarKeyHints())
	{
		ToolbarHints.Add(VestigeHudHints::Format(Hint));
	}

	// The toolbar hints belong together; the other hints are set apart from them and from each other.
	TArray<FText> Groups;
	if (!ToolbarHints.IsEmpty())
	{
		Groups.Add(FText::Join(FText::AsCultureInvariant(TEXT("   ")), ToolbarHints));
	}
	for (const FVestigeKeyHint& Hint : CurrentFarmer->GetActionKeyHints())
	{
		Groups.Add(VestigeHudHints::Format(Hint));
	}
	ControlsText = FText::Join(FText::AsCultureInvariant(TEXT("   |   ")), Groups);
}

void AVestigeHUD::ShowMessage(const FText& Message)
{
	Messages.Add(Message, GetWorld()->GetTimeSeconds(), MaxMessages);
	MessagesChanged();
}

void AVestigeHUD::RemoveExpiredMessages()
{
	Messages.RemoveExpired(GetWorld()->GetTimeSeconds(), MessageSeconds);
	MessagesChanged();
}

void AVestigeHUD::MessagesChanged()
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
		ExpireTimer, this, &AVestigeHUD::RemoveExpiredMessages, FMath::Max(MinimumWaitSeconds, static_cast<float>(SecondsLeft.GetValue())));
}

float AVestigeHUD::GetMessageOpacity(int32 Index) const
{
	const TArray<FVestigeHudMessage>& Shown = Messages.GetMessages();
	if (!Shown.IsValidIndex(Index) || !GetWorld())
	{
		return 0.f;
	}
	return FVestigeHudMessages::GetOpacity(Shown[Index].StartTime, GetWorld()->GetTimeSeconds(), MessageSeconds);
}

#undef LOCTEXT_NAMESPACE
