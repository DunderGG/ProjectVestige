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

	// The engine cylinder is 100 units tall and centered on its origin. It grows taller as the crop grows, from seedling to ripe.
	constexpr float CropWidthScale = 0.3f;
	constexpr float CropSeedlingHeightScale = 0.15f;
	constexpr float CropRipeHeightScale = 0.9f;
	constexpr float CylinderHalfHeight = 50.f;
}

namespace FarmTileLooks
{
	namespace Detail
	{
		/** The tile's water level as a fraction of the field's cap, from 0 to 1. */
		float GetWaterFraction(const FFarmTileState& Tile, int32 MaxWaterLevel)
		{
			return FMath::Clamp(static_cast<float>(Tile.GetWaterLevel()) / static_cast<float>(FMath::Max(MaxWaterLevel, 1)), 0.f, 1.f);
		}

		/** The tile's contamination level as a fraction of the field's highest level, from 0 to 1. */
		float GetContaminationFraction(const FFarmTileState& Tile, int32 MaxContaminationLevel)
		{
			return FMath::Clamp(
				static_cast<float>(Tile.GetContamination()) / static_cast<float>(FMath::Max(MaxContaminationLevel, 1)), 0.f, 1.f);
		}

		FFarmTilePartLook GetSoilLook(
			const FFarmTileState& Tile, const FFarmTileLookSettings& Settings, float WaterFraction, float ContaminationFraction)
		{
			FFarmTilePartLook Soil;
			Soil.Water = WaterFraction;
			Soil.Contamination = ContaminationFraction;
			if (Tile.GetState() != EFarmTileState::Grass && Settings.TilledSoilMesh)
			{
				Soil.Mesh = Settings.TilledSoilMesh;
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
				Soil.Color = FMath::Lerp(TilledColor, WateredColor, WaterFraction);
			}
			return Soil;
		}

		/** How far a planted tile's crop has grown, from 0 on the day planted to 1 when ripe. */
		float GetGrowthFraction(const FFarmTileState& Tile)
		{
			if (Tile.IsRipe())
			{
				return 1.f;
			}
			const UFarmCropData* CropData = Tile.GetCrop();
			if (!CropData)
			{
				return 0.f;
			}
			const float DaysToMature = static_cast<float>(FMath::Max(CropData->GetDaysToMature(), 1));
			return FMath::Clamp(static_cast<float>(Tile.GetGrowthDays()) / DaysToMature, 0.f, 1.f);
		}

		/** The placeholder cylinder's color for a planted or withered tile's crop. */
		FLinearColor GetPlaceholderCropColor(const FFarmTileState& Tile)
		{
			if (Tile.IsWithered())
			{
				return WitheredColor;
			}
			if (Tile.IsMutated())
			{
				return MutatedColor;
			}
			return Tile.IsRipe() ? RipeColor : GrowingColor;
		}

		FFarmTilePartLook GetCropLook(const FFarmTileState& Tile, const FFarmTileLookSettings& Settings, float WaterFraction)
		{
			using namespace FarmTilePlaceholder;

			FFarmTilePartLook Crop;
			Crop.Water = WaterFraction;
			const bool bWithered = Tile.IsWithered();
			const float WitheredHeight = FMath::Clamp(Settings.WitheredCropHeightScale, 0.1f, 1.f);
			const float SurfaceZ = Settings.TilledSoilMesh ? Settings.SoilSurfaceHeight : SoilTop;
			const bool bMutated = Tile.IsMutated();
			const UFarmCropData* CropData = Tile.GetCrop();
			FFarmCropGrowthLook GrowthLook = CropData ? CropData->GetGrowthLook(Tile.GetGrowthDays()) : FFarmCropGrowthLook();
			if (bMutated && CropData)
			{
				// A mutated crop is always ripe, shown at full size like the ripe mesh it replaces.
				GrowthLook = {CropData->GetMutatedRipeMesh(), 1.f};
			}
			if (GrowthLook.Mesh)
			{
				Crop.Mesh = GrowthLook.Mesh;
				FVector Scale(GrowthLook.Scale);
				if (bWithered)
				{
					Crop.Material = Settings.WitheredCropMaterial;
					Scale.Z *= WitheredHeight;
				}
				else if (bMutated)
				{
					Crop.Material = Settings.MutatedCropMaterial;
				}
				Crop.Transform = FTransform(FQuat::Identity, FVector(0.f, 0.f, SurfaceZ), Scale);
				return Crop;
			}

			// The placeholder ignores the stage scale: its height already follows the whole growth.
			const float HeightScale =
				FMath::Lerp(CropSeedlingHeightScale, CropRipeHeightScale, GetGrowthFraction(Tile)) * (bWithered ? WitheredHeight : 1.f);
			Crop.Mesh = Settings.PlaceholderCropMesh;
			Crop.Material = Settings.PlaceholderMaterial;
			Crop.Transform = FTransform(FQuat::Identity, FVector(0.f, 0.f, SurfaceZ + HeightScale * CylinderHalfHeight),
				FVector(CropWidthScale, CropWidthScale, HeightScale));
			Crop.Color = GetPlaceholderCropColor(Tile);
			return Crop;
		}
	}

	FFarmTileLook GetLook(
		const FFarmTileState& Tile, const FFarmTileLookSettings& Settings, int32 MaxWaterLevel, int32 MaxContaminationLevel)
	{
		const float WaterFraction = Detail::GetWaterFraction(Tile, MaxWaterLevel);
		FFarmTileLook Look;
		Look.Soil = Detail::GetSoilLook(Tile, Settings, WaterFraction, Detail::GetContaminationFraction(Tile, MaxContaminationLevel));
		if (Tile.GetState() == EFarmTileState::Planted || Tile.IsWithered())
		{
			Look.Crop = Detail::GetCropLook(Tile, Settings, WaterFraction);
		}
		return Look;
	}
}
