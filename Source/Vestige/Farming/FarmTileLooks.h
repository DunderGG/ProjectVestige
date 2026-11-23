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

	/**
	 * Mesh for tilled and planted soil, dry or wet. Origin at the bottom center, AFarmGrid::UnscaledTileSize wide. Its material
	 * reads the tile's water from per-instance custom data 3 (MI_FarmSoil_Tilled).
	 */
	UPROPERTY(EditAnywhere, Category = "Farming")
	TObjectPtr<UStaticMesh> TilledSoilMesh;

	/** Height of TilledSoilMesh's surface, where crop meshes stand, in cm. */
	UPROPERTY(EditAnywhere, Category = "Farming", meta = (ClampMin = 0))
	float SoilSurfaceHeight = 6.f;

	/**
	 * Put on every slot of a withered crop's mesh (MI_Crop_Withered, an instance of M_Crop with Used with Instanced Static Meshes
	 * saved). Empty keeps the mesh's own materials.
	 */
	UPROPERTY(EditAnywhere, Category = "Farming")
	TObjectPtr<UMaterialInterface> WitheredCropMaterial;

	/**
	 * Put on every slot of a mutated ripe crop's mesh (MI_Crop_Mutated, an instance of M_Crop with Used with Instanced Static
	 * Meshes saved). Empty keeps the mesh's own materials.
	 */
	UPROPERTY(EditAnywhere, Category = "Farming")
	TObjectPtr<UMaterialInterface> MutatedCropMaterial;

	/** How much of its height a withered crop keeps: a slumped dead plant. Also applies to the placeholder. */
	UPROPERTY(EditAnywhere, Category = "Farming", meta = (ClampMin = 0.1, ClampMax = 1))
	float WitheredCropHeightScale = 0.75f;

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

	/** How wet the tile is, from 0 (dry) to 1 (soaked): per-instance custom data 3, read by the soil's material. */
	float Water = 0.f;

	/**
	 * How contaminated the tile is, from 0 (clean) to 1 (the highest level): per-instance custom data 4, read by the soil's and
	 * the placeholder's material. Set on the soil only; 0 on the crop, which shares the placeholder material.
	 */
	float Contamination = 0.f;
};

/** How a whole tile is drawn. */
struct FFarmTileLook
{
	FFarmTilePartLook Soil;

	/** Set while a crop grows or stands withered on the tile. */
	TOptional<FFarmTilePartLook> Crop;
};

/** What a tile looks like, from its state alone. Presentation rules, kept apart from drawing so they are tested without a world. */
namespace FarmTileLooks
{
	/**
	 * How many floats of per-instance custom data each tile instance has: the color's red, green and blue, then the water, then
	 * the contamination.
	 */
	constexpr int32 NumCustomDataFloats = 5;

	/** The index of the water fraction in the custom data. */
	constexpr int32 WaterCustomDataIndex = 3;

	/** The index of the contamination fraction in the custom data. */
	constexpr int32 ContaminationCustomDataIndex = 4;

	// Placeholder colors until there is art for everything.
	inline const FLinearColor GrassColor(0.15f, 0.45f, 0.10f);
	inline const FLinearColor TilledColor(0.35f, 0.20f, 0.08f);
	inline const FLinearColor WateredColor(0.12f, 0.07f, 0.03f);
	inline const FLinearColor GrowingColor(0.20f, 0.70f, 0.15f);
	inline const FLinearColor RipeColor(1.00f, 0.60f, 0.05f);
	inline const FLinearColor WitheredColor(0.25f, 0.18f, 0.08f);

	/** The contamination cue's magenta, for a mutated ripe crop's placeholder. */
	inline const FLinearColor MutatedColor(0.80f, 0.05f, 0.60f);

	/**
	 * How Tile is drawn with Settings: the tilled soil mesh once the tile isn't grass, and the mesh and scale its crop's data gives
	 * for its growth days (UFarmCropData::GetGrowthLook), standing on the soil surface; a withered crop with the withered material
	 * and a lower scale; a mutated ripe crop with its mutated mesh and the mutated material. The water of the soil and the crop is
	 * the tile's level as a fraction of MaxWaterLevel, the field's cap; the soil's contamination is the tile's level as a fraction
	 * of MaxContaminationLevel.
	 */
	FFarmTileLook GetLook(
		const FFarmTileState& Tile, const FFarmTileLookSettings& Settings, int32 MaxWaterLevel, int32 MaxContaminationLevel);
}
