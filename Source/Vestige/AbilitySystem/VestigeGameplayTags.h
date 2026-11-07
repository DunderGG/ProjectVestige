// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "NativeGameplayTags.h"

/** The gameplay tags C++ code uses, registered by the engine at startup. Tags only content uses live in DefaultGameplayTags.ini. */
namespace VestigeGameplayTags
{
	/** Asset tag of the ability that uses the focused interactable. Interacting activates the abilities that have it. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Interact);

	/** Asset tag of every tool ability (each tool's behaviour), for finding or cancelling them as a group. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_UseTool);

	/** The set-by-caller magnitude code sets on the stamina cost effect (GE_StaminaCost): the stamina one use takes, as a negative amount. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_StaminaCost);

	/** The set-by-caller magnitude code sets on the late-night drain effect (GE_LateNightDrain): the stamina each period takes, as a negative amount. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_StaminaDrain);

	/** The set-by-caller magnitude code sets on the stamina refill effect (GE_StaminaRefill): the stamina it sets, before the set's clamp. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_StaminaRefill);

	/** Granted by the exhausted effect (GE_Exhausted) while the player is exhausted from a late night. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Exhausted);

	/**
	 * Owned by a tool ability while it is active, and blocks every tool ability, so one tool use runs at a time. Also held while
	 * the use key is down and the ability waits for a tile it can change, so it means "a tool ability is active", not "a use is
	 * under way".
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_UsingTool);

	/** Held while stamina is at zero, when every action takes longer. UVestigeStaminaSet keeps it in step with the value. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_OutOfStamina);

	// Input tags: an input config (UVestigeInputConfig) names the input action for each, and the player character binds by
	// them, so code never names an input asset (Systems.md, section 5).

	/** The parent of every input tag. An input config accepts only tags under it. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input);

	/** Uses the item in hand while held (E). */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_UseTool);

	/** Uses the focused interactable (F). */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Interact);

	/** Selects one hotbar slot (keys 1 to 9). Input_HotbarSlots lists them in slot order. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_HotbarSlot1);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_HotbarSlot2);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_HotbarSlot3);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_HotbarSlot4);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_HotbarSlot5);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_HotbarSlot6);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_HotbarSlot7);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_HotbarSlot8);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_HotbarSlot9);

	/** The hotbar slot tags in slot order: index 0 selects the first slot. */
	inline constexpr const FNativeGameplayTag* Input_HotbarSlots[] = {&Input_HotbarSlot1, &Input_HotbarSlot2, &Input_HotbarSlot3,
		&Input_HotbarSlot4, &Input_HotbarSlot5, &Input_HotbarSlot6, &Input_HotbarSlot7, &Input_HotbarSlot8, &Input_HotbarSlot9};

	/** Selects the next hotbar slot, wrapping around (mouse wheel down, gamepad right shoulder). */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_HotbarNext);

	/** Selects the previous hotbar slot, wrapping around (mouse wheel up, gamepad left shoulder). */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_HotbarPrevious);
}
