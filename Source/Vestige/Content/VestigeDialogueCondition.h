// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Content/VestigeDialogueContext.h"
#include "UObject/Object.h"
#include "VestigeDialogueCondition.generated.h"

/**
 * Whether a conversation or a reply applies now. A small, named object chosen in a dialogue asset's data, as an item fragment
 * is (Systems.md, section 11). All the conditions of a conversation or reply must hold. A condition is an object inside an
 * asset, so it has no world: a Blueprint condition can't call nodes that need one, and reads what it needs from the context.
 */
UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class UVestigeDialogueCondition : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Extension point: whether this condition holds for Context. Asked for every candidate conversation each time the player
	 * talks, so it must be cheap and change nothing.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Dialogue")
	bool IsMet(const FVestigeDialogueContext& Context) const;
	virtual bool IsMet_Implementation(const FVestigeDialogueContext& Context) const;
};
