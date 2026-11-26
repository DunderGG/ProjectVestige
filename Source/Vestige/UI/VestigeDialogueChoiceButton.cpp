// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "UI/VestigeDialogueChoiceButton.h"

#include "Components/TextBlock.h"

void UVestigeDialogueChoiceButton::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	OnFocusReceived().AddUObject(this, &UVestigeDialogueChoiceButton::ShowFocused);
	OnFocusLost().AddUObject(this, &UVestigeDialogueChoiceButton::ShowUnfocused);
	ShowUnfocused();
}

void UVestigeDialogueChoiceButton::SetupChoice(const FText& Text, int32 InIndex)
{
	ChoiceIndex = InIndex;
	ChoiceText->SetText(Text);

	// A list that shows again starts unfocused; the box focuses the first reply after it.
	ShowUnfocused();
}

void UVestigeDialogueChoiceButton::NativeOnClicked()
{
	Super::NativeOnClicked();
	OnChoiceClicked.Broadcast(ChoiceIndex);
}

void UVestigeDialogueChoiceButton::NativeOnHovered()
{
	Super::NativeOnHovered();

	// The mouse moves the highlight, which is always the focused reply.
	SetFocus();
}

void UVestigeDialogueChoiceButton::ShowFocused()
{
	ChoiceText->SetColorAndOpacity(FocusedColor);
	if (FocusMarker)
	{
		FocusMarker->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UVestigeDialogueChoiceButton::ShowUnfocused()
{
	ChoiceText->SetColorAndOpacity(UnfocusedColor);
	if (FocusMarker)
	{
		FocusMarker->SetVisibility(ESlateVisibility::Hidden);
	}
}
