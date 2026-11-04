// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Farming/FarmTileState.h"
#include "GameFramework/Actor.h"
#include "FarmTile.generated.h"

class UFarmCropData;
class UFarmField;
class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;

/**
 * One square of farmland in the world, showing one tile of a UFarmField: it can be tilled, planted, watered and harvested.
 * The tile's state and rules are an FFarmTileState in the field, which outlives the level; the actor asks the field to
 * change it and shows the result with its meshes. AFarmGrid spawns one per tile and tells it to show itself again when the
 * field changes.
 */
UCLASS()
class AFarmTile : public AActor
{
	GENERATED_BODY()

public:
	/** Width of a tile at scale 1, in cm. The grid scales tiles from this to its own tile size. */
	static constexpr float UnscaledSize = 100.f;

	AFarmTile();

	/** Shows the tile at InCoordinates in InField from now on. Called by the grid that spawns it. */
	void ShowFieldTile(UFarmField& InField, const FIntPoint& InCoordinates);

	/** Shows the field's tile again, after it changed. */
	void Refresh();

	// The farming actions ask the field, following FFarmTileState's rules, documented there. They return false when they
	// change nothing, and always without a field. The field's change event makes the grid refresh this tile.
	bool Till();
	bool Plant(const UFarmCropData* InCrop);
	bool Water();
	bool Harvest();

	// The field's tile; a tile that shows no field reads as grass.
	EFarmTileState GetState() const { return GetTileState().GetState(); }
	bool IsWatered() const { return GetTileState().IsWatered(); }
	bool IsRipe() const { return GetTileState().IsRipe(); }
	bool CanTill() const { return GetTileState().CanTill(); }
	bool CanPlant() const { return GetTileState().CanPlant(); }
	bool CanWater() const { return GetTileState().CanWater(); }
	int32 GetGrowthDays() const { return GetTileState().GetGrowthDays(); }

	/** The crop growing here, or nullptr if nothing is planted. */
	const UFarmCropData* GetCrop() const { return GetTileState().GetCrop(); }

	/** The tile's column (X) and row (Y) in its field. */
	FIntPoint GetCoordinates() const { return Coordinates; }

protected:
	virtual void BeginPlay() override;

private:
	/** The field's tile, or a grass tile if the actor shows no field or its coordinates are outside it. */
	const FFarmTileState& GetTileState() const;

	void RefreshSoil();
	void RefreshCrop();

	/** The placeholder soil's color for the current state: grass, dry soil or watered soil. */
	FLinearColor GetPlaceholderSoilColor() const;

	// Optional art. Anything left empty falls back to the colored placeholder shapes.

	/** Mesh for tilled, planted and watered soil. Origin at the bottom center, UnscaledSize wide. */
	UPROPERTY(EditDefaultsOnly, Category = "Farming|Visuals", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMesh> TilledSoilMesh;

	/** Material swapped onto TilledSoilMesh while the soil is watered. */
	UPROPERTY(EditDefaultsOnly, Category = "Farming|Visuals", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialInterface> WateredSoilMaterial;

	/** Height of the soil surface that custom crop meshes stand on, in cm. */
	UPROPERTY(EditDefaultsOnly, Category = "Farming|Visuals", meta = (AllowPrivateAccess = "true", ClampMin = 0))
	float SoilSurfaceHeight = 6.f;

	UPROPERTY(VisibleAnywhere, Category = "Farming")
	TObjectPtr<UStaticMeshComponent> SoilMesh;

	UPROPERTY(VisibleAnywhere, Category = "Farming")
	TObjectPtr<UStaticMeshComponent> CropMesh;

	UPROPERTY()
	TObjectPtr<UStaticMesh> PlaceholderSoilMesh;

	UPROPERTY()
	TObjectPtr<UStaticMesh> PlaceholderCropMesh;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> SoilMaterial;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> CropMaterial;

	/** The field this tile shows. The field outlives the level, so the tile only points at it. */
	TWeakObjectPtr<UFarmField> Field;

	UPROPERTY(VisibleInstanceOnly, Category = "Farming")
	FIntPoint Coordinates = FIntPoint::ZeroValue;
};
