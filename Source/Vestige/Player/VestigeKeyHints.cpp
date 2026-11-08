// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Player/VestigeKeyHints.h"

#include "InputCoreTypes.h"

#define LOCTEXT_NAMESPACE "VestigeKeyHints"

namespace VestigeKeyHints
{
	FText GetKeyName(const FKey& Key)
	{
		// The engine names these "Gamepad Face Button Left" and so on on Windows, too long for a hint. Its console labels
		// aren't used: the editor sets them from a per-user setting, so the names would differ between machines.
		struct FGamepadLabel
		{
			const FKey& Key;
			FText Label;
		};
		static const FGamepadLabel Labels[] = {
			{EKeys::Gamepad_FaceButton_Bottom, LOCTEXT("FaceButtonBottom", "A")},
			{EKeys::Gamepad_FaceButton_Right, LOCTEXT("FaceButtonRight", "B")},
			{EKeys::Gamepad_FaceButton_Left, LOCTEXT("FaceButtonLeft", "X")},
			{EKeys::Gamepad_FaceButton_Top, LOCTEXT("FaceButtonTop", "Y")},
			{EKeys::Gamepad_LeftShoulder, LOCTEXT("LeftShoulder", "LB")},
			{EKeys::Gamepad_RightShoulder, LOCTEXT("RightShoulder", "RB")},
			{EKeys::Gamepad_LeftTrigger, LOCTEXT("LeftTrigger", "LT")},
			{EKeys::Gamepad_RightTrigger, LOCTEXT("RightTrigger", "RT")},
			{EKeys::Gamepad_LeftThumbstick, LOCTEXT("LeftThumbstick", "LS")},
			{EKeys::Gamepad_RightThumbstick, LOCTEXT("RightThumbstick", "RS")},
			{EKeys::Gamepad_Special_Left, LOCTEXT("SpecialLeft", "View")},
			{EKeys::Gamepad_Special_Right, LOCTEXT("SpecialRight", "Menu")},
			{EKeys::Gamepad_DPad_Up, LOCTEXT("DPadUp", "D-pad Up")},
			{EKeys::Gamepad_DPad_Down, LOCTEXT("DPadDown", "D-pad Down")},
			{EKeys::Gamepad_DPad_Left, LOCTEXT("DPadLeft", "D-pad Left")},
			{EKeys::Gamepad_DPad_Right, LOCTEXT("DPadRight", "D-pad Right")},
		};
		for (const FGamepadLabel& Label : Labels)
		{
			if (Label.Key == Key)
			{
				return Label.Label;
			}
		}
		return Key.GetDisplayName(false);
	}
}

#undef LOCTEXT_NAMESPACE
