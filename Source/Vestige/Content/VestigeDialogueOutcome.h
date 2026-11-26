// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Content/VestigeDialogueContext.h"
#include "UObject/Object.h"
#include "VestigeDialogueOutcome.generated.h"

/**
 * What showing a line or choosing a reply does: a small, named object chosen in a dialogue asset's data, as a condition is
 * (Systems.md, section 11). None is built yet; the trust track adds the first (trust, flags). An outcome changes state only
 * through its owners' functions, never by reaching into another class's data.
 */
UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class UVestigeDialogueOutcome : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Extension point: does what this outcome does for Context. Called when its line is shown, or when its reply is chosen. Like
	 * a condition, it has no world of its own.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Dialogue")
	void Apply(const FVestigeDialogueContext& Context) const;
	virtual void Apply_Implementation(const FVestigeDialogueContext& Context) const;
};
