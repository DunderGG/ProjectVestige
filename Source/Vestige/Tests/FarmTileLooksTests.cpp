// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/ActorTestSpawner.h"
#include "Content/FarmCropData.h"
#include "Content/VestigeContentData.h"
#include "CQTest.h"
#include "Engine/StaticMesh.h"
#include "Farming/FarmTileLooks.h"
#include "Farming/FarmTileState.h"
#include "Materials/Material.h"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace FarmTileLooksTests
{
	/** What each tile state looks like, without a world. */
	TEST_CLASS(FarmTileLooks, "Vestige")
	{
		FActorTestSpawner Spawner;
		FFarmTileLookSettings Settings;

		/** Placeholders only, as a level without art would draw. */
		BEFORE_EACH()
		{
			Settings.PlaceholderSoilMesh = &Spawner.SpawnObject<UStaticMesh>();
			Settings.PlaceholderCropMesh = &Spawner.SpawnObject<UStaticMesh>();
			Settings.PlaceholderMaterial = &Spawner.SpawnObject<UMaterial>();
		}

		void AddSoilArt()
		{
			Settings.TilledSoilMesh = &Spawner.SpawnObject<UStaticMesh>();
			Settings.WateredSoilMaterial = &Spawner.SpawnObject<UMaterial>();
		}

		static FFarmTileState TilledTile()
		{
			FFarmTileState Tile;
			Tile.Till();
			return Tile;
		}

		static FFarmTileState PlantedTile(const UFarmCropData& Crop, int32 GrowthDays)
		{
			FFarmTileState Tile;
			Tile.Restore(EFarmTileState::Planted, false, GrowthDays, &Crop);
			return Tile;
		}

		TEST_METHOD(GrassIsAGreenPlaceholderSlabWithoutACrop)
		{
			AddSoilArt();

			const FFarmTileLook Look = ::FarmTileLooks::GetLook(FFarmTileState(), Settings);

			ASSERT_THAT(IsTrue(Look.Soil.Mesh == Settings.PlaceholderSoilMesh));
			ASSERT_THAT(IsTrue(Look.Soil.Material == Settings.PlaceholderMaterial));
			ASSERT_THAT(IsTrue(Look.Soil.Color == ::FarmTileLooks::GrassColor));
			ASSERT_THAT(IsFalse(Look.Crop.IsSet()));
		}

		TEST_METHOD(TilledSoilUsesTheSoilArtAndTheWateredMaterialWhenWet)
		{
			AddSoilArt();
			FFarmTileState Tile = TilledTile();

			const FFarmTileLook Dry = ::FarmTileLooks::GetLook(Tile, Settings);
			Tile.Water();
			const FFarmTileLook Wet = ::FarmTileLooks::GetLook(Tile, Settings);

			ASSERT_THAT(IsTrue(Dry.Soil.Mesh == Settings.TilledSoilMesh));
			ASSERT_THAT(IsNull(Dry.Soil.Material));
			ASSERT_THAT(IsTrue(Wet.Soil.Mesh == Settings.TilledSoilMesh));
			ASSERT_THAT(IsTrue(Wet.Soil.Material == Settings.WateredSoilMaterial));
		}

		TEST_METHOD(WithoutSoilArtThePlaceholderShowsTilledAndWateredColors)
		{
			FFarmTileState Tile = TilledTile();

			const FFarmTileLook Dry = ::FarmTileLooks::GetLook(Tile, Settings);
			Tile.Water();
			const FFarmTileLook Wet = ::FarmTileLooks::GetLook(Tile, Settings);

			ASSERT_THAT(IsTrue(Dry.Soil.Mesh == Settings.PlaceholderSoilMesh));
			ASSERT_THAT(IsTrue(Dry.Soil.Color == ::FarmTileLooks::TilledColor));
			ASSERT_THAT(IsTrue(Wet.Soil.Color == ::FarmTileLooks::WateredColor));
		}

		TEST_METHOD(ACropWithoutStageMeshesIsAPlaceholderThatGrowsTallerAndRipens)
		{
			const UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();

			const FFarmTileLook Seedling = ::FarmTileLooks::GetLook(PlantedTile(Crop, 0), Settings);
			const FFarmTileLook Ripe = ::FarmTileLooks::GetLook(PlantedTile(Crop, Crop.GetDaysToMature()), Settings);

			ASSERT_THAT(IsTrue(Seedling.Crop.IsSet() && Ripe.Crop.IsSet()));
			ASSERT_THAT(IsTrue(Seedling.Crop->Mesh == Settings.PlaceholderCropMesh));
			ASSERT_THAT(IsTrue(Seedling.Crop->Material == Settings.PlaceholderMaterial));
			ASSERT_THAT(IsTrue(Seedling.Crop->Color == ::FarmTileLooks::GrowingColor));
			ASSERT_THAT(IsTrue(Ripe.Crop->Color == ::FarmTileLooks::RipeColor));
			ASSERT_THAT(IsTrue(Ripe.Crop->Transform.GetScale3D().Z > Seedling.Crop->Transform.GetScale3D().Z));
		}

		TEST_METHOD(ACropWithStageMeshesShowsItsStageOnTheSoilSurface)
		{
			AddSoilArt();
			const FPrimaryAssetId TurnipId(UFarmCropData::AssetType, TEXT("Base.Turnip"));
			const UFarmCropData* Crop = Cast<UFarmCropData>(UVestigeContentData::LoadWithVisuals(TurnipId));
			ASSERT_THAT(IsNotNull(Crop));
			ASSERT_THAT(IsNotNull(Crop->GetGrowthStageMesh(0)));

			const FFarmTileLook Look = ::FarmTileLooks::GetLook(PlantedTile(*Crop, 0), Settings);

			ASSERT_THAT(IsTrue(Look.Crop.IsSet()));
			ASSERT_THAT(IsTrue(Look.Crop->Mesh == Crop->GetGrowthStageMesh(0)));
			ASSERT_THAT(IsNull(Look.Crop->Material));
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Look.Crop->Transform.GetLocation().Z, Settings.SoilSurfaceHeight)));
		}
	};
}

#endif
