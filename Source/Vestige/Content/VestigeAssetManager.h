// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "VestigeAssetManager.generated.h"

/** A content ID that more than one asset uses, with the package of every asset that uses it. */
struct FVestigeDuplicateContentId
{
	FPrimaryAssetId Id;
	TArray<FName> Packages;
};

/**
 * The project's Asset Manager, set as the engine's in Config/DefaultEngine.ini. Once the Asset Manager has scanned the
 * content, it checks every content asset (UVestigeContentData) for duplicate content IDs and logs an error naming every
 * asset that shares one. Two assets with the same ID collapse into one primary asset, so one of them would silently
 * be lost. Startup work that every system needs, such as global game data, belongs here too (Systems.md, section 2).
 */
UCLASS()
class UVestigeAssetManager : public UAssetManager
{
	GENERATED_BODY()

public:
	/**
	 * The IDs that more than one entry of Assets uses, each with every package using it. Each entry is an asset's primary
	 * asset ID and its package. Static and free of the asset registry, so tests can pass made-up assets.
	 */
	static TArray<FVestigeDuplicateContentId> FindDuplicateContentIds(TConstArrayView<TPair<FPrimaryAssetId, FName>> Assets);

protected:
	virtual void PostInitialAssetScan() override;

private:
	/** Reads every content asset's ID from the asset registry, without loading the assets, and logs the duplicates. */
	void ReportDuplicateContentIds() const;
};
