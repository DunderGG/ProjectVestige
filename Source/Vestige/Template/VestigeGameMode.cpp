// Copyright Epic Games, Inc. All Rights Reserved.

#include "Template/VestigeGameMode.h"

#include "Player/VestigePlayerState.h"
#include "UI/FarmHUD.h"

AVestigeGameMode::AVestigeGameMode()
{
	HUDClass = AFarmHUD::StaticClass();
	PlayerStateClass = AVestigePlayerState::StaticClass();
}
