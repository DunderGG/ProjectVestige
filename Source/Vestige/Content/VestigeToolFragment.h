// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Content/VestigeItemData.h"
#include "VestigeToolFragment.generated.h"

/**
 * Makes an item a tool: holding it in hand uses the tool ToolId names (its ability, cost and use time). Several items can
 * name one tool, as every seed item names the Seeds tool. An item without one is used with the farmer's hand.
 * Systems.md, section 8.
 */
UCLASS()
class UVestigeToolFragment : public UVestigeItemFragment
{
	GENERATED_BODY()

public:
	/** The tool holding the item uses (VestigeTool:Base.Hoe). */
	const FPrimaryAssetId& GetToolId() const { return ToolId; }

#if WITH_EDITOR
	/** Reports a ToolId that is empty, of another content type, or names no tool. */
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

#if WITH_DEV_AUTOMATION_TESTS
	/** For tests only. Items get their tool in the editor. */
	void SetToolIdForTests(const FPrimaryAssetId& InToolId) { ToolId = InToolId; }
#endif

private:
	/** The tool holding the item uses, by ID, so the item doesn't load the tool's meshes. */
	UPROPERTY(EditDefaultsOnly, Category = "Tool", meta = (AllowPrivateAccess = "true", AllowedTypes = "VestigeTool"))
	FPrimaryAssetId ToolId;
};
