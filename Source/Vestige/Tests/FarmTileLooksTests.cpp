// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/ActorTestSpawner.h"
#include "Content/FarmCropData.h"
#include "Content/VestigeContentCatalog.h"
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

		void AddSoilArt() { Settings.TilledSoilMesh = &Spawner.SpawnObject<UStaticMesh>(); }

		static constexpr int32 MaxWater = 2;

		static FFarmTileState TilledTile()
		{
			FFarmTileState Tile;
			Tile.Till();
			return Tile;
		}

		static FFarmTileState PlantedTile(const UFarmCropData& Crop, int32 GrowthDays)
		{
			FFarmTileState Tile;
			Tile.Restore(EFarmTileState::Planted, 0, GrowthDays, &Crop, MaxWater);
			return Tile;
		}

		TEST_METHOD(GrassIsAGreenPlaceholderSlabWithoutACrop)
		{
			AddSoilArt();

			const FFarmTileLook Look = ::FarmTileLooks::GetLook(FFarmTileState(), Settings, MaxWater);

			ASSERT_THAT(IsTrue(Look.Soil.Mesh == Settings.PlaceholderSoilMesh));
			ASSERT_THAT(IsTrue(Look.Soil.Material == Settings.PlaceholderMaterial));
			ASSERT_THAT(IsTrue(Look.Soil.Color == ::FarmTileLooks::GrassColor));
			ASSERT_THAT(IsFalse(Look.Crop.IsSet()));
		}

		TEST_METHOD(TilledSoilUsesTheSoilArtAndCarriesTheWaterFraction)
		{
			AddSoilArt();
			FFarmTileState Tile = TilledTile();

			const FFarmTileLook Dry = ::FarmTileLooks::GetLook(Tile, Settings, MaxWater);
			Tile.AddWater(1, MaxWater);
			const FFarmTileLook Wet = ::FarmTileLooks::GetLook(Tile, Settings, MaxWater);

			ASSERT_THAT(IsTrue(Dry.Soil.Mesh == Settings.TilledSoilMesh));
			ASSERT_THAT(IsNull(Dry.Soil.Material));
			ASSERT_THAT(IsTrue(Wet.Soil.Mesh == Settings.TilledSoilMesh));
			ASSERT_THAT(IsTrue(Dry.Soil.Water == 0.f));
			ASSERT_THAT(IsTrue(Wet.Soil.Water == 0.5f));
		}

		TEST_METHOD(SoilAndCropCarryTheWaterAsAFractionOfTheCap)
		{
			AddSoilArt();
			UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();
			const float Expected[] = {0.f, 0.5f, 1.f};

			for (int32 Level = 0; Level <= MaxWater; ++Level)
			{
				FFarmTileState Tile;
				Tile.Restore(EFarmTileState::Planted, Level, 0, &Crop, MaxWater);
				const FFarmTileLook Look = ::FarmTileLooks::GetLook(Tile, Settings, MaxWater);

				ASSERT_THAT(IsTrue(Look.Soil.Water == Expected[Level]));
				ASSERT_THAT(IsTrue(Look.Crop.IsSet() && Look.Crop->Water == Expected[Level]));
			}
		}

		TEST_METHOD(TheFractionFollowsTheFieldsCap)
		{
			FFarmTileState Tile;
			Tile.Restore(EFarmTileState::Tilled, 1, 0, nullptr, 4);

			const FFarmTileLook Look = ::FarmTileLooks::GetLook(Tile, Settings, 4);

			ASSERT_THAT(IsTrue(Look.Soil.Water == 0.25f));
		}

		TEST_METHOD(ACapBelowOneIsReadAsOneAndGrassIsDry)
		{
			FFarmTileState Wet;
			Wet.Restore(EFarmTileState::Tilled, 1, 0, nullptr, 1);

			const FFarmTileLook Soaked = ::FarmTileLooks::GetLook(Wet, Settings, 0);
			const FFarmTileLook Grass = ::FarmTileLooks::GetLook(FFarmTileState(), Settings, MaxWater);

			ASSERT_THAT(IsTrue(Soaked.Soil.Water == 1.f));
			ASSERT_THAT(IsTrue(Grass.Soil.Water == 0.f));
		}

		TEST_METHOD(TheSoilStaysInOneSetWhateverItsWater)
		{
			AddSoilArt();
			FFarmTileState Tile = TilledTile();
			const FFarmTileLook First = ::FarmTileLooks::GetLook(Tile, Settings, MaxWater);

			for (int32 Level = 1; Level <= MaxWater; ++Level)
			{
				Tile.AddWater(1, MaxWater);
				const FFarmTileLook Look = ::FarmTileLooks::GetLook(Tile, Settings, MaxWater);

				ASSERT_THAT(IsTrue(Look.Soil.Mesh == First.Soil.Mesh));
				ASSERT_THAT(IsTrue(Look.Soil.Material == First.Soil.Material));
			}
		}

		TEST_METHOD(ThePlaceholderSoilGetsDarkerWithEachWaterStep)
		{
			FFarmTileState Tile = TilledTile();
			float PreviousBrightness = TNumericLimits<float>::Max();

			for (int32 Level = 0; Level <= MaxWater; ++Level)
			{
				const FLinearColor Color = ::FarmTileLooks::GetLook(Tile, Settings, MaxWater).Soil.Color;
				const float Brightness = Color.R + Color.G + Color.B;

				ASSERT_THAT(IsTrue(Brightness < PreviousBrightness));
				PreviousBrightness = Brightness;
				Tile.AddWater(1, MaxWater);
			}
		}

		TEST_METHOD(WithoutSoilArtThePlaceholderShowsTilledAndWateredColors)
		{
			FFarmTileState Tile = TilledTile();

			const FFarmTileLook Dry = ::FarmTileLooks::GetLook(Tile, Settings, MaxWater);
			Tile.AddWater(MaxWater, MaxWater);
			const FFarmTileLook Wet = ::FarmTileLooks::GetLook(Tile, Settings, MaxWater);

			ASSERT_THAT(IsTrue(Dry.Soil.Mesh == Settings.PlaceholderSoilMesh));
			ASSERT_THAT(IsTrue(Dry.Soil.Color.Equals(::FarmTileLooks::TilledColor)));
			ASSERT_THAT(IsTrue(Wet.Soil.Color.Equals(::FarmTileLooks::WateredColor)));
		}

		TEST_METHOD(ACropWithoutStageMeshesIsAPlaceholderThatGrowsTallerAndRipens)
		{
			const UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();

			const FFarmTileLook Seedling = ::FarmTileLooks::GetLook(PlantedTile(Crop, 0), Settings, MaxWater);
			const FFarmTileLook Ripe = ::FarmTileLooks::GetLook(PlantedTile(Crop, Crop.GetDaysToMature()), Settings, MaxWater);

			ASSERT_THAT(IsTrue(Seedling.Crop.IsSet() && Ripe.Crop.IsSet()));
			ASSERT_THAT(IsTrue(Seedling.Crop->Mesh == Settings.PlaceholderCropMesh));
			ASSERT_THAT(IsTrue(Seedling.Crop->Material == Settings.PlaceholderMaterial));
			ASSERT_THAT(IsTrue(Seedling.Crop->Color == ::FarmTileLooks::GrowingColor));
			ASSERT_THAT(IsTrue(Ripe.Crop->Color == ::FarmTileLooks::RipeColor));
			ASSERT_THAT(IsTrue(Ripe.Crop->Transform.GetScale3D().Z > Seedling.Crop->Transform.GetScale3D().Z));
		}

		/** A crop that takes Days to mature, with two stage meshes and a ripe mesh. */
		UFarmCropData& MeshedCrop(int32 Days, UStaticMesh*& OutRipe)
		{
			UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();
			UStaticMesh& Stage0 = Spawner.SpawnObject<UStaticMesh>();
			UStaticMesh& Stage1 = Spawner.SpawnObject<UStaticMesh>();
			OutRipe = &Spawner.SpawnObject<UStaticMesh>();
			Crop.SetDaysToMatureForTests(Days);
			TArray<FFarmCropGrowthStage> GrowthStages;
			GrowthStages.AddDefaulted(2);
			GrowthStages[0].Mesh = &Stage0;
			GrowthStages[1].StartDay = 1;
			GrowthStages[1].Mesh = &Stage1;
			Crop.SetGrowthStagesForTests(GrowthStages, TSoftObjectPtr<UStaticMesh>(OutRipe));
			return Crop;
		}

		TEST_METHOD(ACropWithStageMeshesShowsItsStageOnTheSoilSurface)
		{
			AddSoilArt();
			const FPrimaryAssetId TurnipId(UFarmCropData::AssetType, TEXT("Base.Turnip"));
			Spawner.InitializeGameSubsystems();
			const UFarmCropData* Crop = UVestigeContentCatalog::Get(&Spawner.GetWorld())->Find<UFarmCropData>(TurnipId);
			ASSERT_THAT(IsNotNull(Crop));
			ASSERT_THAT(IsNotNull(Crop->GetGrowthLook(0).Mesh));

			const FFarmTileLook Look = ::FarmTileLooks::GetLook(PlantedTile(*Crop, 0), Settings, MaxWater);

			ASSERT_THAT(IsTrue(Look.Crop.IsSet()));
			ASSERT_THAT(IsTrue(Look.Crop->Mesh == Crop->GetGrowthLook(0).Mesh));
			ASSERT_THAT(IsNull(Look.Crop->Material));
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Look.Crop->Transform.GetLocation().Z, Settings.SoilSurfaceHeight)));
		}

		TEST_METHOD(AnUnripeCropNeverShowsItsRipeMesh)
		{
			UStaticMesh* RipeMesh = nullptr;
			const UFarmCropData& Crop = MeshedCrop(4, RipeMesh);

			for (int32 Days = 0; Days < 4; ++Days)
			{
				const FFarmTileLook Growing = ::FarmTileLooks::GetLook(PlantedTile(Crop, Days), Settings, MaxWater);
				ASSERT_THAT(IsTrue(Growing.Crop.IsSet() && Growing.Crop->Mesh != RipeMesh));
			}
			const FFarmTileLook Ripe = ::FarmTileLooks::GetLook(PlantedTile(Crop, 4), Settings, MaxWater);
			ASSERT_THAT(IsTrue(Ripe.Crop.IsSet() && Ripe.Crop->Mesh == RipeMesh));
		}

		TEST_METHOD(APlaceholderCropIsAsTallWhenRipeWhateverItsLength)
		{
			float RipeHeight = -1.f;
			for (const int32 Days : {1, 3, 10})
			{
				UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();
				Crop.SetDaysToMatureForTests(Days);

				const FFarmTileLook Ripe = ::FarmTileLooks::GetLook(PlantedTile(Crop, Days), Settings, MaxWater);

				ASSERT_THAT(IsTrue(Ripe.Crop.IsSet()));
				if (RipeHeight < 0.f)
				{
					RipeHeight = Ripe.Crop->Transform.GetScale3D().Z;
				}
				ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Ripe.Crop->Transform.GetScale3D().Z, RipeHeight)));
			}
		}

		TEST_METHOD(AGrowingCropIsScaledFromItsBase)
		{
			AddSoilArt();
			UStaticMesh* RipeMesh = nullptr;
			const UFarmCropData& Crop = MeshedCrop(10, RipeMesh);
			const FFarmCropGrowthLook GrowthLook = Crop.GetGrowthLook(1);
			ASSERT_THAT(IsTrue(GrowthLook.Scale < 1.f));

			const FFarmTileLook Look = ::FarmTileLooks::GetLook(PlantedTile(Crop, 1), Settings, MaxWater);

			ASSERT_THAT(IsTrue(Look.Crop.IsSet()));
			ASSERT_THAT(IsTrue(Look.Crop->Transform.GetScale3D().Equals(FVector(GrowthLook.Scale))));
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Look.Crop->Transform.GetLocation().Z, Settings.SoilSurfaceHeight)));
		}
	};
}

#endif
