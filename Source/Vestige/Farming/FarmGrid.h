// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/VestigeInteractable.h"
#include "FarmGrid.generated.h"

class UBoxComponent;
class UFarmField;
class UFarmFieldMeshesComponent;
struct FFarmTileState;

DECLARE_MULTICAST_DELEGATE(FOnFarmFieldReady);

/**
 * A view of one field (UFarmField) in its level: where the field lies, with the actor's location as one corner. The field's
 * data and rules live in UFarmFieldSubsystem, which keeps the field when the map closes and grows it on every new day; the
 * grid finds its field there by FieldId at BeginPlay, has its UFarmFieldMeshesComponent draw it, and converts between world
 * and grid space. There is no actor per tile: a tile is its coordinates in the field. Levels place BP_FarmGrid, which sets
 * the art the field is drawn with.
 * It is also the interactable for its tiles: facing a ripe tile offers Harvest (Systems.md, section 7, harvesting with F),
 * used from InteractionVolume, which covers the field and one tile around it.
 */
UCLASS()
class AFarmGrid : public AActor, public IVestigeInteractable
{
	GENERATED_BODY()

public:
	/** Width of a tile at scale 1, in cm, which tile art is made for. A tile TileSize wide is scaled from it. */
	static constexpr float UnscaledTileSize = 100.f;

	/** The action of the option that harvests the faced ripe tile. */
	static const FName HarvestAction;

	AFarmGrid();

	/**
	 * Offers Harvest, at the tile's centre, while Interactor's targeting component aims at a ripe tile of this grid. A component
	 * lookup and a tile read: asked five times a second while the farmer is near.
	 */
	virtual void GetInteractionOptions_Implementation(AActor* Interactor, TArray<FVestigeInteractionOption>& OutOptions) const override;

	/** Harvest: asks Interactor's farming component to harvest the tile it aims at. */
	virtual void Interact_Implementation(AActor* Interactor, const FVestigeInteractionOption& Option) override;

	/**
	 * Returns the world's farm grid, or nullptr if the level has none. The game has a single field.
	 * Searches the world, so call it once and keep the result.
	 */
	static AFarmGrid* Find(const UWorld* World);

	/** The field's tile at grid coordinates (column X, row Y), or nullptr outside the field or without one. */
	const FFarmTileState* GetTile(const FIntPoint& Coordinates) const;

	/** A tile's place in the world: centered on the tile, turned with the grid, scaled from UnscaledTileSize to TileSize. */
	FTransform GetTileTransform(const FIntPoint& Coordinates) const;

	/** The center of the tile at Coordinates in a grid's own space, on the ground, for tiles InTileSize wide. */
	static FVector GetLocalTileCenter(const FIntPoint& Coordinates, float InTileSize);

	/** The grid coordinates (column X, row Y) of a world location, ignoring height. Outside the grid they lie beyond its edges. */
	FIntPoint GetCoordinatesAt(const FVector& WorldLocation) const;

	/** A world direction in the grid's own space, on the ground: X along the columns, Y along the rows. */
	FVector2D ToGridDirection(const FVector& WorldDirection) const;

	/** A grid step (column, row) as a world direction on the ground, of length 1. */
	FVector ToWorldDirection(const FIntPoint& Step) const;

	/** Number of tiles along the actor's X axis. */
	int32 GetWidth() const { return Width; }

	/** The field this grid shows, or nullptr before BeginPlay or in a world without a game instance. */
	UFarmField* GetField() const { return Field; }

	/** What draws the field. */
	const UFarmFieldMeshesComponent* GetMeshes() const { return Meshes; }

	/**
	 * Makes the field NewSize (columns X, rows Y), with the same corner, keeping the tiles that fit; the field's change
	 * event redraws it. For developer tools such as the benchmark field (UVestigeCheatManager), not for play. OnFieldReady
	 * doesn't fire again.
	 */
	void Resize(const FIntPoint& NewSize);

	/** True once BeginPlay has looked for the field. Check it before binding to OnFieldReady, which only fires once. */
	bool IsFieldReady() const { return bFieldReady; }

	/** Broadcast once, at the end of BeginPlay, when the grid has its field and draws it. */
	FOnFarmFieldReady OnFieldReady;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Fits the editor outline to Width x Height tiles, and the interaction volume to them and one tile around. */
	void UpdateBounds();

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

	/** Outlines the grid in the editor so it's easy to place. Hidden in game. */
	UPROPERTY(VisibleAnywhere, Category = "Farming")
	TObjectPtr<UBoxComponent> Bounds;

	/**
	 * Where the field's tiles can be used from: the field and one tile around it, from 50 cm below the grid to 200 cm above,
	 * on the interaction channel only.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Farming")
	TObjectPtr<UBoxComponent> InteractionVolume;

	/** Draws the field with instanced meshes. Its art is set in BP_FarmGrid. */
	UPROPERTY(VisibleAnywhere, Category = "Farming", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UFarmFieldMeshesComponent> Meshes;

	/** The field shown, from UFarmFieldSubsystem at BeginPlay. It outlives the grid, so EndPlay stops listening to it. */
	UPROPERTY()
	TObjectPtr<UFarmField> Field;

	bool bFieldReady = false;
};
