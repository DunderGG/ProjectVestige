// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Game/VestigePlayerController.h"

#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Player/VestigeCheatManager.h"

namespace VestigePlayerControllerInput
{
	/** The default contexts' priority, below the character's farming keys, which win if a key is ever mapped twice. */
	constexpr int32 DefaultContextPriority = 0;
}

AVestigePlayerController::AVestigePlayerController()
{
#if UE_WITH_CHEAT_MANAGER
	// The developer console commands. The engine never creates a cheat manager in Shipping builds.
	CheatClass = UVestigeCheatManager::StaticClass();
#endif
}

void AVestigePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!IsLocalPlayerController())
	{
		return;
	}
	UEnhancedInputLocalPlayerSubsystem* EnhancedInput = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (!EnhancedInput)
	{
		return;
	}
	for (const UInputMappingContext* Context : DefaultMappingContexts)
	{
		if (Context)
		{
			EnhancedInput->AddMappingContext(Context, VestigePlayerControllerInput::DefaultContextPriority);
		}
	}
}
