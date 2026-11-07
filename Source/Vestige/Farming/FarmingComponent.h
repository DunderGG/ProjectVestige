// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/VestigeSaveable.h"
#include "Farming/FarmToolbar.h"
#include "GameplayAbilitySpecHandle.h"
#include "FarmingComponent.generated.h"

class AController;
class AFarmGrid;
class APawn;
class UAbilitySystemComponent;
class UFarmCropData;
class UVestigeToolData;
struct FFarmFarmingSaveData;
struct FGameplayAbilitySpec;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnFarmToolChanged, const UVestigeToolData* /* Tool */);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnFarmTargetTileChanged, const TOptional<FIntPoint>& /* Tile */);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnFarmToolUseHeldChanged, bool /* bHeld */);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFarmCropHarvested, const UFarmCropData*, Crop, FIntPoint, Tile);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFarmToolUsed, const UVestigeToolData*, Tool, FIntPoint, Tile);

/**
 * Gives its owner, a pawn, farming tools: the toolbar and the tool in hand, the tile the owner aims at (the neighbour of
 * the owner's tile in the aimed direction), and the seed item the Seeds tool plants. What a tool does is its ability
 * (UVestigeToolData::GetAbility, a UVestigeGameplayAbility_UseTool subclass): this component grants the ability of the tool
 * in hand on the ability system the owner reaches (the player state's), with the tool asset as its source object, and
 * passes the use key's press and release to it. It never calls the abilities; they read the target and the seed item here
 * and report what they did, which this component broadcasts as the extension points OnCropHarvested and OnToolUsed.
 * Saves the tool in hand under Base.Farming through UVestigeSaveRegistry while it is in play.
 */
UCLASS(ClassGroup = (Farming), meta = (BlueprintSpawnableComponent))
class UFarmingComponent : public UActorComponent, public IVestigeSaveable
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
	 * Uses the tool in hand once on the target tile, by activating its ability without pressing the use key: the ability
	 * decides whether a use can start now and what it costs. Does nothing while a tool use is under way, or without an
	 * ability system. For tests and console commands; the use key calls StartUsingTool.
	 */
	void UseTool();

	/**
	 * The use key is pressed: activates the tool's ability with its input pressed, so it uses the tool and keeps using it
	 * while the key is held (UVestigeGameplayAbility_UseTool says when).
	 */
	void StartUsingTool();

	/** The use key is released: the tool's ability ends once a use under way has finished. */
	void StopUsingTool();

	/** Whether the use key is held. The farmer walks slower meanwhile. */
	bool IsToolUseHeld() const { return bToolUseHeld; }

	/** The item the Seeds tool plants (VestigeItem:Base.TurnipSeeds). */
	const FPrimaryAssetId& GetSeedItemId() const { return SeedItemId; }

	/** The crop the seed item grows, loaded with its meshes at BeginPlay, or nullptr if the seed item names none. */
	const UFarmCropData* GetSeedCrop() const { return SeedCrop; }

	/** A tool ability changed the tile at Tile with Tool: broadcasts OnToolUsed. Called by the tool abilities only. */
	void ReportToolUsed(const UVestigeToolData& Tool, const FIntPoint& Tile);

	/** A tool ability harvested Crop from the tile at Tile: broadcasts OnCropHarvested. Called by the tool abilities only. */
	void ReportCropHarvested(const UFarmCropData& Crop, const FIntPoint& Tile);

	/** The tool in hand, for saving. */
	FFarmFarmingSaveData GetSaveData() const;

	/**
	 * Puts the saved tool back in hand, without replaying any action, and broadcasts OnToolChanged. Returns false if the
	 * saved tool isn't on the toolbar: the first slot is in hand then, with a warning.
	 */
	bool RestoreFromSaveData(const FFarmFarmingSaveData& Data);

	virtual FName GetSaveKey() const override;
	virtual int32 GetSaveVersion() const override;
	virtual FVestigeSaveRecord WriteSaveRecord() const override;
	virtual bool ReadSaveRecord(const FVestigeSaveRecord& Record, int32& OutLeftOutCount) override;

	/** Puts the first toolbar slot's tool in hand and broadcasts OnToolChanged. */
	virtual void StartNewGame() override;

	/**
	 * Aims at the neighbouring tile in WorldDirection, snapped to one of the eight grid directions (diagonals included).
	 * The owner's input calls it with the movement direction every frame it is held. A zero direction keeps the current aim,
	 * so the target stays put when the farmer stops. Broadcasts OnTargetTileChanged when the target changes.
	 */
	void AimAt(const FVector& WorldDirection);

	/** The aim as a world direction on the ground, of length 1, for turning the owner to face it. Zero without a grid. */
	FVector GetAimDirection() const;

	/**
	 * The target tile's coordinates in the grid's field: the neighbour of the owner's tile in the aimed direction, never the
	 * tile the owner stands on. Unset when that neighbour is outside the field.
	 */
	const TOptional<FIntPoint>& GetTargetTile() const { return TargetTile; }

	/** The level's farm grid, found at BeginPlay, or nullptr if the level has none. */
	const AFarmGrid* GetGrid() const { return Grid; }

	/** Broadcast whenever the current tool changes, including when a save is loaded. */
	FOnFarmToolChanged OnToolChanged;

	/** Broadcast when the target tile changes, unset when there stops being one. */
	FOnFarmTargetTileChanged OnTargetTileChanged;

	/** Broadcast when the use key is pressed or released, with whether it is now held. The farmer walks slower meanwhile. */
	FOnFarmToolUseHeldChanged OnToolUseHeldChanged;

	/**
	 * Extension point: broadcast when the owner harvests Crop from the tile at the coordinates Tile (column X, row Y) of the
	 * grid's field, once the harvest is in the bag and the tile is tilled soil again. Before OnToolUsed for the same use.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Farming")
	FOnFarmCropHarvested OnCropHarvested;

	/**
	 * Extension point: broadcast when a use of Tool changed the tile at the coordinates Tile (column X, row Y) of the grid's
	 * field, after the change. A use that changes nothing (no target, nothing to water, out of seeds, a full bag) isn't
	 * broadcast, as it costs nothing.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Farming")
	FOnFarmToolUsed OnToolUsed;

protected:
	/** Loads the seed crop and the toolbar, takes its saved tool in hand, then finds the grid and aims. */
	virtual void BeginPlay() override;

	/** Keeps the tool in hand for the next map, takes its ability back, then stops following the grid and the owner. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void LoadSeedCrop();
	void LoadToolbar();

	/** Puts the tool with this ContentId in hand. Returns false, with the first slot in hand, if no slot holds it. */
	bool RestoreCurrentTool(FName ToolContentId);

	/** The tool in hand changed: grants its ability and broadcasts OnToolChanged. */
	void HandleToolChanged();

	/**
	 * Makes the granted ability match the tool in hand and the ability system the owner reaches now: takes back the old
	 * grant if either changed, and gives the tool's ability. Does nothing while the owner reaches no ability system.
	 */
	void UpdateGrantedTool();

	/**
	 * Takes back the granted ability through the ability system it was granted on, which the owner may no longer reach. A
	 * use under way is told the key is released and finishes its time before the ability goes, so a switch can't cut it short.
	 */
	void ReleaseGrantedTool();

	/** The ability system the owner reaches now: the player state's once the owner is possessed. */
	UAbilitySystemComponent* FindAbilitySystem() const;

	/** The granted ability's spec on its ability system, or nullptr. Never kept: specs move when abilities are removed. */
	FGameplayAbilitySpec* FindGrantedSpec() const;

	/** The owner gained or lost its controller, and with it the player state's ability system. Bound to the owner's event. */
	UFUNCTION()
	void FollowController(APawn* Pawn, AController* OldController, AController* NewController);

	/** The neighbour of the owner's tile one AimStep away, or unset if it is outside the field. */
	TOptional<FIntPoint> FindTargetTile() const;

	/** Finds the target tile again and broadcasts OnTargetTileChanged if it changed. */
	void UpdateTargetTile();

	/** Bound to the owner's root component, so the target follows the owner from tile to tile without ticking. */
	void FollowOwnerTransform(USceneComponent* UpdatedComponent, EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport);

	/** Tells the player what happened, through UVestigeFeedbackSubsystem. */
	void Notify(const FText& Message) const;

	UPROPERTY()
	TObjectPtr<AFarmGrid> Grid;

	TOptional<FIntPoint> TargetTile;

	/** The grid step from the owner's tile to the target: X and Y each -1, 0 or 1, never both 0. */
	FIntPoint AimStep = FIntPoint(1, 0);

	/** The owner's tile when the target was last found, so moving within a tile does no work. */
	FIntPoint OwnerCoordinates = FIntPoint::ZeroValue;

	/** Set while the use key is held. Only for the walk speed: the tool's ability reads its spec's input instead. */
	bool bToolUseHeld = false;

	/** The tool whose ability is granted, the ability system it is granted on, and its spec. Empty when nothing is granted. */
	TWeakObjectPtr<const UVestigeToolData> GrantedTool;
	TWeakObjectPtr<UAbilitySystemComponent> GrantedAbilitySystem;
	FGameplayAbilitySpecHandle GrantedSpec;

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
