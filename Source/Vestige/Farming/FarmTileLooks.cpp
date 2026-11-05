// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Farming/FarmTileLooks.h"

#include "Content/FarmCropData.h"
#include "Farming/FarmTileState.h"

namespace FarmTilePlaceholder
{
	// The engine cube is a tile wide, so this makes a thin 95x95 cm slab with a small gap between tiles.
	const FVector SoilScale(0.95f, 0.95f, 0.04f);
	const FVector SoilLocation(0.f, 0.f, 2.f);
	constexpr float SoilTop = 4.f;

	// The engine cylinder is 100 units tall and centered on its origin. It grows taller each stage.
	constexpr float CropWidthScale = 0.3f;
	constexpr float CropSeedlingHeightScale = 0.15f;
	constexpr float CropHeightScalePerDay = 0.25f;
	constexpr float CylinderHalfHeight = 50.f;
}

namespace FarmTileLooks
{
	namespace Detail
	{
		FFarmTilePartLook GetSoilLook(const FFarmTileState& Tile, const FFarmTileLookSettings& Settings)
		{
			FFarmTilePartLook Soil;
			if (Tile.GetState() != EFarmTileState::Grass && Settings.TilledSoilMesh)
			{
				Soil.Mesh = Settings.TilledSoilMesh;
				Soil.Material = Tile.IsWatered() ? Settings.WateredSoilMaterial.Get() : nullptr;
				return Soil;
			}

			Soil.Mesh = Settings.PlaceholderSoilMesh;
			Soil.Material = Settings.PlaceholderMaterial;
			Soil.Transform = FTransform(FQuat::Identity, FarmTilePlaceholder::SoilLocation, FarmTilePlaceholder::SoilScale);
			if (Tile.GetState() == EFarmTileState::Grass)
			{
				Soil.Color = GrassColor;
			}
			else
			{
				Soil.Color = Tile.IsWatered() ? WateredColor : TilledColor;
			}
			return Soil;
		}

		FFarmTilePartLook GetCropLook(const FFarmTileState& Tile, const FFarmTileLookSettings& Settings)
		{
			using namespace FarmTilePlaceholder;

			FFarmTilePartLook Crop;
			const float SurfaceZ = Settings.TilledSoilMesh ? Settings.SoilSurfaceHeight : SoilTop;
			const UFarmCropData* CropData = Tile.GetCrop();
			if (UStaticMesh* StageMesh = CropData ? CropData->GetGrowthStageMesh(Tile.GetGrowthDays()) : nullptr)
			{
				Crop.Mesh = StageMesh;
				Crop.Transform = FTransform(FVector(0.f, 0.f, SurfaceZ));
				return Crop;
			}

			const float HeightScale = CropSeedlingHeightScale + CropHeightScalePerDay * Tile.GetGrowthDays();
			Crop.Mesh = Settings.PlaceholderCropMesh;
			Crop.Material = Settings.PlaceholderMaterial;
			Crop.Transform = FTransform(FQuat::Identity, FVector(0.f, 0.f, SurfaceZ + HeightScale * CylinderHalfHeight),
				FVector(CropWidthScale, CropWidthScale, HeightScale));
			Crop.Color = Tile.IsRipe() ? RipeColor : GrowingColor;
			return Crop;
		}
	}

	FFarmTileLook GetLook(const FFarmTileState& Tile, const FFarmTileLookSettings& Settings)
	{
		FFarmTileLook Look;
		Look.Soil = Detail::GetSoilLook(Tile, Settings);
		if (Tile.GetState() == EFarmTileState::Planted)
		{
			Look.Crop = Detail::GetCropLook(Tile, Settings);
		}
		return Look;
	}
}
