// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Content/VestigeContentData.h"
#include "VestigeToolData.generated.h"

class UStaticMesh;

/** What using a tool does to the targeted tile. A fixed set of farming mechanics that tool assets choose from. */
UENUM(BlueprintType)
enum class EFarmToolAction : uint8
{
	Till,
	Plant,
	Water,
	Harvest
};

/**
 * The definition of one tool: what it does, how it looks in the hand and what it costs to use,
 * plus the ID and name every content asset has. One asset per tool, found through the Asset Manager by its ContentId.
 */
UCLASS()
class UVestigeToolData : public UVestigeContentData
{
	GENERATED_BODY()

public:
	/** The primary asset type tools are registered under in Config/DefaultGame.ini. */
	static const FPrimaryAssetType AssetType;

	EFarmToolAction GetAction() const { return Action; }
	float GetStaminaCost() const { return StaminaCost; }
	float GetUseSeconds() const { return UseSeconds; }

	/** The mesh shown in the farmer's hand, or nullptr if the tool has none or it isn't loaded. */
	UStaticMesh* GetHeldMesh() const;

protected:
	virtual FPrimaryAssetType GetContentType() const override { return AssetType; }

private:
	/** What the tool does to the tile in front of the farmer. */
	UPROPERTY(EditDefaultsOnly, Category = "Farming", meta = (AllowPrivateAccess = "true"))
	EFarmToolAction Action = EFarmToolAction::Till;

	/** Stamina one use takes. A use that changes nothing on the farm costs nothing. */
	UPROPERTY(EditDefaultsOnly, Category = "Farming", meta = (AllowPrivateAccess = "true", ClampMin = 0))
	float StaminaCost = 0.f;

	/** How long one use takes, in seconds, before the next can start. Longer while the player is out of stamina. */
	UPROPERTY(EditDefaultsOnly, Category = "Farming", meta = (AllowPrivateAccess = "true", ClampMin = 0, Units = "s"))
	float UseSeconds = 0.4f;

	/** Mesh held in the right hand. The farmer Blueprint's HeldTool component sets its grip offset and rotation. */
	UPROPERTY(EditDefaultsOnly, Category = "Farming|Visuals", meta = (AllowPrivateAccess = "true", AssetBundles = "Visuals"))
	TSoftObjectPtr<UStaticMesh> HeldMesh;
};
