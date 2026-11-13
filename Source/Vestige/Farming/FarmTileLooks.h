// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "FarmTileLooks.generated.h"

class UMaterialInterface;
class UStaticMesh;
struct FFarmTileState;

/**
 * The art a field is drawn with, set on the grid's UFarmFieldMeshesComponent in BP_FarmGrid. Anything left empty falls back
 * to the colored placeholder shapes. Crop meshes aren't here: each crop's data has its own. BlueprintType only so editor
 * scripts can make and set it; Blueprints don't read it.
 */
USTRUCT(BlueprintType)
struct FFarmTileLookSettings
{
	GENERATED_BODY()

	/** Mesh for tilled, planted and watered soil. Origin at the bottom center, AFarmGrid::UnscaledTileSize wide. */
	UPROPERTY(EditAnywhere, Category = "Farming")
	TObjectPtr<UStaticMesh> TilledSoilMesh;

	/** Material put on every slot of TilledSoilMesh while the soil is watered. */
	UPROPERTY(EditAnywhere, Category = "Farming")
	TObjectPtr<UMaterialInterface> WateredSoilMaterial;

	/** Height of TilledSoilMesh's surface, where crop meshes stand, in cm. */
	UPROPERTY(EditAnywhere, Category = "Farming", meta = (ClampMin = 0))
	float SoilSurfaceHeight = 6.f;

	/** Placeholder for soil without art, and for grass: the engine cube, flattened into a slab. */
	UPROPERTY(EditAnywhere, Category = "Farming|Placeholders")
	TObjectPtr<UStaticMesh> PlaceholderSoilMesh;

	/** Placeholder for a crop without a mesh for its growth stage: the engine cylinder, taller as it grows. */
	UPROPERTY(EditAnywhere, Category = "Farming|Placeholders")
	TObjectPtr<UStaticMesh> PlaceholderCropMesh;

	/**
	 * Material of the placeholder shapes. It reads the color from per-instance custom data 0 to 2 and needs Used with
	 * Instanced Static Meshes set and saved (M_FarmPlaceholder).
	 */
	UPROPERTY(EditAnywhere, Category = "Farming|Placeholders")
	TObjectPtr<UMaterialInterface> PlaceholderMaterial;
};

/** How one part of a tile (its soil or its crop) is drawn: one instance of Mesh. */
struct FFarmTilePartLook
{
	/** The mesh, or nullptr to draw nothing. */
	UStaticMesh* Mesh = nullptr;

	/** Put on every slot of Mesh, or nullptr for the mesh's own materials. */
	UMaterialInterface* Material = nullptr;

	/** Where the instance sits in the tile's own space: origin at the tile's center, AFarmGrid::UnscaledTileSize wide. */
	FTransform Transform;

	/** Per-instance custom data 0 to 2, read by the placeholder material. */
	FLinearColor Color = FLinearColor::White;
};

/** How a whole tile is drawn. */
struct FFarmTileLook
{
	FFarmTilePartLook Soil;

	/** Set while something grows on the tile. */
	TOptional<FFarmTilePartLook> Crop;
};

/** What a tile looks like, from its state alone. Presentation rules, kept apart from drawing so they are tested without a world. */
namespace FarmTileLooks
{
	/** How many floats of per-instance custom data each tile instance has: the color's red, green and blue. */
	constexpr int32 NumCustomDataFloats = 3;

	// Placeholder colors until there is art for everything.
	inline const FLinearColor GrassColor(0.15f, 0.45f, 0.10f);
	inline const FLinearColor TilledColor(0.35f, 0.20f, 0.08f);
	inline const FLinearColor WateredColor(0.12f, 0.07f, 0.03f);
	inline const FLinearColor GrowingColor(0.20f, 0.70f, 0.15f);
	inline const FLinearColor RipeColor(1.00f, 0.60f, 0.05f);

	/**
	 * How Tile is drawn with Settings: the tilled soil mesh once the tile isn't grass, and the mesh and scale its crop's data gives
	 * for its growth days (UFarmCropData::GetGrowthLook), standing on the soil surface.
	 */
	FFarmTileLook GetLook(const FFarmTileState& Tile, const FFarmTileLookSettings& Settings);
}
