// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "VestigeHudMessageWidget.generated.h"

class UTextBlock;

/**
 * One feedback message in a box of its own: the base of WBP_HudMessage (Content/Vestige/UI). UVestigeHudWidget makes these
 * once and sets them up again for each message. While shown it fades in and out on its own, the whole box at once,
 * following FVestigeHudMessages::GetOpacity. It ticks only while shown: Slate ticks only what it paints, and a hidden
 * box is collapsed.
 */
UCLASS(Abstract)
class UVestigeHudMessageWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Shows Text from StartTime (world time, in seconds) for Lifetime seconds, fading in and out. Fires OnMessageShown. */
	void ShowMessage(const FText& Text, double StartTime, double Lifetime);

	/** Collapses the box until the next message. */
	void Hide();

	/** Sets the box's opacity for the world time Now. Called every frame while shown; public so tests need no Slate. */
	void UpdateOpacity(double Now);

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** After the box has been set up and shown with a new message. */
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void OnMessageShown();

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> MessageText;

	double MessageStartTime = 0.0;
	double MessageLifetime = 0.0;
};
