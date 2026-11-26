// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CommonActivatableWidget.h"
#include "CoreMinimal.h"
#include "Styling/SlateColor.h"
#include "VestigeHudWidget.generated.h"

class UTextBlock;
class UVerticalBox;
class UVestigeHudMessageWidget;
struct FVestigeHudMessage;

/**
 * The farming HUD's layout: the base of WBP_Hud (Content/Vestige/UI), which holds the layout and style (Systems.md,
 * section 6). AVestigeHUD builds every text and pushes it here when its event fires; this widget never reads the game
 * or the HUD, so it has no bindings and no tick. Each setter sets its parts first and then fires its event, so a
 * Blueprint can restyle from the event (a stamina bar from the numbers) without C++, and needs no graph to work.
 * Each feedback message shows in a box of its own (UVestigeHudMessageWidget), made once and reused.
 * It is the game layer's activatable widget (Systems.md, section 6, Common UI): while it is the active screen, input goes to the
 * game, so closing the last screen on the menu layer hands the input back. It never takes the mouse or the focus.
 */
UCLASS(Abstract, meta = (DisableNativeTick))
class UVestigeHudWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UVestigeHudWidget(const FObjectInitializer& ObjectInitializer);

	/** "Spring, Day 3   18:40   |   Hoe". */
	void SetStatusText(const FText& Text);

	/**
	 * "Stamina 85 / 100", with the numbers it shows. The line takes StaminaWarningColor while the player is out of
	 * stamina. Fires OnStaminaChanged.
	 */
	void SetStamina(const FText& Text, int32 Stamina, int32 MaxStamina, bool bOutOfStamina, bool bExhausted);

	/** "Coins 250", under the stamina. Does nothing while the widget has no MoneyText. */
	void SetMoneyText(const FText& Text);

	/** "Bag: Turnip Seeds 9   Turnip 2". */
	void SetBagText(const FText& Text);

	/** "F Sleep" while an interactable is focused; empty collapses the prompt box. Fires OnPromptChanged. */
	void SetPromptText(const FText& Text);

	/** The key hints line. */
	void SetKeyHintsText(const FText& Text);

	/** "Lightly contaminated" for the faced tile, or empty. Does nothing while the widget has no TileText. */
	void SetTileText(const FText& Text);

	/**
	 * Hides the prompt box and the key hints while a talk runs, and shows them again after: the box has its own hint, and the
	 * prompt is about the world. The clock, date, stamina and bag stay. Fires OnTalkingChanged.
	 */
	void SetTalking(bool bInTalking);

	/** The input config while the HUD is the active screen: the game's, with the mouse captured and hidden. */
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

	/**
	 * Shows each message in its own box, oldest first, fading over Lifetime seconds, and collapses the boxes beyond the
	 * list. Boxes are made only when the list is longer than any before and are never removed.
	 */
	void ShowMessages(const TArray<FVestigeHudMessage>& Messages, double Lifetime);

	/** The box each feedback message shows in, WBP_HudMessage on WBP_Hud. */
	TSubclassOf<UVestigeHudMessageWidget> GetMessageWidgetClass() const { return MessageWidgetClass; }

protected:
	/** After the stamina line has been set: the numbers it shows, for a Blueprint that draws stamina its own way. */
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void OnStaminaChanged(int32 Stamina, int32 MaxStamina, bool bOutOfStamina, bool bExhausted);

	/** After the prompt box has been shown or collapsed. */
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void OnPromptChanged(bool bShown);

	/** After the prompt and key hints have been hidden for a talk or shown again. */
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void OnTalkingChanged(bool bInTalking);

private:
	/** Shows or collapses the prompt box: shown while the prompt has a text and no talk runs. */
	void ApplyPromptVisibility();

	/** Whether the prompt has a text to show, kept so a talk can hide the box and give it back. */
	bool bPromptHasText = false;

	/** Whether a talk runs, which hides the prompt and the key hints. */
	bool bTalking = false;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> StaminaText;

	/**
	 * The money line under the stamina. Optional, as TileText is: it was added to the C++ after WBP_Hud's layout, so the widget
	 * compiles before a person adds it in the designer. Without it the HUD shows no money line.
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MoneyText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> BagText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PromptText;

	/**
	 * The faced tile's contamination line, under the prompt for now. Optional for the same reason as MoneyText: a person adds
	 * it to WBP_Hud in the designer. Without it the HUD shows no tile line.
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TileText;

	/** The backdrop around PromptText, collapsed while the prompt is empty. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> PromptBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> KeyHintsText;

	/** Where the message boxes go, one under the other. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> MessageList;

	/** The stamina line's colour. */
	UPROPERTY(EditDefaultsOnly, Category = "Style", meta = (AllowPrivateAccess = "true"))
	FSlateColor StaminaColor = FLinearColor::White;

	/** The stamina line's colour while the player is out of stamina. */
	UPROPERTY(EditDefaultsOnly, Category = "Style", meta = (AllowPrivateAccess = "true"))
	FSlateColor StaminaWarningColor = FLinearColor(1.f, 0.3f, 0.25f);

	/** The gap above every message box but the first, in Slate units. */
	UPROPERTY(EditDefaultsOnly, Category = "Style", meta = (AllowPrivateAccess = "true", ClampMin = 0))
	float MessageSpacing = 6.f;

	/** The box each feedback message shows in. */
	UPROPERTY(EditDefaultsOnly, Category = "Style", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UVestigeHudMessageWidget> MessageWidgetClass;

	/** The message boxes made so far, in MessageList's order. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UVestigeHudMessageWidget>> MessageBoxes;

	/** Makes message boxes until there are Count, adding each to MessageList. */
	void AddMessageBoxes(int32 Count);
};
