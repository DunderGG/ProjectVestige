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

	/** The set-by-caller magnitude code sets on the late-night drain effect (GE_LateNightDrain): the stamina one game minute takes, as a negative amount. */
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

	// Feedback tags: what gameplay reports for the player through UVestigeFeedbackSubsystem::Report. Presentation picks the
	// response to each (DA_FeedbackResponses on BP_HUD), so gameplay never writes the player's text (Systems.md, section 15).

	/** The tool ability, on the press: the farmer faces no farm tile. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Feedback_Tool_NoTarget);

	/** The farming component, on the press: the hand holds nothing that can be used. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Feedback_Tool_NothingInHand);

	/** The till ability, on the press: the tile isn't grass. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Feedback_Farm_CantTill);

	/** The plant ability, on the press: no seed is in hand. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Feedback_Farm_NothingToPlant);

	/** The plant ability, on the press: the tile isn't empty tilled soil. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Feedback_Farm_CantPlant);

	/** The water ability, on the press: nothing on the tile can be watered. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Feedback_Farm_CantWater);

	/** The harvest ability, on the press: no ripe crop on the tile. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Feedback_Farm_NothingRipe);

	/** The harvest ability, on the press: the inventory has no room for the harvest, which stays in the ground. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Feedback_Inventory_Full);

	/** The stamina set: stamina has just reached zero. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Feedback_Stamina_Empty);

	/** The rest component, at its warning hour: going to bed after midnight will leave the player exhausted. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Feedback_Rest_GettingLate);

	/** The rest component, at its late-night hour: stamina drains until the player sleeps. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Feedback_Rest_PastMidnight);

	/** The rest component: the player wakes exhausted after a late night. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Feedback_Rest_WokeExhausted);

	/** The rest component: a night that ended before midnight cleared the exhaustion. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Feedback_Rest_WokeRested);

	/** The game mode: the player slept and a new day begins. Values Day, Season and DayOfSeason: the day that begins. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Feedback_Day_Slept);

	/**
	 * The game mode: the player stayed up until the day ended and a new day begins. Values Day, Season and DayOfSeason: the day
	 * that begins.
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Feedback_Day_StayedUp);

	/**
	 * The game mode, on the first morning of a season: the season has begun. Values Day, Season and DayOfSeason of that day, and
	 * PreviousSeason, the season that ended.
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Feedback_Season_Started);

	/** The save subsystem: a save was loaded. Values Day, Season and DayOfSeason: the loaded day. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Feedback_Save_Loaded);

	/** The save subsystem: the game was saved. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Feedback_Save_Saved);

	/** The save subsystem: the save couldn't be written, and the previous one is kept. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Feedback_Save_Failed);

	/** The save subsystem: content the save names no longer exists and was left out. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Feedback_Save_ContentMissing);

	/** The save subsystem: no save slot could be read, so saving is off until a new game. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Feedback_Save_Unreadable);

	/** The save subsystem: the save is from a newer version of the game, so saving is off until a new game. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Feedback_Save_TooNew);

	/** The save subsystem: part of the save is from a newer version of the game, so saving is off until a new game. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Feedback_Save_PartTooNew);

	/** The parent of every season tag. The seasons themselves are data: the calendar in DA_GameData names them (Systems.md, section 4). */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Season);

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
