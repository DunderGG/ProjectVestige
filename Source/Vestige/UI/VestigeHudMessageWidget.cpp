// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "UI/VestigeHudMessageWidget.h"

#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "UI/VestigeHudMessages.h"

void UVestigeHudMessageWidget::ShowMessage(const FText& Text, double StartTime, double Lifetime)
{
	MessageText->SetText(Text);
	MessageStartTime = StartTime;
	MessageLifetime = Lifetime;
	if (const UWorld* World = GetWorld())
	{
		UpdateOpacity(World->GetTimeSeconds());
	}
	SetVisibility(ESlateVisibility::HitTestInvisible);
	OnMessageShown();
}

void UVestigeHudMessageWidget::Hide()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UVestigeHudMessageWidget::UpdateOpacity(double Now)
{
	SetRenderOpacity(FVestigeHudMessages::GetOpacity(MessageStartTime, Now, MessageLifetime));
}

void UVestigeHudMessageWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// The fade follows the world's clock, so it pauses with the game, as the messages' expiry does.
	if (const UWorld* World = GetWorld())
	{
		UpdateOpacity(World->GetTimeSeconds());
	}
}
