// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

// Only in builds with tests. WITH_TESTS rather than the tests' WITH_DEV_AUTOMATION_TESTS, because it is the one the header
// tool accepts around reflected types.
#if WITH_TESTS

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "VestigeTestEventRecorder.generated.h"

class UFarmCropData;
class UVestigeToolData;

/**
 * Listens to Blueprint events the way a Blueprint does, through functions bound to the dynamic delegates, and writes down
 * each event it hears in order, so tests can check what a Blueprint listening to the extension points would get.
 */
UCLASS(NotBlueprintable, HideDropdown)
class UVestigeTestEventRecorder : public UObject
{
	GENERATED_BODY()

public:
	/** Writes down an event a test hears some other way, such as through a native delegate, to check the order. */
	void Record(const FString& Event);

	/** Every event heard so far, oldest first, as "NewDay 2", "CropHarvested Base.Turnip X=3 Y=0", "ToolUsed Base.Hoe X=3 Y=0". */
	const TArray<FString>& GetEvents() const { return Events; }

	UFUNCTION()
	void RecordNewDay(int32 Day);

	UFUNCTION()
	void RecordCropHarvested(const UFarmCropData* Crop, FIntPoint Tile);

	UFUNCTION()
	void RecordToolUsed(const UVestigeToolData* Tool, FIntPoint Tile);

private:
	TArray<FString> Events;
};

#endif
