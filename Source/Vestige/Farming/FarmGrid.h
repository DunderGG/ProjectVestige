// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FarmGrid.generated.h"

class AVestigeDayCycle;
class AFarmTile;
class UFarmCropData;
class UBoxComponent;
struct FFarmTileSaveData;

DECLARE_MULTICAST_DELEGATE(FOnFarmTilesReady);

/**
 * A rectangle of farm tiles, spawned at BeginPlay with the actor's location as one corner.
 * Advances every tile when the world's AVestigeDayCycle starts a new day.
 */
UCLASS()
class AFarmGrid : public AActor
{
	GENERATED_BODY()

public:
	AFarmGrid();

	/**
	 * Returns the world's farm grid, or nullptr if the level has none. The game has a single field.
	 * Searches the world, so call it once and keep the result.
	 */
	static AFarmGrid* Find(const UWorld* World);

	/** Returns the tile under a world location, ignoring height, or nullptr if it's outside the grid. */
	AFarmTile* GetTileAtLocation(const FVector& WorldLocation) const;

	/** Returns the tile at grid coordinates (column X, row Y), or nullptr if they are outside the grid. */
	AFarmTile* GetTileAt(const FIntPoint& Coordinates) const;

	/** The grid coordinates (column X, row Y) of a world location, ignoring height. Outside the grid they lie beyond its edges. */
	FIntPoint GetCoordinatesAt(const FVector& WorldLocation) const;

	/** A world direction in the grid's own space, on the ground: X along the columns, Y along the rows. */
	FVector2D ToGridDirection(const FVector& WorldDirection) const;

	/** A grid step (column, row) as a world direction on the ground, of length 1. */
	FVector ToWorldDirection(const FIntPoint& Step) const;

	/** Every tile, row by row. Empty until BeginPlay has spawned them. */
	const TArray<TObjectPtr<AFarmTile>>& GetTiles() const { return Tiles; }

	/** Number of tiles along the actor's X axis. */
	int32 GetWidth() const { return Width; }

	/** Every tile's saved state with its coordinates, for saving. */
	TArray<FFarmTileSaveData> GetTilesSaveData() const;

	/**
	 * Restores the tiles a save mentions, each found by its coordinates, with its crop looked up in Crops by ContentId.
	 * Tiles the save doesn't mention keep their state. Entries outside the grid are skipped, and a crop missing from Crops
	 * leaves its tile as empty tilled soil, each with a warning. Returns how many entries it couldn't fully restore.
	 */
	int32 RestoreTilesFromSaveData(const TArray<FFarmTileSaveData>& SavedTiles, const TMap<FName, const UFarmCropData*>& Crops);

	/** True once BeginPlay has spawned the tiles. Check it before binding to OnTilesReady, which only fires once. */
	bool AreTilesReady() const { return bTilesReady; }

	/** Broadcast once, at the end of BeginPlay, when every tile has been spawned. */
	FOnFarmTilesReady OnTilesReady;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Spawns Width x Height tiles of TileClass, row by row. */
	void SpawnTiles();

	/** Grows watered crops and dries the soil on every tile. */
	void AdvanceTiles(int32 NewDay);

	/** Number of tiles along the actor's X axis. */
	UPROPERTY(EditAnywhere, Category = "Farming", meta = (AllowPrivateAccess = "true", ClampMin = 1))
	int32 Width = 8;

	/** Number of tiles along the actor's Y axis. */
	UPROPERTY(EditAnywhere, Category = "Farming", meta = (AllowPrivateAccess = "true", ClampMin = 1))
	int32 Height = 8;

	/** Size of one tile in Unreal units (cm). */
	UPROPERTY(EditAnywhere, Category = "Farming", meta = (AllowPrivateAccess = "true", ClampMin = 10))
	float TileSize = 100.f;

	/** Tile actor to spawn. A Blueprint subclass of AFarmTile (BP_FarmTile) sets how tiles look. */
	UPROPERTY(EditAnywhere, Category = "Farming", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AFarmTile> TileClass;

	/** Outlines the grid in the editor so it's easy to place. Hidden in game. */
	UPROPERTY(VisibleAnywhere, Category = "Farming")
	TObjectPtr<UBoxComponent> Bounds;

	UPROPERTY()
	TArray<TObjectPtr<AFarmTile>> Tiles;

	/** The day cycle this grid listens to, so EndPlay can stop listening. */
	TWeakObjectPtr<AVestigeDayCycle> DayCycle;

	bool bTilesReady = false;
};
