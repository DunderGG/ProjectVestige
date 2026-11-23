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
	/** The highest contamination level the looks below are drawn for. */
	constexpr int32 MaxContamination = 2;

	/** A saved tile with only the values from before contamination: clean, no watered nights, not mutated. */
	FFarmTileSaveData SavedTile(EFarmTileState State, int32 WaterLevel, int32 GrowthDays)
	{
		FFarmTileSaveData Saved;
		Saved.State = State;
		Saved.WaterLevel = WaterLevel;
		Saved.GrowthDays = GrowthDays;
		return Saved;
	}

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
			Tile.Restore(SavedTile(EFarmTileState::Planted, 0, GrowthDays), &Crop, MaxWater, MaxContamination);
			return Tile;
		}

		TEST_METHOD(GrassIsAGreenPlaceholderSlabWithoutACrop)
		{
			AddSoilArt();

			const FFarmTileLook Look = ::FarmTileLooks::GetLook(FFarmTileState(), Settings, MaxWater, MaxContamination);

			ASSERT_THAT(IsTrue(Look.Soil.Mesh == Settings.PlaceholderSoilMesh));
			ASSERT_THAT(IsTrue(Look.Soil.Material == Settings.PlaceholderMaterial));
			ASSERT_THAT(IsTrue(Look.Soil.Color == ::FarmTileLooks::GrassColor));
			ASSERT_THAT(IsFalse(Look.Crop.IsSet()));
		}

		TEST_METHOD(TilledSoilUsesTheSoilArtAndCarriesTheWaterFraction)
		{
			AddSoilArt();
			FFarmTileState Tile = TilledTile();

			const FFarmTileLook Dry = ::FarmTileLooks::GetLook(Tile, Settings, MaxWater, MaxContamination);
			Tile.AddWater(1, MaxWater);
			const FFarmTileLook Wet = ::FarmTileLooks::GetLook(Tile, Settings, MaxWater, MaxContamination);

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
				Tile.Restore(SavedTile(EFarmTileState::Planted, Level, 0), &Crop, MaxWater, MaxContamination);
				const FFarmTileLook Look = ::FarmTileLooks::GetLook(Tile, Settings, MaxWater, MaxContamination);

				ASSERT_THAT(IsTrue(Look.Soil.Water == Expected[Level]));
				ASSERT_THAT(IsTrue(Look.Crop.IsSet() && Look.Crop->Water == Expected[Level]));
			}
		}

		TEST_METHOD(TheFractionFollowsTheFieldsCap)
		{
			FFarmTileState Tile;
			Tile.Restore(SavedTile(EFarmTileState::Tilled, 1, 0), nullptr, 4, MaxContamination);

			const FFarmTileLook Look = ::FarmTileLooks::GetLook(Tile, Settings, 4, MaxContamination);

			ASSERT_THAT(IsTrue(Look.Soil.Water == 0.25f));
		}

		TEST_METHOD(ACapBelowOneIsReadAsOneAndGrassIsDry)
		{
			FFarmTileState Wet;
			Wet.Restore(SavedTile(EFarmTileState::Tilled, 1, 0), nullptr, 1, MaxContamination);

			const FFarmTileLook Soaked = ::FarmTileLooks::GetLook(Wet, Settings, 0, MaxContamination);
			const FFarmTileLook Grass = ::FarmTileLooks::GetLook(FFarmTileState(), Settings, MaxWater, MaxContamination);

			ASSERT_THAT(IsTrue(Soaked.Soil.Water == 1.f));
			ASSERT_THAT(IsTrue(Grass.Soil.Water == 0.f));
		}

		TEST_METHOD(TheSoilStaysInOneSetWhateverItsWater)
		{
			AddSoilArt();
			FFarmTileState Tile = TilledTile();
			const FFarmTileLook First = ::FarmTileLooks::GetLook(Tile, Settings, MaxWater, MaxContamination);

			for (int32 Level = 1; Level <= MaxWater; ++Level)
			{
				Tile.AddWater(1, MaxWater);
				const FFarmTileLook Look = ::FarmTileLooks::GetLook(Tile, Settings, MaxWater, MaxContamination);

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
				const FLinearColor Color = ::FarmTileLooks::GetLook(Tile, Settings, MaxWater, MaxContamination).Soil.Color;
				const float Brightness = Color.R + Color.G + Color.B;

				ASSERT_THAT(IsTrue(Brightness < PreviousBrightness));
				PreviousBrightness = Brightness;
				Tile.AddWater(1, MaxWater);
			}
		}

		TEST_METHOD(WithoutSoilArtThePlaceholderShowsTilledAndWateredColors)
		{
			FFarmTileState Tile = TilledTile();

			const FFarmTileLook Dry = ::FarmTileLooks::GetLook(Tile, Settings, MaxWater, MaxContamination);
			Tile.AddWater(MaxWater, MaxWater);
			const FFarmTileLook Wet = ::FarmTileLooks::GetLook(Tile, Settings, MaxWater, MaxContamination);

			ASSERT_THAT(IsTrue(Dry.Soil.Mesh == Settings.PlaceholderSoilMesh));
			ASSERT_THAT(IsTrue(Dry.Soil.Color.Equals(::FarmTileLooks::TilledColor)));
			ASSERT_THAT(IsTrue(Wet.Soil.Color.Equals(::FarmTileLooks::WateredColor)));
		}

		TEST_METHOD(ACropWithoutStageMeshesIsAPlaceholderThatGrowsTallerAndRipens)
		{
			const UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();

			const FFarmTileLook Seedling = ::FarmTileLooks::GetLook(PlantedTile(Crop, 0), Settings, MaxWater, MaxContamination);
			const FFarmTileLook Ripe =
				::FarmTileLooks::GetLook(PlantedTile(Crop, Crop.GetDaysToMature()), Settings, MaxWater, MaxContamination);

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

			const FFarmTileLook Look = ::FarmTileLooks::GetLook(PlantedTile(*Crop, 0), Settings, MaxWater, MaxContamination);

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
				const FFarmTileLook Growing = ::FarmTileLooks::GetLook(PlantedTile(Crop, Days), Settings, MaxWater, MaxContamination);
				ASSERT_THAT(IsTrue(Growing.Crop.IsSet() && Growing.Crop->Mesh != RipeMesh));
			}
			const FFarmTileLook Ripe = ::FarmTileLooks::GetLook(PlantedTile(Crop, 4), Settings, MaxWater, MaxContamination);
			ASSERT_THAT(IsTrue(Ripe.Crop.IsSet() && Ripe.Crop->Mesh == RipeMesh));
		}

		TEST_METHOD(APlaceholderCropIsAsTallWhenRipeWhateverItsLength)
		{
			float RipeHeight = -1.f;
			for (const int32 Days : {1, 3, 10})
			{
				UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();
				Crop.SetDaysToMatureForTests(Days);

				const FFarmTileLook Ripe = ::FarmTileLooks::GetLook(PlantedTile(Crop, Days), Settings, MaxWater, MaxContamination);

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

			const FFarmTileLook Look = ::FarmTileLooks::GetLook(PlantedTile(Crop, 1), Settings, MaxWater, MaxContamination);

			ASSERT_THAT(IsTrue(Look.Crop.IsSet()));
			ASSERT_THAT(IsTrue(Look.Crop->Transform.GetScale3D().Equals(FVector(GrowthLook.Scale))));
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Look.Crop->Transform.GetLocation().Z, Settings.SoilSurfaceHeight)));
		}

		static FFarmTileState WitheredTile(const UFarmCropData& Crop, int32 GrowthDays, int32 Water)
		{
			FFarmTileState Tile;
			Tile.Restore(SavedTile(EFarmTileState::Withered, Water, GrowthDays), &Crop, MaxWater, MaxContamination);
			return Tile;
		}

		TEST_METHOD(AWitheredCropShowsItsStagesMeshWithTheWitheredMaterialLowerOnTheWetSoil)
		{
			AddSoilArt();
			Settings.WitheredCropMaterial = &Spawner.SpawnObject<UMaterial>();
			Settings.WitheredCropHeightScale = 0.5f;
			UStaticMesh* RipeMesh = nullptr;
			const UFarmCropData& Crop = MeshedCrop(10, RipeMesh);
			const FFarmCropGrowthLook GrowthLook = Crop.GetGrowthLook(1);

			const FFarmTileLook Look = ::FarmTileLooks::GetLook(WitheredTile(Crop, 1, 1), Settings, MaxWater, MaxContamination);

			ASSERT_THAT(IsTrue(Look.Crop.IsSet()));
			ASSERT_THAT(IsTrue(Look.Crop->Mesh == GrowthLook.Mesh));
			ASSERT_THAT(IsTrue(Look.Crop->Material == Settings.WitheredCropMaterial));
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Look.Crop->Transform.GetScale3D().X, GrowthLook.Scale)));
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Look.Crop->Transform.GetScale3D().Z, GrowthLook.Scale * 0.5f)));
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Look.Crop->Transform.GetLocation().Z, Settings.SoilSurfaceHeight)));
			ASSERT_THAT(IsTrue(Look.Soil.Mesh == Settings.TilledSoilMesh));
			ASSERT_THAT(IsTrue(Look.Soil.Water == 0.5f));
			ASSERT_THAT(IsTrue(Look.Crop->Water == 0.5f));
		}

		TEST_METHOD(AWitheredCropWithoutTheMaterialKeepsTheMeshsOwnAndStillSlumps)
		{
			AddSoilArt();
			UStaticMesh* RipeMesh = nullptr;
			const UFarmCropData& Crop = MeshedCrop(10, RipeMesh);
			const FFarmCropGrowthLook GrowthLook = Crop.GetGrowthLook(1);

			const FFarmTileLook Look = ::FarmTileLooks::GetLook(WitheredTile(Crop, 1, 0), Settings, MaxWater, MaxContamination);

			ASSERT_THAT(IsTrue(Look.Crop.IsSet()));
			ASSERT_THAT(IsNull(Look.Crop->Material));
			ASSERT_THAT(IsTrue(Look.Crop->Transform.GetScale3D().Z < GrowthLook.Scale));
		}

		TEST_METHOD(AWitheredCropsHeightFactorIsClampedToItsRange)
		{
			AddSoilArt();
			UStaticMesh* RipeMesh = nullptr;
			const UFarmCropData& Crop = MeshedCrop(10, RipeMesh);
			const FFarmCropGrowthLook GrowthLook = Crop.GetGrowthLook(1);

			Settings.WitheredCropHeightScale = 5.f;
			const FFarmTileLook Tall = ::FarmTileLooks::GetLook(WitheredTile(Crop, 1, 0), Settings, MaxWater, MaxContamination);
			Settings.WitheredCropHeightScale = 0.f;
			const FFarmTileLook Flat = ::FarmTileLooks::GetLook(WitheredTile(Crop, 1, 0), Settings, MaxWater, MaxContamination);

			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Tall.Crop->Transform.GetScale3D().Z, GrowthLook.Scale)));
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Flat.Crop->Transform.GetScale3D().Z, GrowthLook.Scale * 0.1f)));
		}

		TEST_METHOD(AWitheredPlaceholderIsBrownGreyAndLowerWithItsBottomOnTheSoil)
		{
			const UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();
			const FFarmTileLook Growing = ::FarmTileLooks::GetLook(PlantedTile(Crop, 1), Settings, MaxWater, MaxContamination);

			const FFarmTileLook Withered = ::FarmTileLooks::GetLook(WitheredTile(Crop, 1, 0), Settings, MaxWater, MaxContamination);

			ASSERT_THAT(IsTrue(Withered.Crop.IsSet()));
			ASSERT_THAT(IsTrue(Withered.Crop->Mesh == Settings.PlaceholderCropMesh));
			ASSERT_THAT(IsTrue(Withered.Crop->Color == ::FarmTileLooks::WitheredColor));
			const float GrowingHeight = Growing.Crop->Transform.GetScale3D().Z;
			const float WitheredHeight = Withered.Crop->Transform.GetScale3D().Z;
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(WitheredHeight, GrowingHeight * Settings.WitheredCropHeightScale, 0.001f)));

			// The cylinder is 100 cm tall and centred on its origin: its bottom is its Z minus half its scaled height.
			const float Surface = Growing.Crop->Transform.GetLocation().Z - GrowingHeight * 50.f;
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Withered.Crop->Transform.GetLocation().Z - WitheredHeight * 50.f, Surface, 0.01f)));
		}

		TEST_METHOD(ClearedSoilShowsNoCrop)
		{
			AddSoilArt();
			const UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();
			FFarmTileState Cleared = WitheredTile(Crop, 1, 1);
			Cleared.ClearWithered();

			const FFarmTileLook Look = ::FarmTileLooks::GetLook(Cleared, Settings, MaxWater, MaxContamination);

			ASSERT_THAT(IsFalse(Look.Crop.IsSet()));
			ASSERT_THAT(IsTrue(Look.Soil.Mesh == Settings.TilledSoilMesh));
		}

		// Contamination (Systems/27-Contamination.md).
		TEST_METHOD(TheSoilCarriesTheContaminationAsAFractionOnGrassAndTilledSoilButTheCropNever)
		{
			AddSoilArt();
			const UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();
			const float Expected[] = {0.f, 0.5f, 1.f};

			for (int32 Level = 0; Level <= MaxContamination; ++Level)
			{
				const FString Name = FString::Printf(TEXT("level %d"), Level);
				FFarmTileState Grass;
				Grass.SetContamination(Level, MaxContamination);
				FFarmTileState Tilled = TilledTile();
				Tilled.SetContamination(Level, MaxContamination);
				FFarmTileState Planted = Tilled;
				Planted.Plant(&Crop);

				const FFarmTileLook GrassLook = ::FarmTileLooks::GetLook(Grass, Settings, MaxWater, MaxContamination);
				const FFarmTileLook TilledLook = ::FarmTileLooks::GetLook(Tilled, Settings, MaxWater, MaxContamination);
				const FFarmTileLook PlantedLook = ::FarmTileLooks::GetLook(Planted, Settings, MaxWater, MaxContamination);

				ASSERT_THAT(IsTrue(GrassLook.Soil.Contamination == Expected[Level], *Name));
				ASSERT_THAT(IsTrue(TilledLook.Soil.Contamination == Expected[Level], *Name));
				ASSERT_THAT(IsTrue(PlantedLook.Soil.Contamination == Expected[Level], *Name));
				ASSERT_THAT(IsTrue(PlantedLook.Crop.IsSet() && PlantedLook.Crop->Contamination == 0.f, *Name));
			}
		}

		TEST_METHOD(TheContaminationFollowsTheFieldsCap)
		{
			FFarmTileState Tile;
			Tile.SetContamination(1, 4);

			ASSERT_THAT(IsTrue(::FarmTileLooks::GetLook(Tile, Settings, MaxWater, 4).Soil.Contamination == 0.25f));
			ASSERT_THAT(IsTrue(::FarmTileLooks::GetLook(Tile, Settings, MaxWater, 0).Soil.Contamination == 1.f));
		}

		/** A ripe crop, mutated, restored as a save would. */
		static FFarmTileState MutatedTile(const UFarmCropData& Crop)
		{
			FFarmTileSaveData Saved = SavedTile(EFarmTileState::Planted, 0, Crop.GetDaysToMature());
			Saved.Contamination = 1;
			Saved.bMutated = true;
			FFarmTileState Tile;
			Tile.Restore(Saved, &Crop, MaxWater, MaxContamination);
			return Tile;
		}

		static const FPrimaryAssetId& MutatedItemId()
		{
			static const FPrimaryAssetId Id(TEXT("VestigeItem"), TEXT("Test.Mutated"));
			return Id;
		}

		TEST_METHOD(AMutatedCropShowsItsMutatedMeshWithTheMutatedMaterialAtFullSize)
		{
			AddSoilArt();
			Settings.MutatedCropMaterial = &Spawner.SpawnObject<UMaterial>();
			UStaticMesh* RipeMesh = nullptr;
			UFarmCropData& Crop = MeshedCrop(4, RipeMesh);
			UStaticMesh& MutatedMesh = Spawner.SpawnObject<UStaticMesh>();
			Crop.SetMutationForTests(MutatedItemId(), TSoftObjectPtr<UStaticMesh>(&MutatedMesh));
			const FFarmTileState Tile = MutatedTile(Crop);
			ASSERT_THAT(IsTrue(Tile.IsMutated()));

			const FFarmTileLook Look = ::FarmTileLooks::GetLook(Tile, Settings, MaxWater, MaxContamination);

			ASSERT_THAT(IsTrue(Look.Crop.IsSet()));
			ASSERT_THAT(IsTrue(Look.Crop->Mesh == &MutatedMesh));
			ASSERT_THAT(IsTrue(Look.Crop->Material == Settings.MutatedCropMaterial));
			ASSERT_THAT(IsTrue(Look.Crop->Transform.GetScale3D().Equals(FVector::OneVector)));
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Look.Crop->Transform.GetLocation().Z, Settings.SoilSurfaceHeight)));
		}

		TEST_METHOD(AMutatedCropWithoutItsOwnMeshOrTheMaterialShowsTheRipeMeshAsItIs)
		{
			AddSoilArt();
			UStaticMesh* RipeMesh = nullptr;
			UFarmCropData& Crop = MeshedCrop(4, RipeMesh);
			Crop.SetMutationForTests(MutatedItemId(), TSoftObjectPtr<UStaticMesh>());

			const FFarmTileLook Look = ::FarmTileLooks::GetLook(MutatedTile(Crop), Settings, MaxWater, MaxContamination);

			ASSERT_THAT(IsTrue(Look.Crop.IsSet()));
			ASSERT_THAT(IsTrue(Look.Crop->Mesh == RipeMesh));
			ASSERT_THAT(IsNull(Look.Crop->Material));

			// With the material but no mutated mesh: the ripe mesh in the mutated material.
			Settings.MutatedCropMaterial = &Spawner.SpawnObject<UMaterial>();
			const FFarmTileLook Tinted = ::FarmTileLooks::GetLook(MutatedTile(Crop), Settings, MaxWater, MaxContamination);
			ASSERT_THAT(IsTrue(Tinted.Crop->Mesh == RipeMesh));
			ASSERT_THAT(IsTrue(Tinted.Crop->Material == Settings.MutatedCropMaterial));
		}

		TEST_METHOD(AMutatedPlaceholderIsMagentaAtItsRipeHeight)
		{
			UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();
			const FFarmTileLook Ripe =
				::FarmTileLooks::GetLook(PlantedTile(Crop, Crop.GetDaysToMature()), Settings, MaxWater, MaxContamination);
			Crop.SetMutationForTests(MutatedItemId(), TSoftObjectPtr<UStaticMesh>());

			const FFarmTileLook Mutated = ::FarmTileLooks::GetLook(MutatedTile(Crop), Settings, MaxWater, MaxContamination);

			ASSERT_THAT(IsTrue(Mutated.Crop.IsSet()));
			ASSERT_THAT(IsTrue(Mutated.Crop->Mesh == Settings.PlaceholderCropMesh));
			ASSERT_THAT(IsTrue(Mutated.Crop->Color == ::FarmTileLooks::MutatedColor));
			ASSERT_THAT(IsTrue(Mutated.Crop->Color != ::FarmTileLooks::RipeColor && Mutated.Crop->Color != ::FarmTileLooks::WitheredColor));
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Mutated.Crop->Transform.GetScale3D().Z, Ripe.Crop->Transform.GetScale3D().Z)));
		}
	};
}

#endif
