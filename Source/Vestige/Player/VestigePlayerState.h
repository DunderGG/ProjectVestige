// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "VestigePlayerState.generated.h"

class UVestigeInventoryComponent;

/**
 * The player's data that outlives the pawn: for now the inventory, later the ability system and its stamina.
 * BP_PlayerState sets its values (the starting items), and the game mode names it. Code below the player layer doesn't
 * know this class: it finds the components with FindComponentByClass on the player's APlayerState.
 */
UCLASS()
class AVestigePlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AVestigePlayerState();

	UVestigeInventoryComponent* GetInventory() const { return Inventory; }

private:
	UPROPERTY(VisibleAnywhere, Category = "Inventory", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UVestigeInventoryComponent> Inventory;
};
