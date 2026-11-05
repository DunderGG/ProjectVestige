// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Farming/FarmTileLooks.h"
#include "InstanceDataTypes.h"
#include "UObject/ObjectKey.h"
#include "FarmFieldMeshesComponent.generated.h"

class UFarmField;
class UInstancedStaticMeshComponent;

/**
 * Draws a field's tiles with instanced meshes: one instanced static mesh component per instance set (one mesh with one
 * material override), one soil instance and at most one crop instance per tile, and the color in per-instance custom data
 * instead of a dynamic material. No actor and no object per tile. Presentation only: it follows the field's events and never
 * ticks. AFarmGrid has one and tells it which field to show; instances are in the grid's own space. Systems.md, section 22.
 */
UCLASS(ClassGroup = (Farming))
class UFarmFieldMeshesComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UFarmFieldMeshesComponent();

	/** Draws InField from now on, each tile InTileSize wide from this component's origin, and follows its events. */
	void ShowField(UFarmField& InField, float InTileSize);

	/** Stops following the field and removes every instance. */
	void StopShowing();

	/** How many instances draw Mesh with Material (nullptr for the mesh's own materials). */
	int32 GetInstanceCount(const UStaticMesh* Mesh, const UMaterialInterface* Material = nullptr) const;

	/** How many instances draw the field, all sets together. */
	int32 GetTotalInstanceCount() const;

	/** The custom data of the soil instance of the tile at Coordinates, or an empty array if it has none. */
	TArray<float> GetSoilCustomData(const FIntPoint& Coordinates) const;

	const FFarmTileLookSettings& GetLookSettings() const { return Looks; }

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Where one part of a tile is drawn: its set's index in Sets and its instance there, named by ID because indices move. */
	struct FTileInstance
	{
		int32 SetIndex = INDEX_NONE;
		FPrimitiveInstanceId Id;
	};

	/** Instances waiting to be added to one set while the whole field is redrawn. */
	struct FPendingSet
	{
		TArray<FTransform> Transforms;
		TArray<float> CustomData;
		TArray<FTileInstance*> Instances;
	};

	/** Draws the tile at Coordinates again. Bound to the field's OnTileChanged. */
	void RedrawTile(const FIntPoint& Coordinates);

	/** Draws every tile again, each set filled in one batch. Bound to the field's OnTilesChanged. */
	void RedrawField();

	/** Moves Instance to show Look in the tile at TileTransform, or removes it when Look is null or has no mesh. */
	void ShowPart(FTileInstance& Instance, const FFarmTilePartLook* Look, const FTransform& TileTransform);

	/** Removes Instance from its set, if it is in one. */
	void HidePart(FTileInstance& Instance);

	/** Queues Look for Instance in its set's batch during RedrawField. */
	void QueuePart(TArray<FPendingSet>& Pending, FTileInstance& Instance, const FFarmTilePartLook& Look, const FTransform& TileTransform);

	/** The index of the set that draws Mesh with Material, creating its component the first time. */
	int32 FindOrAddSet(UStaticMesh& Mesh, UMaterialInterface* Material);

	/** A tile's transform in this component's space: centered on the tile, scaled from AFarmGrid::UnscaledTileSize. */
	FTransform GetTileTransform(const FIntPoint& Coordinates) const;

	/** The art the field is drawn with. Set in BP_FarmGrid; empty entries fall back to the placeholders. */
	UPROPERTY(EditAnywhere, Category = "Farming|Visuals", meta = (AllowPrivateAccess = "true", ShowOnlyInnerProperties))
	FFarmTileLookSettings Looks;

	/** One instanced mesh component per instance set, created when a tile first needs it and kept until the grid ends. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> Sets;

	/** Which set draws a mesh with a material override, by index in Sets. */
	TMap<TPair<TObjectKey<UStaticMesh>, TObjectKey<UMaterialInterface>>, int32> SetIndices;

	/** Each tile's soil and crop instance, in the field's tile order (row by row). */
	TArray<FTileInstance> SoilInstances;
	TArray<FTileInstance> CropInstances;

	/** The field shown. It outlives the level, so the component only points at it and stops listening in EndPlay. */
	TWeakObjectPtr<UFarmField> Field;

	/** Columns of the field when it was last drawn in full, for turning coordinates into an index. */
	int32 FieldWidth = 0;

	float TileSize = 100.f;
};
