// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "UI/VestigeDialogueWidget.h"

#include "Characters/VestigeConversationComponent.h"
#include "Characters/VestigeDialogueLog.h"
#include "CommonInputSubsystem.h"
#include "CommonInputTypeEnum.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Content/VestigeCharacterData.h"
#include "Content/VestigeDialogueData.h"
#include "Engine/EngineBaseTypes.h"
#include "GameFramework/PlayerState.h"
#include "Input/CommonUIInputTypes.h"
#include "Input/Reply.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "UI/VestigeDialogueChoiceButton.h"
#include "UI/VestigeHUD.h"

#define LOCTEXT_NAMESPACE "VestigeDialogueWidget"

UVestigeDialogueWidget::UVestigeDialogueWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// Above every context the pawn applies, or the pawn's actions would keep the keys they share with the box (Systems.md, section 6).
	InputMappingPriority = VestigeDialogue::InputMappingPriority;

	// A focused box is how keys reach it when no reply button is shown.
	SetIsFocusable(true);
}

TOptional<FUIInputConfig> UVestigeDialogueWidget::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

void UVestigeDialogueWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!NextAction || !EndAction)
	{
		VESTIGE_LOG(this, LogVestigeDialogue, Error, "Next Action or End Action isn't set on {Class}, so the box has no keys.",
			GetClass()->GetName());
		return;
	}

	// Not shown in an action bar: the box has its own hint line.
	constexpr bool bShowInActionBar = false;
	RegisterUIActionBinding(
		FBindUIActionArgs(NextAction.Get(), bShowInActionBar, FSimpleDelegate::CreateUObject(this, &UVestigeDialogueWidget::PressNext)));
	RegisterUIActionBinding(
		FBindUIActionArgs(EndAction.Get(), bShowInActionBar, FSimpleDelegate::CreateUObject(this, &UVestigeDialogueWidget::PressEnd)));
}

void UVestigeDialogueWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	Conversation = UVestigeConversationComponent::Find(GetOwningPlayerState());
	UVestigeConversationComponent* Talk = Conversation.Get();
	if (!Talk)
	{
		VESTIGE_LOG(this, LogVestigeDialogue, Error, "The player has no conversation component, so the dialogue box closes.");
		DeactivateWidget();
		return;
	}
	Talk->OnLineChanged.AddUObject(this, &UVestigeDialogueWidget::ShowLine);
	Talk->OnConversationEnded.AddUObject(this, &UVestigeDialogueWidget::CloseBox);
	FollowInputMethod();
	ShowLine();
}

void UVestigeDialogueWidget::NativeOnDeactivated()
{
	StopFollowingConversation();
	StopFollowingInputMethod();
	bTyping = false;
	Super::NativeOnDeactivated();
}

void UVestigeDialogueWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Only while a line types out; Slate ticks only what it paints, and the box is off the screen otherwise.
	if (!bTyping)
	{
		return;
	}
	TypingSeconds += InDeltaTime;
	const int32 ShouldShow = FMath::Min(FMath::FloorToInt(TypingSeconds * CharactersPerSecond), FullLine.Len());
	if (ShouldShow >= FullLine.Len())
	{
		FinishTyping();
	}
	else if (ShouldShow != ShownCharacters)
	{
		ShownCharacters = ShouldShow;
		LineText->SetText(FText::AsCultureInvariant(FullLine.Left(ShownCharacters)));
	}
}

UWidget* UVestigeDialogueWidget::NativeGetDesiredFocusTarget() const
{
	if (bRepliesShown && !ChoiceButtons.IsEmpty())
	{
		return ChoiceButtons[0];
	}
	return const_cast<UVestigeDialogueWidget*>(this);
}

FReply UVestigeDialogueWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton || bRepliesShown)
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}
	PressNext();
	return FReply::Handled();
}

void UVestigeDialogueWidget::PressNext()
{
	UVestigeConversationComponent* Talk = Conversation.Get();
	if (!Talk)
	{
		return;
	}
	if (bTyping)
	{
		FinishTyping();
		return;
	}
	if (bRepliesShown)
	{
		Talk->Choose(FindFocusedReply());
		return;
	}
	Talk->Advance();
}

void UVestigeDialogueWidget::PressEnd()
{
	if (UVestigeConversationComponent* Talk = Conversation.Get())
	{
		Talk->EndConversation();
	}
}

void UVestigeDialogueWidget::ShowLine()
{
	const UVestigeConversationComponent* Talk = Conversation.Get();
	if (!Talk)
	{
		return;
	}
	const UVestigeCharacterData* Speaker = Talk->GetSpeaker();
	SpeakerNameText->SetText(Speaker ? Speaker->GetDisplayName() : FText::GetEmpty());

	// Replies wait for the whole line; the box types it out from the start.
	HideReplies();
	FullLine = Talk->GetLineText().ToString();
	ShownCharacters = 0;
	TypingSeconds = 0.f;
	LineText->SetText(FText::GetEmpty());
	bTyping = CharactersPerSecond > 0.f && !FullLine.IsEmpty();
	if (!bTyping)
	{
		FinishTyping();
		return;
	}
	RefreshHint();
}

void UVestigeDialogueWidget::FinishTyping()
{
	const UVestigeConversationComponent* Talk = Conversation.Get();
	if (!Talk)
	{
		return;
	}
	bTyping = false;
	ShownCharacters = FullLine.Len();

	// The line's own text, not the typed copy of it.
	LineText->SetText(Talk->GetLineText());
	if (!Talk->GetChoiceTexts().IsEmpty())
	{
		ShowReplies(Talk->GetChoiceTexts());
	}
	RefreshHint();
}

void UVestigeDialogueWidget::ShowReplies(const TArray<FText>& Choices)
{
	const int32 Count = FMath::Min(Choices.Num(), VestigeDialogue::MaxChoices);
	AddChoiceButtons(Count);
	for (int32 Index = 0; Index < ChoiceButtons.Num(); ++Index)
	{
		UVestigeDialogueChoiceButton* Button = ChoiceButtons[Index];
		if (Index < Count)
		{
			Button->SetupChoice(Choices[Index], Index);
			Button->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			Button->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	bRepliesShown = Count > 0;

	// Common UI asked for the desired focus target on activation, when no reply existed.
	RequestRefreshFocus();
}

void UVestigeDialogueWidget::HideReplies()
{
	const bool bHadReplies = bRepliesShown;
	bRepliesShown = false;
	for (UVestigeDialogueChoiceButton* Button : ChoiceButtons)
	{
		Button->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (bHadReplies)
	{
		RequestRefreshFocus();
	}
}

void UVestigeDialogueWidget::AddChoiceButtons(int32 Count)
{
	if (ChoiceButtons.Num() >= Count)
	{
		return;
	}
	if (!ChoiceButtonClass)
	{
		VESTIGE_LOG(this, LogVestigeDialogue, Error, "No Choice Button Class is set on {Class}, so the box can't show replies.",
			GetClass()->GetName());
		return;
	}

	while (ChoiceButtons.Num() < Count)
	{
		UVestigeDialogueChoiceButton* Button = CreateWidget<UVestigeDialogueChoiceButton>(this, ChoiceButtonClass);
		if (!Button)
		{
			return;
		}
		Button->OnChoiceClicked.AddUObject(this, &UVestigeDialogueWidget::ChooseReply);
		if (UVerticalBoxSlot* ButtonSlot = ChoiceList->AddChildToVerticalBox(Button))
		{
			ButtonSlot->SetHorizontalAlignment(HAlign_Left);
			ButtonSlot->SetPadding(FMargin(0.f, ChoiceButtons.IsEmpty() ? 0.f : ChoiceSpacing, 0.f, 0.f));
		}
		ChoiceButtons.Add(Button);
	}
}

void UVestigeDialogueWidget::ChooseReply(int32 Index)
{
	if (UVestigeConversationComponent* Talk = Conversation.Get())
	{
		Talk->Choose(Index);
	}
}

int32 UVestigeDialogueWidget::FindFocusedReply() const
{
	for (const UVestigeDialogueChoiceButton* Button : ChoiceButtons)
	{
		if (Button->GetVisibility() != ESlateVisibility::Collapsed && Button->HasUserFocus(GetOwningPlayer()))
		{
			return Button->GetChoiceIndex();
		}
	}
	return 0;
}

void UVestigeDialogueWidget::RefreshHint()
{
	const UVestigeConversationComponent* Talk = Conversation.Get();
	const UInputMappingContext* Mapping = InputMapping;
	if (!Talk || !Mapping || !NextAction)
	{
		HintText->SetText(FText::GetEmpty());
		return;
	}

	FText Action = LOCTEXT("HintNext", "Next");
	if (bRepliesShown)
	{
		Action = LOCTEXT("HintChoose", "Choose");
	}
	else if (Talk->IsLastLine())
	{
		Action = LOCTEXT("HintClose", "Close");
	}
	const FKey Key = VestigeKeyHints::FindHintKey(*Mapping, *NextAction, HintDevice);
	if (!Key.IsValid())
	{
		HintText->SetText(FText::GetEmpty());
		return;
	}
	HintText->SetText(
		FText::FormatNamed(LOCTEXT("Hint", "{Key} {Action}"), TEXT("Key"), VestigeKeyHints::GetKeyName(Key), TEXT("Action"), Action));
}

void UVestigeDialogueWidget::FollowInputMethod()
{
	UCommonInputSubsystem* InputMethod = UCommonInputSubsystem::Get(GetOwningLocalPlayer());
	if (!InputMethod)
	{
		return;
	}
	HintDevice = AVestigeHUD::GetHintDevice(InputMethod->GetCurrentInputType());
	InputMethod->OnInputMethodChangedNative.AddUObject(this, &UVestigeDialogueWidget::HandleInputMethodChanged);
}

void UVestigeDialogueWidget::StopFollowingInputMethod()
{
	if (UCommonInputSubsystem* InputMethod = UCommonInputSubsystem::Get(GetOwningLocalPlayer()))
	{
		InputMethod->OnInputMethodChangedNative.RemoveAll(this);
	}
}

void UVestigeDialogueWidget::HandleInputMethodChanged(ECommonInputType InputType)
{
	const EVestigeHintDevice NewDevice = AVestigeHUD::GetHintDevice(InputType);
	if (NewDevice == HintDevice)
	{
		return;
	}
	HintDevice = NewDevice;
	RefreshHint();
}

void UVestigeDialogueWidget::CloseBox()
{
	StopFollowingConversation();
	DeactivateWidget();
}

void UVestigeDialogueWidget::StopFollowingConversation()
{
	if (UVestigeConversationComponent* Talk = Conversation.Get())
	{
		Talk->OnLineChanged.RemoveAll(this);
		Talk->OnConversationEnded.RemoveAll(this);
	}
	Conversation.Reset();
}

#if WITH_DEV_AUTOMATION_TESTS
FText UVestigeDialogueWidget::GetShownLineForTests() const
{
	return LineText->GetText();
}

FText UVestigeDialogueWidget::GetHintForTests() const
{
	return HintText->GetText();
}

bool UVestigeDialogueWidget::IsReplyShownForTests(int32 Index) const
{
	return bRepliesShown && ChoiceButtons.IsValidIndex(Index) && ChoiceButtons[Index]->GetVisibility() != ESlateVisibility::Collapsed;
}
#endif

#undef LOCTEXT_NAMESPACE
