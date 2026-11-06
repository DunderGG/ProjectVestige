// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "VestigeClockSaveData.generated.h"

/** UVestigeClockSubsystem's part of a save, under Base.Clock: plain values only. The clock fills it and restores from it. */
USTRUCT()
struct FVestigeClockSaveData
{
	GENERATED_BODY()

	/** This layout's version (Systems.md, section 1). Raise it when a change would load older parts wrong. */
	static constexpr int32 Version = 1;

	UPROPERTY()
	int32 Day = 1;

	/** Hours since midnight at the start of the day. */
	UPROPERTY()
	float TimeOfDay = 0.f;
};
