// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "VestigePlayerController.generated.h"

/**
 * The player's will (Architecture.md, section 2): makes UVestigeCheatManager its cheat manager outside Shipping. The pawn
 * adds the mapping contexts for its body (moving, looking, farming) while it is possessed; menus will add their own here.
 */
UCLASS()
class AVestigePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AVestigePlayerController();
};