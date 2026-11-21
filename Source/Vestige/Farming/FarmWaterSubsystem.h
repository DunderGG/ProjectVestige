// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Core/VestigeSaveable.h"
#include "Farming/FarmWaterStore.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FarmWaterSubsystem.generated.h"

class UVestigeClockSubsystem;
class UVestigeSaveRegistry;

/**
 * The farm's clean water: one store that the well, the watering can and later the sprinkler draw on, whichever map is open. A
 * game-instance subsystem, so the water outlives its map as the fields do. The well gives a daily supply
 * (UFarmWaterSettings) that is back in full each day: what is left is worked out from the clock's day (FFarmWaterStore), so
 * there is no new-day handler and no order to keep against other listeners. Saves what was drawn under Base.FarmWater through
 * UVestigeSaveRegistry. Systems.md, section 24.
 */
UCLASS()
class UFarmWaterSubsystem : public UGameInstanceSubsystem, public IVestigeSaveable
{
	GENERATED_BODY()

public:
	/** The water of World's game instance, or nullptr in a world without one (editor and preview worlds). */
	static UFarmWaterSubsystem* Get(const UWorld* World);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** How much clean water is left today, in waterings: the day's supply less what was drawn. 0 without a clock. */
	int32 GetCleanWaterLeft() const;

	/**
	 * Takes up to Amount of today's clean water and returns how much it took: nothing for 0 or less, never more than is left.
	 * Without a clock nothing is taken.
	 */
	int32 DrawCleanWater(int32 Amount);

	/** Makes the store full: today's water is back to the whole supply. */
	virtual void StartNewGame() override;

	virtual FName GetSaveKey() const override;
	virtual int32 GetSaveVersion() const override;
	virtual FVestigeSaveRecord WriteSaveRecord() const override;
	virtual bool ReadSaveRecord(const FVestigeSaveRecord& Record, int32& OutLeftOutCount) override;

private:
	FFarmWaterStore Store;

	UPROPERTY()
	TObjectPtr<UVestigeClockSubsystem> Clock;

	UPROPERTY()
	TObjectPtr<UVestigeSaveRegistry> SaveRegistry;
};
