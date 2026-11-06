// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

// Only in builds with tests, like UVestigeTestEventRecorder.
#if WITH_TESTS

#include "CoreMinimal.h"
#include "Core/VestigeSaveable.h"
#include "UObject/Object.h"
#include "VestigeTestSaveable.generated.h"

/** The saved state of a UVestigeTestSaveable. */
USTRUCT()
struct FVestigeTestSaveData
{
	GENERATED_BODY()

	static constexpr int32 Version = 1;

	UPROPERTY()
	int32 Value = 0;

	UPROPERTY()
	FName Name;
};

/**
 * An owner of saved state for tests: a number and a name saved under a key and version the test chooses, and a count of
 * how often it was asked to start a new game.
 */
UCLASS(NotBlueprintable, HideDropdown)
class UVestigeTestSaveable : public UObject, public IVestigeSaveable
{
	GENERATED_BODY()

public:
	/** Saves under Key, as an owner whose layout is OwnerVersion. */
	void Setup(FName Key, int32 OwnerVersion = FVestigeTestSaveData::Version);

	FVestigeTestSaveData State;

	/** How many times StartNewGame was called. */
	int32 NewGameCount = 0;

	virtual FName GetSaveKey() const override { return SaveKey; }
	virtual int32 GetSaveVersion() const override { return SaveVersion; }
	virtual FVestigeSaveRecord WriteSaveRecord() const override;
	virtual bool ReadSaveRecord(const FVestigeSaveRecord& Record, int32& OutLeftOutCount) override;
	virtual void StartNewGame() override;

private:
	FName SaveKey;
	int32 SaveVersion = FVestigeTestSaveData::Version;
};

#endif
