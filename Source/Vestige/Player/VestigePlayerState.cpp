// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Player/VestigePlayerState.h"

#include "Inventory/VestigeInventoryComponent.h"

AVestigePlayerState::AVestigePlayerState()
{
	Inventory = CreateDefaultSubobject<UVestigeInventoryComponent>(TEXT("Inventory"));
}
