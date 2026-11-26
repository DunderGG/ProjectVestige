// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "UI/VestigeLayoutWidget.h"

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
