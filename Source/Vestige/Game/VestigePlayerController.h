// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "VestigePlayerController.generated.h"

class UInputMappingContext;

/**
 * The player's will (Architecture.md, section 2): adds the input mapping contexts the movement and camera use to its
 * local player, and makes UVestigeCheatManager its cheat manager outside Shipping. BP_PlayerController sets the contexts;
 * the player character still adds the context it builds in code for the farming keys.
 */
UCLASS()
class AVestigePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AVestigePlayerController();

protected:
	/** Adds DefaultMappingContexts to the local player's Enhanced Input, once the controller has its input component. */
	virtual void SetupInputComponent() override;

private:
	/** Mapping contexts every local player gets, such as moving and looking (IMC_Default, IMC_MouseLook). */
	UPROPERTY(EditDefaultsOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UInputMappingContext>> DefaultMappingContexts;
};
