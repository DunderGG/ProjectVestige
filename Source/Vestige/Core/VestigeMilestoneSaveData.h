// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "VestigeMilestoneSaveData.generated.h"

/**
 * UVestigeMilestoneSubsystem's part of a save, under Base.Milestones: the milestones reached, as their tag names, sorted so the
 * same state writes the same bytes. A tag that no longer exists is skipped on load.
 */
USTRUCT()
struct FVestigeMilestoneSaveData
{
	GENERATED_BODY()

	/** This layout's version (Systems.md, section 1). Raise it when a change would load older parts wrong. */
	static constexpr int32 Version = 1;

	UPROPERTY()
	TArray<FName> Reached;
};
