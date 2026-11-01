// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "NativeGameplayTags.h"

/** The gameplay tags C++ code uses, registered by the engine at startup. Tags only content uses live in DefaultGameplayTags.ini. */
namespace VestigeGameplayTags
{
	/** Asset tag of the ability that uses the focused interactable. Interacting activates the abilities that have it. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Interact);

	/** Asset tag of the ability that uses the tool in hand. Using a tool activates the abilities that have it. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_UseTool);

	/** The set-by-caller magnitude of the stamina cost effect: the stamina one use takes, as a negative amount. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_StaminaCost);

	/** Held while stamina is at zero, when every action takes longer. UVestigeStaminaSet keeps it in step with the value. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_OutOfStamina);
}
