// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Tests/VestigeTestEventRecorder.h"

#if WITH_TESTS

#include "Content/FarmCropData.h"
#include "Content/VestigeToolData.h"

namespace VestigeTestEventRecorder
{
	FString DescribeContent(const UVestigeContentData* Content)
	{
		return Content ? Content->GetContentId().ToString() : TEXT("None");
	}
}

void UVestigeTestEventRecorder::Record(const FString& Event)
{
	Events.Add(Event);
}

void UVestigeTestEventRecorder::RecordNewDay(int32 Day)
{
	Record(FString::Printf(TEXT("NewDay %d"), Day));
}

void UVestigeTestEventRecorder::RecordCropHarvested(const UFarmCropData* Crop, FIntPoint Tile)
{
	Record(FString::Printf(TEXT("CropHarvested %s %s"), *VestigeTestEventRecorder::DescribeContent(Crop), *Tile.ToString()));
}

void UVestigeTestEventRecorder::RecordToolUsed(const UVestigeToolData* Tool, FIntPoint Tile)
{
	Record(FString::Printf(TEXT("ToolUsed %s %s"), *VestigeTestEventRecorder::DescribeContent(Tool), *Tile.ToString()));
}

#endif
