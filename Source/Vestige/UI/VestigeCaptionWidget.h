// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CommonUserWidget.h"
#include "CoreMinimal.h"
#include "VestigeCaptionWidget.generated.h"

class UTextBlock;
class UVestigeConversationComponent;

/**
 * The caption: the base of WBP_Caption (Content/Vestige/UI), placed in WBP_Hud's overlay by its layout (Systems.md, section 6). It
 * shows the speaker's name and the line while a character says one in passing (Systems.md, section 11), low on the screen, and
 * fades by itself with the line's time. It takes no input and never the mouse. It finds the player's conversation component
 * itself, follows its passing line, and ticks only while a line shows.
 */
UCLASS(Abstract)
class UVestigeCaptionWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UVestigeCaptionWidget(const FObjectInitializer& ObjectInitializer);

	/** Whether the caption is showing a line. */
	bool IsCaptionShown() const { return bShown; }

#if WITH_DEV_AUTOMATION_TESTS
	/** For tests only. Follows Conversation instead of the owning player state's, and shows its line if one shows. */
	void SetConversationForTests(UVestigeConversationComponent* Conversation);

	/** For tests only. The texts shown. */
	FText GetSpeakerNameForTests() const;
	FText GetLineForTests() const;
#endif

protected:
	/** Finds the conversation component, follows it and shows a line already being said. */
	virtual void NativeConstruct() override;

	/** Stops following it. */
	virtual void NativeDestruct() override;

	/** Fades the caption with the line's time. */
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** After a line has been set and shown. */
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void OnCaptionShown();

private:
	void Follow(UVestigeConversationComponent* Conversation);
	void StopFollowing();

	/** Shows the component's passing line. Bound to its OnPassingLineStarted. */
	void ShowCaption();

	/** Collapses the caption. Bound to its OnPassingLineEnded. */
	void HideCaption();

	void UpdateOpacity();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SpeakerNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> LineText;

	TWeakObjectPtr<UVestigeConversationComponent> Followed;

	bool bShown = false;
};
