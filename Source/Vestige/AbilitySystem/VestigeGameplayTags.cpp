// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "AbilitySystem/VestigeGameplayTags.h"

namespace VestigeGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Interact, "Vestige.Ability.Interact", "The ability that uses the focused interactable.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_UseTool, "Vestige.Ability.UseTool", "Every tool ability: what a tool does.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_StaminaCost, "Vestige.Data.StaminaCost", "The stamina an action costs, set by the caller.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Data_StaminaDrain, "Vestige.Data.StaminaDrain", "The stamina each period of a drain takes, set by the caller.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_StaminaRefill, "Vestige.Data.StaminaRefill", "The stamina a refill sets, set by the caller.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		State_Exhausted, "Vestige.State.Exhausted", "Exhausted from a late night: the day started with less stamina.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		State_UsingTool, "Vestige.State.UsingTool", "A tool ability is active: no other tool ability can start.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_OutOfStamina, "Vestige.State.OutOfStamina", "Stamina is at zero: actions take longer.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input, "Vestige.Input", "The parent of every input tag, which an input config binds actions to.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_UseTool, "Vestige.Input.UseTool", "Input: use the item in hand while held.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Interact, "Vestige.Input.Interact", "Input: use the focused interactable.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_HotbarSlot1, "Vestige.Input.HotbarSlot.1", "Input: select the first hotbar slot.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_HotbarSlot2, "Vestige.Input.HotbarSlot.2", "Input: select the second hotbar slot.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_HotbarSlot3, "Vestige.Input.HotbarSlot.3", "Input: select the third hotbar slot.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_HotbarSlot4, "Vestige.Input.HotbarSlot.4", "Input: select the fourth hotbar slot.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_HotbarSlot5, "Vestige.Input.HotbarSlot.5", "Input: select the fifth hotbar slot.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_HotbarSlot6, "Vestige.Input.HotbarSlot.6", "Input: select the sixth hotbar slot.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_HotbarSlot7, "Vestige.Input.HotbarSlot.7", "Input: select the seventh hotbar slot.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_HotbarSlot8, "Vestige.Input.HotbarSlot.8", "Input: select the eighth hotbar slot.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_HotbarSlot9, "Vestige.Input.HotbarSlot.9", "Input: select the ninth hotbar slot.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_HotbarNext, "Vestige.Input.HotbarNext", "Input: select the next hotbar slot, wrapping around.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Input_HotbarPrevious, "Vestige.Input.HotbarPrevious", "Input: select the previous hotbar slot, wrapping around.");
}
