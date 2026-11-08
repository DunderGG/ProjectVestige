// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"

struct FKey;

/** A key the player can press and what it does, for on-screen hints. */
struct FVestigeKeyHint
{
	FText Key;
	FText Action;
};

/** The device key hints name the keys of: the one the player used last (Systems.md, section 5, gamepad key hints). */
enum class EVestigeHintDevice : uint8
{
	KeyboardAndMouse,
	Gamepad
};

namespace VestigeKeyHints
{
	/**
	 * The short name a hint shows for Key. Gamepad buttons get the Xbox labels players on Windows expect ("X", "LB"), until
	 * controller data gives each gamepad its own icons; every other key is its short display name, as "E" or "1".
	 */
	FText GetKeyName(const FKey& Key);
}
