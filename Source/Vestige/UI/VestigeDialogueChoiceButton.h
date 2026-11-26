// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CommonButtonBase.h"
#include "CoreMinimal.h"
#include "Styling/SlateColor.h"
#include "VestigeDialogueChoiceButton.generated.h"

class UTextBlock;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnVestigeChoiceClicked, int32 /* Index */);

/**
 * One reply the player can pick: the base of WBP_DialogueChoice (Content/Vestige/UI). The dialogue box makes up to three once and
 * sets them up again for each list of replies. A click (Enter, Space, gamepad A, a mouse click: the button's own accept keys)
 * reports the reply's index; hovering it with the mouse focuses it, so the highlight is always the focused reply: the text takes
 * FocusedColor and the optional marker shows while the button has the focus.
 */
UCLASS(Abstract)
class UVestigeDialogueChoiceButton : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	/** Shows Text and remembers that this button is the reply at Index among the listed ones. */
	void SetupChoice(const FText& Text, int32 InIndex);

	/** The index SetupChoice was given, or INDEX_NONE. */
	int32 GetChoiceIndex() const { return ChoiceIndex; }

	/** Broadcast with the reply's index when the button is clicked. */
	FOnVestigeChoiceClicked OnChoiceClicked;

protected:
	/** Follows the button's own focus, to highlight the focused reply. */
	virtual void NativeOnInitialized() override;
	virtual void NativeOnClicked() override;
	virtual void NativeOnHovered() override;

private:
	/** Shows the highlight, or hides it. Bound to the button's focus events. */
	void ShowFocused();
	void ShowUnfocused();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ChoiceText;

	/** A mark beside the focused reply, such as an arrow. Optional: without it only the text's colour shows the focus. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> FocusMarker;

	/** The text's colour while the reply has the focus. */
	UPROPERTY(EditDefaultsOnly, Category = "Style", meta = (AllowPrivateAccess = "true"))
	FSlateColor FocusedColor = FLinearColor(1.f, 0.85f, 0.45f);

	/** The text's colour otherwise. */
	UPROPERTY(EditDefaultsOnly, Category = "Style", meta = (AllowPrivateAccess = "true"))
	FSlateColor UnfocusedColor = FLinearColor(0.85f, 0.85f, 0.85f);

	int32 ChoiceIndex = INDEX_NONE;
};
