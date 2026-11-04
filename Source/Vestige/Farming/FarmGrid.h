// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FarmGrid.generated.h"

class AFarmTile;
class UBoxComponent;
class UFarmField;

DECLARE_MULTICAST_DELEGATE(FOnFarmTilesReady);

/**
 * A view of one field (UFarmField) in its level: where the field lies, with the actor's location as one corner, and a tile
 * actor for each of its tiles, spawned at BeginPlay. The field's data and rules live in UFarmFieldSubsystem, which keeps
 * the field when the map closes and grows it on every new day; the grid finds its field there by FieldId, redraws a tile
 * when the field says it changed, and converts between world and grid space.
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

	/** The field this grid shows, or nullptr before BeginPlay or in a world without a game instance. */
	UFarmField* GetField() const { return Field; }

	/**
	 * Makes the field NewSize (columns X, rows Y), with the same corner, keeping the tiles that fit, and spawns a tile for
	 * each. For developer tools such as the benchmark field (UVestigeCheatManager), not for play. OnTilesReady doesn't fire
	 * again.
	 */
	void Resize(const FIntPoint& NewSize);

	/** True once BeginPlay has spawned the tiles. Check it before binding to OnTilesReady, which only fires once. */
	bool AreTilesReady() const { return bTilesReady; }

	/** Broadcast once, at the end of BeginPlay, when every tile has been spawned. */
	FOnFarmTilesReady OnTilesReady;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Spawns a tile of TileClass for each of the field's Width x Height tiles, row by row. */
	void SpawnTiles();

	/** Destroys every tile and empties the list. */
	void DestroyTiles();

	/** Fits the editor outline to Width x Height tiles. */
	void UpdateBounds();

	/** Shows the tile at Coordinates again. Bound to the field's OnTileChanged. */
	void RefreshTile(const FIntPoint& Coordinates);

	/** Shows every tile again. Bound to the field's OnTilesChanged. */
	void RefreshTiles();

	/**
	 * Which field this grid shows: its ID in UFarmFieldSubsystem and in saves, unique across every map. Base content uses
	 * the Base namespace (Base.Farm), a mod's map its own. Changing it on a placed grid starts that field over.
	 */
	UPROPERTY(EditAnywhere, Category = "Farming", meta = (AllowPrivateAccess = "true"))
	FName FieldId = TEXT("Base.Farm");

	/** Number of tiles along the actor's X axis. */
	UPROPERTY(EditAnywhere, Category = "Farming", meta = (AllowPrivateAccess = "true", ClampMin = 1))
	int32 Width = 8;

	/** Number of tiles along the actor's Y axis. */
	UPROPERTY(EditAnywhere, Category = "Farming", meta = (AllowPrivateAccess = "true", ClampMin = 1))
	int32 Height = 8;

	/** Size of one tile in Unreal units (cm). */
	UPROPERTY(EditAnywhere, Category = "Farming", meta = (AllowPrivateAccess = "true", ClampMin = 10))
	float TileSize = 100.f;

	/** Tile actor to spawn, one per tile. A Blueprint subclass of AFarmTile (BP_FarmTile) sets how tiles look. */
	UPROPERTY(EditAnywhere, Category = "Farming", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AFarmTile> TileClass;

	/** Outlines the grid in the editor so it's easy to place. Hidden in game. */
	UPROPERTY(VisibleAnywhere, Category = "Farming")
	TObjectPtr<UBoxComponent> Bounds;

	UPROPERTY()
	TArray<TObjectPtr<AFarmTile>> Tiles;

	/** The field shown, from UFarmFieldSubsystem at BeginPlay. It outlives the grid, so EndPlay stops listening to it. */
	UPROPERTY()
	TObjectPtr<UFarmField> Field;

	bool bTilesReady = false;
};
