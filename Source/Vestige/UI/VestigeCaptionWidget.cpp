// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "UI/VestigeCaptionWidget.h"

#include "Characters/VestigeConversationComponent.h"
#include "Components/TextBlock.h"
#include "Content/VestigeCharacterData.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "UI/VestigeHudMessages.h"

UVestigeCaptionWidget::UVestigeCaptionWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// The caption never takes the mouse. Internal, as the HUD widget's constructor does, so no visibility event fires on the default object.
	SetVisibilityInternal(ESlateVisibility::Collapsed);
}

void UVestigeCaptionWidget::NativeConstruct()
{
	Super::NativeConstruct();

	Follow(UVestigeConversationComponent::Find(GetOwningPlayerState()));
}

void UVestigeCaptionWidget::NativeDestruct()
{
	StopFollowing();
	Super::NativeDestruct();
}

void UVestigeCaptionWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// The fade follows the world's clock, so it pauses with the game, as the feedback messages do.
	if (bShown)
	{
		UpdateOpacity();
	}
}

void UVestigeCaptionWidget::Follow(UVestigeConversationComponent* Conversation)
{
	StopFollowing();
	if (!Conversation)
	{
		return;
	}
	Conversation->OnPassingLineStarted.AddUObject(this, &UVestigeCaptionWidget::ShowCaption);
	Conversation->OnPassingLineEnded.AddUObject(this, &UVestigeCaptionWidget::HideCaption);
	Followed = Conversation;
	if (Conversation->IsSayingPassingLine())
	{
		ShowCaption();
	}
}

void UVestigeCaptionWidget::StopFollowing()
{
	if (UVestigeConversationComponent* Conversation = Followed.Get())
	{
		Conversation->OnPassingLineStarted.RemoveAll(this);
		Conversation->OnPassingLineEnded.RemoveAll(this);
	}
	Followed.Reset();
}

void UVestigeCaptionWidget::ShowCaption()
{
	const UVestigeConversationComponent* Conversation = Followed.Get();
	const UVestigeCharacterData* Speaker = Conversation ? Conversation->GetPassingSpeaker() : nullptr;
	if (!Conversation || !Speaker)
	{
		return;
	}
	SpeakerNameText->SetText(Speaker->GetDisplayName());
	LineText->SetText(Conversation->GetPassingLineText());
	bShown = true;
	UpdateOpacity();
	SetVisibility(ESlateVisibility::HitTestInvisible);
	OnCaptionShown();
}

void UVestigeCaptionWidget::HideCaption()
{
	bShown = false;
	SetVisibility(ESlateVisibility::Collapsed);
}

void UVestigeCaptionWidget::UpdateOpacity()
{
	const UVestigeConversationComponent* Conversation = Followed.Get();
	const UWorld* World = GetWorld();
	if (!Conversation || !World)
	{
		return;
	}
	SetRenderOpacity(FVestigeHudMessages::GetOpacity(
		Conversation->GetPassingLineStartTime(), World->GetTimeSeconds(), Conversation->GetPassingLineDuration()));
}

#if WITH_DEV_AUTOMATION_TESTS
void UVestigeCaptionWidget::SetConversationForTests(UVestigeConversationComponent* Conversation)
{
	Follow(Conversation);
}

FText UVestigeCaptionWidget::GetSpeakerNameForTests() const
{
	return SpeakerNameText->GetText();
}

FText UVestigeCaptionWidget::GetLineForTests() const
{
	return LineText->GetText();
}
#endif
