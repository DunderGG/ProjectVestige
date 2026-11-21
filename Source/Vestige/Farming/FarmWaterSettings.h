// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "FarmWaterSettings.generated.h"

/**
 * The numbers of the farm's water: for now how much clean water the well gives a day. The same for every map, so these are
 * project settings. Project Settings > Game > Vestige Farm Water, stored in Config/DefaultGame.ini. UFarmWaterSubsystem reads
 * them on every call, so a change applies at once. Systems.md, section 24.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Vestige Farm Water"))
class UFarmWaterSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UFarmWaterSettings& Get() { return *GetDefault<UFarmWaterSettings>(); }

	/** How much clean water the well gives each day, in waterings; never below 0, which ClampMin only guarantees in the editor. */
	int32 GetWellWaterPerDay() const { return FMath::Max(0, WellWaterPerDay); }

#if WITH_DEV_AUTOMATION_TESTS
	/** For tests only: sets the supply on the default object, which the test puts back afterwards. */
	void SetWellWaterPerDayForTests(int32 InWellWaterPerDay) { WellWaterPerDay = InWellWaterPerDay; }
#endif

private:
	/** How much clean water the well gives each day, in waterings. The store is full again each morning; nothing carries over. */
	UPROPERTY(Config, EditAnywhere, Category = "Well", meta = (AllowPrivateAccess = "true", ClampMin = 0))
	int32 WellWaterPerDay = 24;
};
