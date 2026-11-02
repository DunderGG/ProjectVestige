// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class AVestigeHUD;
class STextBlock;
class SVerticalBox;

/**
 * The farming HUD's layout, built in Slate so it needs no widget assets.
 * Shows the texts its AVestigeHUD keeps, and each feedback message in a box of its own. Its bindings run every frame, but
 * only read text the HUD has already built and each message box's opacity.
 */
class SVestigeHudWidget : public SCompoundWidget
{
public:
	// clang-format off
	SLATE_BEGIN_ARGS(SVestigeHudWidget) {}
		SLATE_ARGUMENT(TWeakObjectPtr<const AVestigeHUD>, Hud)
	SLATE_END_ARGS()
	// clang-format on

	void Construct(const FArguments& InArgs);

	/** Shows the HUD's messages, one box each. The HUD calls it whenever a message arrives or expires. */
	void RefreshMessages();

	/** Colors the stamina line, in the warning color while the player is out of stamina. The HUD calls it when that changes. */
	void RefreshStaminaColor();

private:
	FText GetStatusText() const;
	FText GetStaminaText() const;
	FText GetInventoryText() const;
	FText GetControlsText() const;
	FText GetInteractionText() const;

	/** Hides the prompt box while nothing is focused. */
	EVisibility GetInteractionVisibility() const;

	/** The backdrop color of the message box at Index, faded to that message's opacity. */
	FSlateColor GetMessageBackdropColor(int32 Index) const;

	/** Fades everything inside the message box at Index, text and shadow alike, to that message's opacity. */
	FLinearColor GetMessageContentTint(int32 Index) const;

	/** Adds the boxes the messages show in, one per message the HUD can show at once. They are reused, never rebuilt. */
	void AddMessageBoxes(int32 Count);

	TWeakObjectPtr<const AVestigeHUD> Hud;

	TSharedPtr<STextBlock> StaminaLine;
	TSharedPtr<SVerticalBox> MessageList;
	TArray<TSharedPtr<SWidget>> MessageBoxes;
	TArray<TSharedPtr<STextBlock>> MessageTexts;
};
