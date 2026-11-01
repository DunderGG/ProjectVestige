// Copyright Epic Games, Inc. All Rights Reserved.

#include "Template/VestigeGameMode.h"

#include "Player/VestigePlayerState.h"
#include "UI/VestigeHUD.h"

AVestigeGameMode::AVestigeGameMode()
{
	HUDClass = AVestigeHUD::StaticClass();
	PlayerStateClass = AVestigePlayerState::StaticClass();
}
