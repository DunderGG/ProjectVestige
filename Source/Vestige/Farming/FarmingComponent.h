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
class UVestigeToolData;
class UAbilitySystemComponent;
class UVestigeInventoryComponent;
struct FAbilityEndedData;
struct FFarmFarmingSaveData;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnFarmToolChanged, const UVestigeToolData* /* Tool */);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnFarmTargetTileChanged, const AFarmTile* /* Tile */);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFarmCropHarvested, const UFarmCropData*, Crop, const AFarmTile*, Tile);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFarmToolUsed, const UVestigeToolData*, Tool, const AFarmTile*, Tile);

/**
 * Gives its owner, a pawn, farming tools. Works on the tile next to the owner's in the direction the owner aims, takes
 * seeds from and puts harvests into the inventory on the owner's player state, and tells the player what happened
 * through UVestigeFeedbackSubsystem.
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
	const UVestigeToolData* GetCurrentTool() const { return Toolbar.GetCurrentTool(); }
	const FFarmToolbar& GetToolbar() const { return Toolbar; }

	/**
	 * Uses the tool in hand on the target tile, through the use-tool ability on the owner's ability system: the ability
	 * decides whether a use can start now and what it costs, and calls UseToolOnTarget. Does nothing while the last use
	 * is still under way, or without an ability system.
	 */
	void UseTool();

	/**
	 * The use key is pressed: uses the tool once, like UseTool, and keeps using it while the key is held. Each later
	 * use starts when the last one has finished or the target tile changes, and only if it would change the target
	 * tile; a use that would change nothing is skipped quietly until the next tile, so only the press posts its message.
	 */
	void StartUsingTool();

	/** The use key is released: no more uses start. A use under way still finishes. */
	void StopUsingTool();

	/** Whether the use key is held, so uses repeat. */
	bool IsToolUseHeld() const { return bToolUseHeld; }

	/**
	 * Applies the tool in hand to the target tile, and tells the player what happened. Returns false when nothing on the
	 * farm changed (no target, no tool, out of seeds, nothing to water...), so the use costs nothing. Called by the
	 * use-tool ability; everything else calls UseTool.
	 */
	bool UseToolOnTarget();

	/** The item the Seeds tool plants (VestigeItem:Base.TurnipSeeds). */
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

	/**
	 * Extension point: broadcast when the owner harvests Crop from Tile, once the harvest is in the bag and the tile is
	 * tilled soil again. Before OnToolUsed for the same use.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Farming")
	FOnFarmCropHarvested OnCropHarvested;

	/**
	 * Extension point: broadcast when a use of Tool changed the farm at Tile, after the change. A use that changes
	 * nothing (no target, nothing to water, out of seeds, a full bag) isn't broadcast, as it costs nothing.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Farming")
	FOnFarmToolUsed OnToolUsed;

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

	/** Whether the inventory holds a seed item the Seeds tool can plant. */
	bool HasSeedToPlant() const;

	/** Whether the inventory has room for what Crop yields. */
	bool HasRoomForHarvest(const UFarmCropData& Crop) const;

	/** Starts the next use while the use key is held, if it would change the target tile. */
	void RepeatToolUse();

	/** Whether Tool's action would change Tile now, asking the same rules ApplyTool follows, but changing nothing. */
	bool WouldToolChange(const UVestigeToolData& Tool, const AFarmTile& Tile) const;

	/** Repeats the tool use once a use-tool ability has ended. Bound to the owner's ability system's OnAbilityEnded. */
	void ContinueAfterUse(const FAbilityEndedData& EndedData);

	/** Listens for the end of uses on AbilitySystem, and stops listening to the one before. */
	void FollowAbilitySystem(UAbilitySystemComponent* AbilitySystem);

	/** Applies Tool's action to Tile. Each of these returns whether the tile changed. */
	bool ApplyTool(const UVestigeToolData& Tool, AFarmTile& Tile);
	bool Till(AFarmTile& Tile) const;
	bool Plant(AFarmTile& Tile);
	bool Water(AFarmTile& Tile) const;
	bool Harvest(AFarmTile& Tile);

	/** Tells the player what happened, through UVestigeFeedbackSubsystem. */
	void Notify(const FText& Message) const;

	UPROPERTY()
	TObjectPtr<AFarmGrid> Grid;

	TWeakObjectPtr<AFarmTile> TargetTile;

	/** The grid step from the owner's tile to the target: X and Y each -1, 0 or 1, never both 0. */
	FIntPoint AimStep = FIntPoint(1, 0);

	/** The owner's tile when the target was last found, so moving within a tile does no work. */
	FIntPoint OwnerCoordinates = FIntPoint::ZeroValue;

	/** Set while the use key is held. */
	bool bToolUseHeld = false;

	/** The ability system whose ended uses repeat the tool use, found on the first press: the pawn begins play unpossessed. */
	TWeakObjectPtr<UAbilitySystemComponent> FollowedAbilitySystem;

	/** The seed item the Seeds tool plants, by its ID (VestigeItem:Base.TurnipSeeds). Its seed fragment names the crop. */
	UPROPERTY(EditDefaultsOnly, Category = "Farming", meta = (AllowPrivateAccess = "true", AllowedTypes = "VestigeItem"))
	FPrimaryAssetId SeedItemId;

	/** The crop the seed item grows, loaded with its meshes at BeginPlay. */
	UPROPERTY()
	TObjectPtr<const UFarmCropData> SeedCrop;

	/**
	 * Tool assets in the toolbar slots, in order (VestigeTool:Base.Hoe).
	 * Keys 1 to 9 select the first nine; the first is in hand at the start.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Farming", meta = (AllowPrivateAccess = "true", AllowedTypes = "VestigeTool"))
	TArray<FPrimaryAssetId> ToolbarIds;

	/** The tools loaded from ToolbarIds at BeginPlay, and the one in hand. */
	UPROPERTY()
	FFarmToolbar Toolbar;
};
