// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "FarmToolbar.generated.h"

class UVestigeToolData;

/**
 * The player's numbered tool slots and which slot is in hand. Holds tool definitions only;
 * UFarmingComponent fills it and uses the tools. Kept apart from the component so its rules can be tested without a level.
 */
USTRUCT()
struct FFarmToolbar
{
	GENERATED_BODY()

public:
	/** Replaces every slot and selects the first one. A nullptr entry is an empty slot. */
	void SetTools(TArray<TObjectPtr<const UVestigeToolData>> InTools);

	/** Returns false, keeping the current slot, if Slot is outside the toolbar. */
	bool SelectSlot(int32 Slot);

	/** Selects the first slot holding the tool with this ContentId. Returns false, keeping the current slot, if none does. */
	bool SelectTool(FName ToolContentId);

	/** The tool in the current slot, or nullptr if that slot is empty or the toolbar has no slots. */
	const UVestigeToolData* GetCurrentTool() const;

	int32 GetCurrentSlot() const { return CurrentSlot; }
	const TArray<TObjectPtr<const UVestigeToolData>>& GetTools() const { return Tools; }

private:
	UPROPERTY()
	TArray<TObjectPtr<const UVestigeToolData>> Tools;

	int32 CurrentSlot = 0;
};
