// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "AbilitySystem/VestigeGameplayTags.h"

namespace VestigeGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Interact, "Vestige.Ability.Interact", "The ability that uses the focused interactable.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_UseTool, "Vestige.Ability.UseTool", "The ability that uses the tool in hand.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_StaminaCost, "Vestige.Data.StaminaCost", "The stamina an action costs, set by the caller.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_OutOfStamina, "Vestige.State.OutOfStamina", "Stamina is at zero: actions take longer.");
}
