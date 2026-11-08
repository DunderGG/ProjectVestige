// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "UI/VestigeHUD.h"

#include "AbilitySystem/VestigeGameplayTags.h"
#include "AbilitySystem/VestigeStaminaSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "CommonInputSubsystem.h"
#include "CommonInputTypeEnum.h"
#include "Content/VestigeItemData.h"
#include "Core/VestigeFeedbackSubsystem.h"
#include "Core/VestigeLog.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameplayTagContainer.h"
#include "Interaction/VestigeInteractionComponent.h"
#include "Inventory/VestigeInventoryComponent.h"
#include "Player/VestigePlayerCharacter.h"
#include "Time/VestigeClockSubsystem.h"
#include "TimerManager.h"
#include "UI/VestigeFeedbackResponses.h"
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

AVestigeHUD::AVestigeHUD()
{
	// The HUD ticks for the clock alone: the time changes every frame with no event (the clock broadcasts only whole
	// hours, Roadmap.md), so Tick compares two numbers and rebuilds the status line only when the shown minute changes.
	// The tick is turned off when there is no widget or no clock to show.
	PrimaryActorTick.bCanEverTick = true;
}

void AVestigeHUD::BeginPlay()
{
	Super::BeginPlay();

	const UVestigeHudSettings& Settings = UVestigeHudSettings::Get();
	MaxMessages = Settings.GetMaxMessages();
	MessageSeconds = Settings.GetMessageSeconds();

	// A widget needs a local player on screen: CreateWidget refuses a controller without one.
	const UWorld* World = GetWorld();
	if (!PlayerOwner || !PlayerOwner->GetLocalPlayer() || !World || !World->GetGameViewport())
	{
		SetActorTickEnabled(false);
		return;
	}
	if (!HudWidgetClass)
	{
		VESTIGE_LOG(this, LogVestige, Error, "No HUD Widget Class is set, so the HUD shows nothing. Set it on BP_HUD.");
		SetActorTickEnabled(false);
		return;
	}
	HudWidget = CreateWidget<UVestigeHudWidget>(PlayerOwner, HudWidgetClass);
	if (!HudWidget)
	{
		SetActorTickEnabled(false);
		return;
	}
	HudWidget->AddToPlayerScreen();

	// First, so the first hints and prompt already name the keys of the device in use.
	FollowInputMethod();

	// Each follow refreshes its texts at once, which now reach the widget.
	Clock = UVestigeClockSubsystem::Get(World);
	SetActorTickEnabled(Clock.IsValid());
	PlayerOwner->OnPossessedPawnChanged.AddDynamic(this, &AVestigeHUD::FollowPawn);
	FollowPawn(nullptr, PlayerOwner->GetPawn());
	FollowInventory();
	FollowAbilitySystem();
	if (!FeedbackResponses)
	{
		VESTIGE_LOG(this, LogVestige, Error, "No Feedback Responses are set, so the HUD shows no messages. Set them on BP_HUD.");
	}
	else if (UVestigeFeedbackSubsystem* Feedback = World->GetSubsystem<UVestigeFeedbackSubsystem>())
	{
		Feedback->OnFeedback.AddUObject(this, &AVestigeHUD::ShowFeedback);
	}
	MessagesChanged();
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
		CurrentInventory->OnItemInHandChanged.RemoveAll(this);
	}
	if (UAbilitySystemComponent* CurrentAbilitySystem = AbilitySystem.Get())
	{
		CurrentAbilitySystem->GetGameplayAttributeValueChangeDelegate(UVestigeStaminaSet::GetStaminaAttribute()).RemoveAll(this);
		CurrentAbilitySystem->GetGameplayAttributeValueChangeDelegate(UVestigeStaminaSet::GetMaxStaminaAttribute()).RemoveAll(this);
		CurrentAbilitySystem->RegisterGameplayTagEvent(VestigeGameplayTags::State_OutOfStamina).RemoveAll(this);
		CurrentAbilitySystem->RegisterGameplayTagEvent(VestigeGameplayTags::State_Exhausted).RemoveAll(this);
	}
	if (UVestigeFeedbackSubsystem* Feedback = UWorld::GetSubsystem<UVestigeFeedbackSubsystem>(GetWorld()))
	{
		Feedback->OnFeedback.RemoveAll(this);
	}
	if (UCommonInputSubsystem* CurrentInputMethod = InputMethod.Get())
	{
		CurrentInputMethod->OnInputMethodChangedNative.RemoveAll(this);
	}

	// The widget's outer is the game instance, which outlives this map, so it is let go here.
	if (HudWidget)
	{
		HudWidget->RemoveFromParent();
		HudWidget = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void AVestigeHUD::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const UVestigeClockSubsystem* CurrentClock = Clock.Get();
	if (!CurrentClock)
	{
		return;
	}
	if (CurrentClock->GetCurrentDay() != StatusDay || VestigeHudClock::ToShownMinutes(CurrentClock->GetTimeOfDay()) != StatusClockMinutes)
	{
		RefreshStatusText();
	}
}

void AVestigeHUD::RefreshStatusText()
{
	const UVestigeClockSubsystem* CurrentClock = Clock.Get();
	if (!HudWidget || !CurrentClock)
	{
		return;
	}

	StatusDay = CurrentClock->GetCurrentDay();
	StatusClockMinutes = VestigeHudClock::ToShownMinutes(CurrentClock->GetTimeOfDay());
	HudWidget->SetStatusText(FText::FormatNamed(LOCTEXT("Status", "Day {Day}   {Clock}   |   {Item}"), TEXT("Day"), StatusDay,
		TEXT("Clock"), VestigeHudClock::Format(StatusClockMinutes), TEXT("Item"), ItemInHandText));
}

EVestigeHintDevice AVestigeHUD::GetHintDevice(ECommonInputType InputType)
{
	return InputType == ECommonInputType::Gamepad ? EVestigeHintDevice::Gamepad : EVestigeHintDevice::KeyboardAndMouse;
}

void AVestigeHUD::FollowInputMethod()
{
	// Common Input sorts every key, button, mouse move and touch by the key itself, and is what Common UI's menus will
	// follow too (Systems.md, section 5, gamepad key hints).
	InputMethod = UCommonInputSubsystem::Get(PlayerOwner ? PlayerOwner->GetLocalPlayer() : nullptr);
	if (UCommonInputSubsystem* CurrentInputMethod = InputMethod.Get())
	{
		HintDevice = GetHintDevice(CurrentInputMethod->GetCurrentInputType());
		CurrentInputMethod->OnInputMethodChangedNative.AddUObject(this, &AVestigeHUD::HandleInputMethodChanged);
	}
}

void AVestigeHUD::HandleInputMethodChanged(ECommonInputType InputType)
{
	const EVestigeHintDevice NewDevice = GetHintDevice(InputType);
	if (NewDevice == HintDevice)
	{
		return;
	}
	HintDevice = NewDevice;
	RefreshKeyHintsText();
	RefreshPromptText();
}

void AVestigeHUD::FollowPawn(APawn* OldPawn, APawn* NewPawn)
{
	StopFollowingPawn();
	Farmer = Cast<AVestigePlayerCharacter>(NewPawn);
	RefreshKeyHintsText();

	Interaction = NewPawn ? NewPawn->FindComponentByClass<UVestigeInteractionComponent>() : nullptr;
	if (UVestigeInteractionComponent* NewInteraction = Interaction.Get())
	{
		NewInteraction->OnFocusChanged.AddUObject(this, &AVestigeHUD::RefreshPromptText);
	}
	RefreshPromptText();
}

void AVestigeHUD::StopFollowingPawn()
{
	if (UVestigeInteractionComponent* OldInteraction = Interaction.Get())
	{
		OldInteraction->OnFocusChanged.RemoveAll(this);
	}
}

void AVestigeHUD::RefreshPromptText()
{
	if (!HudWidget)
	{
		return;
	}

	const UVestigeInteractionComponent* CurrentInteraction = Interaction.Get();
	const AVestigePlayerCharacter* CurrentFarmer = Farmer.Get();
	if (!CurrentInteraction || !CurrentFarmer || !CurrentInteraction->GetFocusedActor())
	{
		HudWidget->SetPromptText(FText::GetEmpty());
		return;
	}
	HudWidget->SetPromptText(
		VestigeHudHints::Format({CurrentFarmer->GetInteractKey(HintDevice), CurrentInteraction->GetFocusedOption().Text}));
}

void AVestigeHUD::FollowInventory()
{
	const APlayerState* PlayerState = PlayerOwner ? PlayerOwner->PlayerState.Get() : nullptr;
	Inventory = PlayerState ? PlayerState->FindComponentByClass<UVestigeInventoryComponent>() : nullptr;
	if (UVestigeInventoryComponent* CurrentInventory = Inventory.Get())
	{
		CurrentInventory->OnInventoryChanged.AddUObject(this, &AVestigeHUD::RefreshInventoryTexts);
		CurrentInventory->OnItemInHandChanged.AddUObject(this, &AVestigeHUD::RefreshItemInHandText);
	}
	RefreshInventoryTexts();
	RefreshItemInHandText();
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
					RefreshStamina();
				});
		}

		// The set decides when the player is out of stamina; the HUD follows its tag rather than judging the number itself.
		for (const FGameplayTag& Tag : {VestigeGameplayTags::State_OutOfStamina.GetTag(), VestigeGameplayTags::State_Exhausted.GetTag()})
		{
			CurrentAbilitySystem->RegisterGameplayTagEvent(Tag).AddWeakLambda(this,
				[this](const FGameplayTag, int32)
				{
					RefreshStamina();
				});
		}
	}
	RefreshStamina();
}

void AVestigeHUD::RefreshStamina()
{
	if (!HudWidget)
	{
		return;
	}

	const UAbilitySystemComponent* CurrentAbilitySystem = AbilitySystem.Get();
	if (!CurrentAbilitySystem || !CurrentAbilitySystem->HasAttributeSetForAttribute(UVestigeStaminaSet::GetStaminaAttribute()))
	{
		HudWidget->SetStamina(FText::GetEmpty(), 0, 0, false, false);
		return;
	}

	// Rounded up, so the HUD only shows 0 when the player is really out of stamina.
	const int32 Stamina = FMath::CeilToInt(CurrentAbilitySystem->GetNumericAttribute(UVestigeStaminaSet::GetStaminaAttribute()));
	const int32 MaxStamina = FMath::RoundToInt(CurrentAbilitySystem->GetNumericAttribute(UVestigeStaminaSet::GetMaxStaminaAttribute()));
	const bool bOutOfStamina = CurrentAbilitySystem->HasMatchingGameplayTag(VestigeGameplayTags::State_OutOfStamina);
	const bool bExhausted = CurrentAbilitySystem->HasMatchingGameplayTag(VestigeGameplayTags::State_Exhausted);
	FText Text = FText::FormatNamed(
		LOCTEXT("Stamina", "Stamina {Stamina} / {MaxStamina}"), TEXT("Stamina"), Stamina, TEXT("MaxStamina"), MaxStamina);
	if (bExhausted)
	{
		Text = FText::FormatNamed(LOCTEXT("StaminaExhausted", "{Stamina}   Exhausted"), TEXT("Stamina"), Text);
	}
	HudWidget->SetStamina(Text, Stamina, MaxStamina, bOutOfStamina, bExhausted);
}

void AVestigeHUD::RefreshItemInHandText()
{
	// Read now rather than passed, since a nested change may reach this handler out of order.
	const UVestigeInventoryComponent* CurrentInventory = Inventory.Get();
	const UVestigeItemData* Item = CurrentInventory ? CurrentInventory->GetItemInHand() : nullptr;
	ItemInHandText = Item ? Item->GetDisplayName() : LOCTEXT("EmptyHand", "Empty hand");
	RefreshStatusText();
}

void AVestigeHUD::RefreshInventoryTexts()
{
	RefreshBagText();

	// The hotbar hints name the items in the hotbar slots.
	RefreshKeyHintsText();
}

void AVestigeHUD::RefreshBagText()
{
	if (!HudWidget)
	{
		return;
	}

	const UVestigeInventoryComponent* CurrentInventory = Inventory.Get();
	if (!CurrentInventory)
	{
		HudWidget->SetBagText(FText::GetEmpty());
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
	HudWidget->SetBagText(FText::FormatNamed(LOCTEXT("Inventory", "Bag: {Contents}"), TEXT("Contents"), Contents));
}

void AVestigeHUD::RefreshKeyHintsText()
{
	if (!HudWidget)
	{
		return;
	}

	const AVestigePlayerCharacter* CurrentFarmer = Farmer.Get();
	if (!CurrentFarmer)
	{
		HudWidget->SetKeyHintsText(FText::GetEmpty());
		return;
	}

	TArray<FText> HotbarHints;
	if (const UVestigeInventoryComponent* CurrentInventory = Inventory.Get())
	{
		for (const FVestigeKeyHint& Hint : CurrentFarmer->GetHotbarKeyHints(*CurrentInventory, HintDevice))
		{
			HotbarHints.Add(VestigeHudHints::Format(Hint));
		}
	}

	// The hotbar hints belong together; the other hints are set apart from them and from each other.
	TArray<FText> Groups;
	if (!HotbarHints.IsEmpty())
	{
		Groups.Add(FText::Join(FText::AsCultureInvariant(TEXT("   ")), HotbarHints));
	}
	for (const FVestigeKeyHint& Hint : CurrentFarmer->GetActionKeyHints(HintDevice))
	{
		Groups.Add(VestigeHudHints::Format(Hint));
	}
	HudWidget->SetKeyHintsText(FText::Join(FText::AsCultureInvariant(TEXT("   |   ")), Groups));
}

void AVestigeHUD::ShowFeedback(const FVestigeFeedbackReport& Report)
{
	const FVestigeFeedbackResponse* Response = FeedbackResponses ? FeedbackResponses->FindResponse(Report.Tag) : nullptr;
	if (!Response)
	{
		VESTIGE_LOG(this, LogVestigeFeedback, Warning, "{Responses} has no response to {Tag}, so the player sees nothing for it.",
			GetNameSafe(FeedbackResponses), Report.Tag.ToString());
		return;
	}

	// An empty message is a deliberate choice to show nothing for that report.
	if (!Response->Message.IsEmpty())
	{
		ShowMessage(UVestigeFeedbackResponses::FormatMessage(*Response, Report));
	}
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
		HudWidget->ShowMessages(Messages.GetMessages(), MessageSeconds);
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

#undef LOCTEXT_NAMESPACE
