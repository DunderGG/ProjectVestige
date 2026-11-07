// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Game/VestigePlayerController.h"

#include "Player/VestigeCheatManager.h"

AVestigePlayerController::AVestigePlayerController()
{
#if UE_WITH_CHEAT_MANAGER
	// The developer console commands. The engine never creates a cheat manager in Shipping builds.
	CheatClass = UVestigeCheatManager::StaticClass();
#endif
}