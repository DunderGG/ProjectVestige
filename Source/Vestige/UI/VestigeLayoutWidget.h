// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CommonUserWidget.h"
#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "VestigeLayoutWidget.generated.h"

/** The layers of the root layout, bottom first. A new layer (a modal for confirmations) is added here when a screen needs it. */
enum class EVestigeUILayer : uint8
{
	/** The HUD: input goes to the game while it is the active screen. */
	Game,

	/** Screens that take the input: the dialogue box, later the menus. Painted above the game layer, and given the input first. */
	Menu
};

/**
 * The player's root layout: the base of WBP_Layout (Content/Vestige/UI), an overlay of two Common UI stacks, one per layer.
 * AVestigeHUD makes it, adds it to the screen once and pushes the HUD and the screens onto its layers. It knows no screen, and
 * holds no state besides the stacks (Systems.md, section 6).
 */
UCLASS(Abstract)
class UVestigeLayoutWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * Adds a WidgetClass widget to the top of Layer's stack, which activates it, and returns it. Reuses a widget of that class the
	 * stack made before. Nullptr without a class or a stack. A screen closes by deactivating itself, which pops it.
	 */
	template <typename T> T* PushToLayer(EVestigeUILayer Layer, TSubclassOf<T> WidgetClass)
	{
		UCommonActivatableWidgetStack* Stack = GetLayer(Layer);
		return Stack && WidgetClass ? Stack->AddWidget<T>(WidgetClass) : nullptr;
	}

	/** The stack of Layer, or nullptr when the Blueprint lacks it. */
	UCommonActivatableWidgetStack* GetLayer(EVestigeUILayer Layer) const;

private:
	/** The game layer's stack, which holds the HUD. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonActivatableWidgetStack> GameLayer;

	/** The menu layer's stack, last in the overlay so it paints above the game layer and gets the input first. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonActivatableWidgetStack> MenuLayer;
};
