// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CommonActivatableWidget.h"
#include "CoreMinimal.h"
#include "Player/VestigeKeyHints.h"
#include "VestigeDialogueWidget.generated.h"

class UInputAction;
class UTextBlock;
class UVerticalBox;
class UVestigeConversationComponent;
class UVestigeDialogueChoiceButton;
enum class ECommonInputType : uint8;

namespace VestigeDialogue
{
	/**
	 * The priority of the dialogue box's mapping context: above every context the pawn applies (0 and 1), so while the box is
	 * open its keys win over the pawn's actions on the shared keys (F, Space, gamepad A and B). Systems.md, section 6.
	 */
	constexpr int32 InputMappingPriority = 100;
}

/**
 * The dialogue box: the base of WBP_Dialogue (Content/Vestige/UI), a Common UI screen on the layout's menu layer, the first that
 * takes the input (Systems.md, section 6). While active it shows what the player's conversation component says (the speaker's
 * name, the line typed out, the replies under it, a key hint) and reports presses back through the component's Advance, Choose
 * and EndConversation; it never decides what comes next. It finds the component itself when it is activated and deactivates
 * itself when the talk ends, which pops it. Typing is presentation only: the component already holds the whole line.
 */
UCLASS(Abstract)
class UVestigeDialogueWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UVestigeDialogueWidget(const FObjectInitializer& ObjectInitializer);

	/** Menu input with the cursor shown: moving, looking, the tool key and interacting are off while the box is open. */
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

	/**
	 * A press of Next (the action's keys, or a left click on the box). While the line types out it shows the whole line; with
	 * replies listed it picks the focused one; otherwise it advances the talk, which on the last line ends it.
	 */
	void PressNext();

	/** A press of End: ends the talk now. */
	void PressEnd();

	/** Whether the line is still typing out. */
	bool IsTyping() const { return bTyping; }

#if WITH_DEV_AUTOMATION_TESTS
	/** For tests only. The text shown in the line part now, which grows while typing. */
	FText GetShownLineForTests() const;

	/** For tests only. The key hint shown. */
	FText GetHintForTests() const;

	/** For tests only. The reply buttons made so far. */
	const TArray<TObjectPtr<UVestigeDialogueChoiceButton>>& GetChoiceButtonsForTests() const { return ChoiceButtons; }

	/** For tests only. Whether the reply at Index is shown. */
	bool IsReplyShownForTests(int32 Index) const;
#endif

protected:
	/** Registers the keys' bindings. */
	virtual void NativeOnInitialized() override;

	/** Finds the conversation component, follows it and shows its line. Deactivates at once, logging an error, without one. */
	virtual void NativeOnActivated() override;

	/** Stops following the component and the device. */
	virtual void NativeOnDeactivated() override;

	/** Types the line out while it is typing. Does nothing otherwise. */
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** The first reply while replies show, else the box itself, so keys go to the box and not to the world. */
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	/** A left click on the box itself (the reply buttons handle their own) is a press of Next while no replies show. */
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	/** Shows the component's current line from its start. Bound to its OnLineChanged. */
	void ShowLine();

	/** Shows the whole line and, under it, the replies the line lists. */
	void FinishTyping();

	/** Shows the reply buttons for Choices and asks Common UI to focus the first. */
	void ShowReplies(const TArray<FText>& Choices);

	/** Collapses the reply buttons and asks Common UI to put the focus back on the box. */
	void HideReplies();

	/** Makes reply buttons until there are Count, at most the most replies a line lists. */
	void AddChoiceButtons(int32 Count);

	/** Picks the reply at Index. Bound to each button's click. */
	void ChooseReply(int32 Index);

	/** The index among the listed replies of the one with the focus, or the first. */
	int32 FindFocusedReply() const;

	/** Rebuilds the key hint: "F Next", "F Close" on the last line, "F Choose" with replies. */
	void RefreshHint();

	/** Follows the device the player uses, so the hint names its key. */
	void FollowInputMethod();
	void StopFollowingInputMethod();

	/** Rebuilds the hint when the device whose keys it names changes. Bound to Common Input's OnInputMethodChangedNative. */
	void HandleInputMethodChanged(ECommonInputType InputType);

	/** The talk ended: lets go of the component and pops the box. Bound to its OnConversationEnded. */
	void CloseBox();

	void StopFollowingConversation();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SpeakerNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> LineText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> HintText;

	/** Where the reply buttons go, one under the other. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> ChoiceList;

	/** How fast a line types out, in characters per second. 0 shows each line at once. */
	UPROPERTY(EditDefaultsOnly, Category = "Style", meta = (AllowPrivateAccess = "true", ClampMin = 0, Units = "Hz"))
	float CharactersPerSecond = 40.f;

	/** The gap above every reply but the first, in Slate units. */
	UPROPERTY(EditDefaultsOnly, Category = "Style", meta = (AllowPrivateAccess = "true", ClampMin = 0))
	float ChoiceSpacing = 8.f;

	/** The button each reply shows in (WBP_DialogueChoice). */
	UPROPERTY(EditDefaultsOnly, Category = "Style", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UVestigeDialogueChoiceButton> ChoiceButtonClass;

	/** Advances the talk (IA_DialogueNext), bound in NativeOnInitialized. Its keys are in the box's mapping context, IMC_Dialogue. */
	UPROPERTY(EditDefaultsOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<const UInputAction> NextAction;

	/** Ends the talk early (IA_DialogueEnd). */
	UPROPERTY(EditDefaultsOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<const UInputAction> EndAction;

	/** The reply buttons made so far, in ChoiceList's order. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UVestigeDialogueChoiceButton>> ChoiceButtons;

	TWeakObjectPtr<UVestigeConversationComponent> Conversation;

	/** The line being typed out, as plain characters, and how many of them show. */
	FString FullLine;
	int32 ShownCharacters = 0;
	float TypingSeconds = 0.f;
	bool bTyping = false;

	/** Whether the reply buttons show. */
	bool bRepliesShown = false;

	/** The device whose keys the hint names. */
	EVestigeHintDevice HintDevice = EVestigeHintDevice::KeyboardAndMouse;
};
