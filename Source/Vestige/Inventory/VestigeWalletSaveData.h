// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "VestigeWalletSaveData.generated.h"

/**
 * UVestigeWalletComponent's part of a save, under Base.Wallet: the player's money. No record (a new game, or a save from
 * before coins) is the starting purse.
 */
USTRUCT()
struct FVestigeWalletSaveData
{
	GENERATED_BODY()

	/** This layout's version (Systems.md, section 1). Raise it when a change would load older parts wrong. */
	static constexpr int32 Version = 1;

	UPROPERTY()
	int32 Money = 0;
};
