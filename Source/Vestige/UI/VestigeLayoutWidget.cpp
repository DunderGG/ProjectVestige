// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "UI/VestigeLayoutWidget.h"

void UVestigeLayoutWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// As Lyra's UPrimaryGameLayout does for every layer: while a stack's transition runs, the gamepad's focus can't move to the
	// widget just pushed, so the layers never have one, whatever the Blueprint says.
	constexpr double NoTransitionSeconds = 0.0;
	for (UCommonActivatableWidgetStack* Stack : {GameLayer.Get(), MenuLayer.Get()})
	{
		if (Stack)
		{
			Stack->SetTransitionDuration(NoTransitionSeconds);
		}
	}
}

UCommonActivatableWidgetStack* UVestigeLayoutWidget::GetLayer(EVestigeUILayer Layer) const
{
	switch (Layer)
	{
	case EVestigeUILayer::Game:
		return GameLayer;
	case EVestigeUILayer::Menu:
		return MenuLayer;
	}
	return nullptr;
}
