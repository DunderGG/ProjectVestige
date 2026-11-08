// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "UI/VestigeHudWidget.h"

#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Core/VestigeLog.h"
#include "UI/VestigeHudMessageWidget.h"
#include "UI/VestigeHudMessages.h"

UVestigeHudWidget::UVestigeHudWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// The HUD never takes the mouse, and stays out of the hit-test grid. Internal, as UUserWidget's own constructor does,
	// so no visibility event fires on the class default object.
	SetVisibilityInternal(ESlateVisibility::HitTestInvisible);
}

void UVestigeHudWidget::SetStatusText(const FText& Text)
{
	StatusText->SetText(Text);
}

void UVestigeHudWidget::SetStamina(const FText& Text, int32 Stamina, int32 MaxStamina, bool bOutOfStamina, bool bExhausted)
{
	StaminaText->SetText(Text);
	StaminaText->SetColorAndOpacity(bOutOfStamina ? StaminaWarningColor : StaminaColor);
	OnStaminaChanged(Stamina, MaxStamina, bOutOfStamina, bExhausted);
}

void UVestigeHudWidget::SetBagText(const FText& Text)
{
	BagText->SetText(Text);
}

void UVestigeHudWidget::SetPromptText(const FText& Text)
{
	const bool bShown = !Text.IsEmpty();
	PromptText->SetText(Text);
	PromptBox->SetVisibility(bShown ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	OnPromptChanged(bShown);
}

void UVestigeHudWidget::SetKeyHintsText(const FText& Text)
{
	KeyHintsText->SetText(Text);
}

void UVestigeHudWidget::ShowMessages(const TArray<FVestigeHudMessage>& Messages, double Lifetime)
{
	AddMessageBoxes(Messages.Num());
	for (int32 Index = 0; Index < MessageBoxes.Num(); ++Index)
	{
		UVestigeHudMessageWidget* MessageBox = MessageBoxes[Index];
		if (Messages.IsValidIndex(Index))
		{
			MessageBox->ShowMessage(Messages[Index].Text, Messages[Index].StartTime, Lifetime);
		}
		else
		{
			MessageBox->Hide();
		}
	}
}

void UVestigeHudWidget::AddMessageBoxes(int32 Count)
{
	if (MessageBoxes.Num() >= Count)
	{
		return;
	}
	if (!MessageWidgetClass)
	{
		VESTIGE_LOG(this, LogVestige, Error, "No Message Widget Class is set on {Class}, so it can't show feedback messages.",
			GetClass()->GetName());
		return;
	}

	while (MessageBoxes.Num() < Count)
	{
		UVestigeHudMessageWidget* MessageBox = CreateWidget<UVestigeHudMessageWidget>(this, MessageWidgetClass);
		if (!MessageBox)
		{
			return;
		}
		if (UVerticalBoxSlot* BoxSlot = MessageList->AddChildToVerticalBox(MessageBox))
		{
			BoxSlot->SetHorizontalAlignment(HAlign_Center);
			BoxSlot->SetPadding(FMargin(0.f, MessageBoxes.IsEmpty() ? 0.f : MessageSpacing, 0.f, 0.f));
		}
		MessageBoxes.Add(MessageBox);
	}
}
