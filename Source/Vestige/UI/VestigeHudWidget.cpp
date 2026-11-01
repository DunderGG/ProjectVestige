// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "UI/VestigeHudWidget.h"

#include "Styling/CoreStyle.h"
#include "UI/VestigeHUD.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace VestigeHudStyle
{
	const FLinearColor Backdrop(0.f, 0.f, 0.f, 0.45f);
	const FLinearColor Text(1.f, 1.f, 1.f);
	const FLinearColor Hint(0.8f, 0.8f, 0.8f);
	const FLinearColor Message(1.f, 0.9f, 0.55f);
	const FVector2D ShadowOffset(1.f, 1.f);

	const FName BackdropBrush(TEXT("WhiteBrush"));
	const FName BoldFont(TEXT("Bold"));
	const FName RegularFont(TEXT("Regular"));
	constexpr int32 StatusFontSize = 20;
	constexpr int32 InventoryFontSize = 16;
	constexpr int32 MessageFontSize = 18;
	constexpr int32 HintFontSize = 13;

	// Spacing in Slate units, which scale with the screen's DPI.
	constexpr float ScreenMargin = 24.f;
	constexpr float LineSpacing = 4.f;
	constexpr float MessageBottomMargin = 80.f;
	constexpr float MessageSpacing = 6.f;
	const FMargin PanelPadding(16.f, 10.f);
	const FMargin MessagePadding(16.f, 8.f);
}

void SVestigeHudWidget::Construct(const FArguments& InArgs)
{
	Hud = InArgs._Hud;

	const FSlateBrush* Backdrop = FCoreStyle::Get().GetBrush(VestigeHudStyle::BackdropBrush);

	// clang-format off
	ChildSlot
	[
		SNew(SOverlay)
		.Visibility(EVisibility::HitTestInvisible)

		// Status panel, top left.
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		.Padding(VestigeHudStyle::ScreenMargin)
		[
			SNew(SBorder)
			.BorderImage(Backdrop)
			.BorderBackgroundColor(VestigeHudStyle::Backdrop)
			.Padding(VestigeHudStyle::PanelPadding)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Font(FCoreStyle::GetDefaultFontStyle(VestigeHudStyle::BoldFont, VestigeHudStyle::StatusFontSize))
					.ColorAndOpacity(VestigeHudStyle::Text)
					.ShadowOffset(VestigeHudStyle::ShadowOffset)
					.Text(this, &SVestigeHudWidget::GetStatusText)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, VestigeHudStyle::LineSpacing, 0.f, 0.f)
				[
					SNew(STextBlock)
					.Font(FCoreStyle::GetDefaultFontStyle(VestigeHudStyle::RegularFont, VestigeHudStyle::InventoryFontSize))
					.ColorAndOpacity(VestigeHudStyle::Text)
					.ShadowOffset(VestigeHudStyle::ShadowOffset)
					.Text(this, &SVestigeHudWidget::GetInventoryText)
				]
			]
		]

		// Feedback messages, each in its own box, above the controls hint. AddMessageBoxes fills the list.
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(0.f, 0.f, 0.f, VestigeHudStyle::MessageBottomMargin))
		[
			SAssignNew(MessageList, SVerticalBox)
		]

		// Controls hint, bottom center.
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(0.f, 0.f, 0.f, VestigeHudStyle::ScreenMargin))
		[
			SNew(STextBlock)
			.Font(FCoreStyle::GetDefaultFontStyle(VestigeHudStyle::RegularFont, VestigeHudStyle::HintFontSize))
			.ColorAndOpacity(VestigeHudStyle::Hint)
			.ShadowOffset(VestigeHudStyle::ShadowOffset)
			.Text(this, &SVestigeHudWidget::GetControlsText)
		]
	];
	// clang-format on

	if (const AVestigeHUD* OwningHud = Hud.Get())
	{
		AddMessageBoxes(OwningHud->GetMaxMessages());
	}
}

FText SVestigeHudWidget::GetStatusText() const
{
	const AVestigeHUD* OwningHud = Hud.Get();
	return OwningHud ? OwningHud->GetStatusText() : FText::GetEmpty();
}

FText SVestigeHudWidget::GetInventoryText() const
{
	const AVestigeHUD* OwningHud = Hud.Get();
	return OwningHud ? OwningHud->GetInventoryText() : FText::GetEmpty();
}

FText SVestigeHudWidget::GetControlsText() const
{
	const AVestigeHUD* OwningHud = Hud.Get();
	return OwningHud ? OwningHud->GetControlsText() : FText::GetEmpty();
}

void SVestigeHudWidget::AddMessageBoxes(int32 Count)
{
	const FSlateBrush* Backdrop = FCoreStyle::Get().GetBrush(VestigeHudStyle::BackdropBrush);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		TSharedPtr<SBorder> MessageBox;
		TSharedPtr<STextBlock> MessageText;

		// clang-format off
		MessageList->AddSlot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		.Padding(0.f, Index == 0 ? 0.f : VestigeHudStyle::MessageSpacing, 0.f, 0.f)
		[
			SAssignNew(MessageBox, SBorder)
			.Visibility(EVisibility::Collapsed)
			.BorderImage(Backdrop)
			.BorderBackgroundColor(this, &SVestigeHudWidget::GetMessageBackdropColor, Index)
			.ColorAndOpacity(this, &SVestigeHudWidget::GetMessageContentTint, Index)
			.Padding(VestigeHudStyle::MessagePadding)
			[
				SAssignNew(MessageText, STextBlock)
				.Font(FCoreStyle::GetDefaultFontStyle(VestigeHudStyle::BoldFont, VestigeHudStyle::MessageFontSize))
				.ColorAndOpacity(VestigeHudStyle::Message)
				.ShadowOffset(VestigeHudStyle::ShadowOffset)
			]
		];
		// clang-format on

		MessageBoxes.Add(MessageBox);
		MessageTexts.Add(MessageText);
	}
}

void SVestigeHudWidget::RefreshMessages()
{
	const AVestigeHUD* OwningHud = Hud.Get();
	const TArray<FVestigeHudMessage> NoMessages;
	const TArray<FVestigeHudMessage>& Shown = OwningHud ? OwningHud->GetMessages().GetMessages() : NoMessages;
	for (int32 Index = 0; Index < MessageBoxes.Num(); ++Index)
	{
		const bool bShown = Shown.IsValidIndex(Index);
		MessageTexts[Index]->SetText(bShown ? Shown[Index].Text : FText::GetEmpty());
		MessageBoxes[Index]->SetVisibility(bShown ? EVisibility::HitTestInvisible : EVisibility::Collapsed);
	}
}

FSlateColor SVestigeHudWidget::GetMessageBackdropColor(int32 Index) const
{
	const AVestigeHUD* OwningHud = Hud.Get();
	FLinearColor Color = VestigeHudStyle::Backdrop;
	Color.A *= OwningHud ? OwningHud->GetMessageOpacity(Index) : 0.f;
	return Color;
}

FLinearColor SVestigeHudWidget::GetMessageContentTint(int32 Index) const
{
	const AVestigeHUD* OwningHud = Hud.Get();
	return FLinearColor(1.f, 1.f, 1.f, OwningHud ? OwningHud->GetMessageOpacity(Index) : 0.f);
}
