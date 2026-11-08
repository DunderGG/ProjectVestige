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

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Feedback_Tool_NoTarget, "Vestige.Feedback.Tool.NoTarget", "Feedback: the farmer faces no farm tile.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Feedback_Tool_NothingInHand, "Vestige.Feedback.Tool.NothingInHand", "Feedback: nothing in hand can be used.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Feedback_Farm_CantTill, "Vestige.Feedback.Farm.CantTill", "Feedback: only grass can be tilled.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Feedback_Farm_NothingToPlant, "Vestige.Feedback.Farm.NothingToPlant", "Feedback: no seed in hand to plant.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Feedback_Farm_CantPlant, "Vestige.Feedback.Farm.CantPlant", "Feedback: seeds need empty tilled soil.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Feedback_Farm_CantWater, "Vestige.Feedback.Farm.CantWater", "Feedback: nothing on the tile can be watered.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Feedback_Farm_NothingRipe, "Vestige.Feedback.Farm.NothingRipe", "Feedback: no ripe crop to harvest.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Feedback_Inventory_Full, "Vestige.Feedback.Inventory.Full", "Feedback: no room in the inventory, so the harvest stays.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Feedback_Stamina_Empty, "Vestige.Feedback.Stamina.Empty", "Feedback: stamina has just run out.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Feedback_Rest_GettingLate, "Vestige.Feedback.Rest.GettingLate", "Feedback: bed after midnight means waking exhausted.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Feedback_Rest_PastMidnight, "Vestige.Feedback.Rest.PastMidnight", "Feedback: stamina drains until the player sleeps.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Feedback_Rest_WokeExhausted, "Vestige.Feedback.Rest.WokeExhausted", "Feedback: the player wakes exhausted.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Feedback_Rest_WokeRested, "Vestige.Feedback.Rest.WokeRested", "Feedback: an early night cleared the exhaustion.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Feedback_Day_Slept, "Vestige.Feedback.Day.Slept", "Feedback: the player slept; value Day, the day that begins.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Feedback_Day_StayedUp, "Vestige.Feedback.Day.StayedUp", "Feedback: the player stayed up; value Day, the day that begins.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Feedback_Save_Loaded, "Vestige.Feedback.Save.Loaded", "Feedback: a save was loaded; value Day, the loaded day.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Feedback_Save_Saved, "Vestige.Feedback.Save.Saved", "Feedback: the game was saved.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Feedback_Save_Failed, "Vestige.Feedback.Save.Failed", "Feedback: the save couldn't be written; the previous one is kept.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Feedback_Save_ContentMissing, "Vestige.Feedback.Save.ContentMissing", "Feedback: saved content no longer exists.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Feedback_Save_Unreadable, "Vestige.Feedback.Save.Unreadable", "Feedback: the save couldn't be read; saving is off.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Feedback_Save_TooNew, "Vestige.Feedback.Save.TooNew", "Feedback: the save is from a newer version; saving is off.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Feedback_Save_PartTooNew, "Vestige.Feedback.Save.PartTooNew", "Feedback: part of the save is newer; saving is off.");

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
