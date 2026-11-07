// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "VestigeAssetManager.generated.h"

class UVestigeGameData;

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
 * be lost. At startup it loads the global game data (UVestigeGameData) once, from GameDataPath in Config/DefaultGame.ini,
 * and keeps it loaded for the whole run (Systems.md, sections 2 and 9).
 */
UCLASS(Config = Game)
class UVestigeAssetManager : public UAssetManager
{
	GENERATED_BODY()

public:
	/**
	 * The IDs that more than one entry of Assets uses, each with every package using it. Each entry is an asset's primary
	 * asset ID and its package. Static and free of the asset registry, so tests can pass made-up assets.
	 */
	static TArray<FVestigeDuplicateContentId> FindDuplicateContentIds(TConstArrayView<TPair<FPrimaryAssetId, FName>> Assets);

	/** The game data loaded at startup, or nullptr if it didn't load or the Asset Manager isn't this class. Use UVestigeGameData::Get. */
	static const UVestigeGameData* GetGameData();

protected:
	/** Loads the game data, after the base class has started its own loading. */
	virtual void StartInitialLoading() override;

	virtual void PostInitialAssetScan() override;

private:
	/** Reads every content asset's ID from the asset registry, without loading the assets, and logs the duplicates. */
	void ReportDuplicateContentIds() const;

	/** Loads GameDataPath into GameData, logging an error if it doesn't load. Synchronous: it runs once, before any world. */
	void LoadGameData();

	/** The game data asset, DA_GameData. Its effects are hard references, so they load with it. */
	UPROPERTY(Config)
	TSoftObjectPtr<UVestigeGameData> GameDataPath;

	/** The loaded game data, kept here so it stays loaded. */
	UPROPERTY(Transient)
	TObjectPtr<const UVestigeGameData> GameData;
};
