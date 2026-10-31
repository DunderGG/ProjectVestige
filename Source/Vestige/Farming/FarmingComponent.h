// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Farming/FarmToolbar.h"
#include "FarmingComponent.generated.h"

class AFarmGrid;
class AFarmTile;
class UFarmCropData;
class UFarmToolData;
class UVestigeInventoryComponent;
struct FFarmFarmingSaveData;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnFarmToolChanged, const UFarmToolData* /* Tool */);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnFarmTargetTileChanged, const AFarmTile* /* Tile */);

/**
 * Gives its owner, a pawn, farming tools. Works on the tile next to the owner's in the direction the owner aims, takes
 * seeds from and puts harvests into the inventory on the owner's player state, and tells the player what happened
 * through UFarmFeedbackSubsystem.
 */
UCLASS(ClassGroup = (Farming), meta = (BlueprintSpawnableComponent))
class UFarmingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFarmingComponent();

	/** Puts the tool in a toolbar slot (0 is the first) in hand. Does nothing if the slot doesn't exist. */
	void SelectToolbarSlot(int32 Slot);

	/** The tool in hand, or nullptr if its slot is empty. */
	const UFarmToolData* GetCurrentTool() const { return Toolbar.GetCurrentTool(); }
	const FFarmToolbar& GetToolbar() const { return Toolbar; }

	/** Uses the tool in hand on the target tile. Does nothing to the farm without a target or a tool. */
	void UseTool();

	/** The item the Seeds tool plants (FarmItem:Base.TurnipSeeds). */
	const FPrimaryAssetId& GetSeedItemId() const { return SeedItemId; }

	/** The tool in hand, for saving. */
	FFarmFarmingSaveData GetSaveData() const;

	/**
	 * Puts the saved tool back in hand, without replaying any action, and broadcasts OnToolChanged. Returns false if the
	 * saved tool isn't on the toolbar: the first slot is in hand then, with a warning.
	 */
	bool RestoreFromSaveData(const FFarmFarmingSaveData& Data);

	/**
	 * Aims at the neighbouring tile in WorldDirection, snapped to one of the eight grid directions (diagonals included).
	 * The owner's input calls it with the movement direction every frame it is held. A zero direction keeps the current aim,
	 * so the target stays put when the farmer stops. Broadcasts OnTargetTileChanged when the target changes.
	 */
	void AimAt(const FVector& WorldDirection);

	/** The aim as a world direction on the ground, of length 1, for turning the owner to face it. Zero without a grid. */
	FVector GetAimDirection() const;

	/**
	 * The target tile: the neighbour of the owner's tile in the aimed direction, never the tile the owner stands on.
	 * nullptr when that neighbour is outside the grid.
	 */
	const AFarmTile* GetTargetTile() const { return TargetTile.Get(); }

	/** Broadcast whenever the current tool changes, including when a save is loaded. */
	FOnFarmToolChanged OnToolChanged;

	/** Broadcast when the target tile changes, with nullptr when there stops being one. */
	FOnFarmTargetTileChanged OnTargetTileChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void LoadSeedCrop();
	void LoadToolbar();

	/** Puts the tool with this ContentId in hand. Returns false, with the first slot in hand, if no slot holds it. */
	bool RestoreCurrentTool(FName ToolContentId);

	/** The neighbour of the owner's tile one AimStep away, or nullptr if it is outside the grid. */
	AFarmTile* FindTargetTile() const;

	/** Finds the target tile again and broadcasts OnTargetTileChanged if it changed. */
	void UpdateTargetTile();

	/** Bound to the owner's root component, so the target follows the owner from tile to tile without ticking. */
	void FollowOwnerTransform(USceneComponent* UpdatedComponent, EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport);

	/**
	 * The inventory on the owner's player state, or nullptr if the owner isn't possessed or has none. Asked for on each
	 * action rather than kept, because a pawn begins play before it is possessed.
	 */
	UVestigeInventoryComponent* GetInventory() const;

	void Plant(AFarmTile& Tile);
	void Harvest(AFarmTile& Tile);

	/** Tells the player what happened, through UFarmFeedbackSubsystem. */
	void Notify(const FText& Message) const;

	UPROPERTY()
	TObjectPtr<AFarmGrid> Grid;

	TWeakObjectPtr<AFarmTile> TargetTile;

	/** The grid step from the owner's tile to the target: X and Y each -1, 0 or 1, never both 0. */
	FIntPoint AimStep = FIntPoint(1, 0);

	/** The owner's tile when the target was last found, so moving within a tile does no work. */
	FIntPoint OwnerCoordinates = FIntPoint::ZeroValue;

	/** The seed item the Seeds tool plants, by its ID (FarmItem:Base.TurnipSeeds). Its seed fragment names the crop. */
	UPROPERTY(EditDefaultsOnly, Category = "Farming", meta = (AllowPrivateAccess = "true", AllowedTypes = "FarmItem"))
	FPrimaryAssetId SeedItemId;

	/** The crop the seed item grows, loaded with its meshes at BeginPlay. */
	UPROPERTY()
	TObjectPtr<const UFarmCropData> SeedCrop;

	/**
	 * Tool assets in the toolbar slots, in order (FarmTool:Base.Hoe).
	 * Keys 1 to 9 select the first nine; the first is in hand at the start.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Farming", meta = (AllowPrivateAccess = "true", AllowedTypes = "FarmTool"))
	TArray<FPrimaryAssetId> ToolbarIds;

	/** The tools loaded from ToolbarIds at BeginPlay, and the one in hand. */
	UPROPERTY()
	FFarmToolbar Toolbar;
};
