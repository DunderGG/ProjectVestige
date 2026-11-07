// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FarmTargetingComponent.generated.h"

class AFarmGrid;
class USceneComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnFarmTargetTileChanged, const TOptional<FIntPoint>& /* Tile */);

/**
 * The tile its owner, a pawn, aims at: the neighbour of the owner's tile in the aimed direction, snapped to the eight grid
 * directions. Finds the level's farm grid at BeginPlay and follows the owner from tile to tile through its root component's
 * transform updates, never ticking. Tool abilities and the target highlight read the target and listen for changes.
 * Systems.md, section 8.
 */
UCLASS(ClassGroup = (Farming), meta = (BlueprintSpawnableComponent))
class UFarmTargetingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFarmTargetingComponent();

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

	/** Broadcast when the target tile changes, unset when there stops being one. */
	FOnFarmTargetTileChanged OnTargetTileChanged;

protected:
	/** Finds the grid, aims where the owner faces and starts following the owner. */
	virtual void BeginPlay() override;

	/** Stops following the grid and the owner. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
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
};
