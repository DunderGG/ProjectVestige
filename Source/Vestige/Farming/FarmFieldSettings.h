// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Farming/FarmTileState.h"
#include "FarmFieldSettings.generated.h"

/**
 * The numbers of the farm fields' rules: for now how much water a tile holds, what watering adds and what a night takes.
 * The same for every field, whichever map it is on, so these are project settings rather than a level's. Project Settings >
 * Game > Vestige Farm Fields, stored in Config/DefaultGame.ini. UFarmFieldSubsystem reads them when it makes a field.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Vestige Farm Fields"))
class UFarmFieldSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UFarmFieldSettings& Get() { return *GetDefault<UFarmFieldSettings>(); }

	/** The water rules with every number at least 1. */
	FFarmWaterRules GetWaterRules() const { return FFarmWaterRules::GetClamped(WaterRules); }

private:
	UPROPERTY(Config, EditAnywhere, Category = "Water", meta = (AllowPrivateAccess = "true"))
	FFarmWaterRules WaterRules;
};
