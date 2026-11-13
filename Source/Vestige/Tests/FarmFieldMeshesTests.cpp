// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/ActorTestSpawner.h"
#include "Content/FarmCropData.h"
#include "Content/VestigeContentCatalog.h"
#include "CQTest.h"
#include "EngineUtils.h"
#include "Farming/FarmField.h"
#include "Farming/FarmFieldMeshesComponent.h"
#include "Farming/FarmGrid.h"
#include "Farming/FarmTileLooks.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/UObjectIterator.h"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace FarmFieldMeshesTests
{
	/** How a grid draws its field: instances that follow the field's changes, and no actor or material per tile. */
	TEST_CLASS(FarmFieldMeshes, "Vestige")
	{
		FActorTestSpawner Spawner;
		AFarmGrid* Grid = nullptr;
		const UFarmFieldMeshesComponent* Meshes = nullptr;
		UFarmField* Field = nullptr;

		/** A default 8 x 8 grid with placeholder looks, playing. */
		BEFORE_EACH()
		{
			// The grid's field lives in the game instance.
			Spawner.InitializeGameSubsystems();
			Grid = &Spawner.SpawnActor<AFarmGrid>();
			Grid->DispatchBeginPlay();
			Meshes = Grid->GetMeshes();
			Field = Grid->GetField();
			ASSERT_THAT(IsNotNull(Meshes));
			ASSERT_THAT(IsNotNull(Field));
		}

		const FFarmTileLookSettings& Looks() const { return Meshes->GetLookSettings(); }

		/** How many instances draw a placeholder shape. */
		int32 PlaceholderCount(const UStaticMesh* Mesh) const { return Meshes->GetInstanceCount(Mesh, Looks().PlaceholderMaterial); }

		static bool HasColor(const TArray<float>& CustomData, const FLinearColor& Color)
		{
			return CustomData.Num() == FarmTileLooks::NumCustomDataFloats && CustomData[0] == Color.R && CustomData[1] == Color.G
				&& CustomData[2] == Color.B;
		}

		/** Tills, plants Crop in and waters the tile at Coordinates. */
		void PlantAndWater(const FIntPoint& Coordinates, const UFarmCropData& Crop)
		{
			Field->Till(Coordinates);
			Field->Plant(Coordinates, &Crop);
			Field->Water(Coordinates);
		}

		static int32 CountActors(UWorld& World)
		{
			int32 Count = 0;
			for (TActorIterator<AActor> It(&World); It; ++It)
			{
				++Count;
			}
			return Count;
		}

		static int32 CountDynamicMaterials()
		{
			int32 Count = 0;
			for (TObjectIterator<UMaterialInstanceDynamic> It; It; ++It)
			{
				++Count;
			}
			return Count;
		}

		TEST_METHOD(AFieldIsDrawnWithOneSoilInstancePerTileAndNoActorPerTile)
		{
			const int32 ActorsBefore = CountActors(Spawner.GetWorld());

			AFarmGrid& Other = Spawner.SpawnActor<AFarmGrid>();
			Other.DispatchBeginPlay();

			ASSERT_THAT(IsTrue(CountActors(Spawner.GetWorld()) == ActorsBefore + 1));
			ASSERT_THAT(IsTrue(PlaceholderCount(Looks().PlaceholderSoilMesh) == 64));
			ASSERT_THAT(IsTrue(Meshes->GetTotalInstanceCount() == 64));
		}

		TEST_METHOD(TillingAndWateringChangeOnlyThatTilesColor)
		{
			Field->Till(FIntPoint(1, 2));
			ASSERT_THAT(IsTrue(HasColor(Meshes->GetSoilCustomData(FIntPoint(1, 2)), FarmTileLooks::TilledColor)));

			Field->Water(FIntPoint(1, 2));
			ASSERT_THAT(IsTrue(HasColor(Meshes->GetSoilCustomData(FIntPoint(1, 2)), FarmTileLooks::WateredColor)));
			ASSERT_THAT(IsTrue(HasColor(Meshes->GetSoilCustomData(FIntPoint(0, 0)), FarmTileLooks::GrassColor)));
			ASSERT_THAT(IsTrue(Meshes->GetTotalInstanceCount() == 64));
		}

		TEST_METHOD(PlantingAddsACropInstanceAndHarvestingRemovesIt)
		{
			const UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();
			const FIntPoint Tile(4, 4);
			PlantAndWater(Tile, Crop);
			ASSERT_THAT(IsTrue(PlaceholderCount(Looks().PlaceholderCropMesh) == 1));

			for (int32 Day = 0; Day < Crop.GetDaysToMature(); ++Day)
			{
				Field->Water(Tile);
				Field->AdvanceDay();
			}
			ASSERT_THAT(IsTrue(Field->Harvest(Tile)));

			ASSERT_THAT(IsTrue(PlaceholderCount(Looks().PlaceholderCropMesh) == 0));
			ASSERT_THAT(IsTrue(Meshes->GetTotalInstanceCount() == 64));
		}

		TEST_METHOD(ANewDayMovesAGrowingCropToItsNextStagesMesh)
		{
			const FPrimaryAssetId TurnipId(UFarmCropData::AssetType, TEXT("Base.Turnip"));
			const UFarmCropData* Turnip = UVestigeContentCatalog::Get(&Spawner.GetWorld())->Find<UFarmCropData>(TurnipId);
			ASSERT_THAT(IsNotNull(Turnip));
			const UStaticMesh* Seedling = Turnip->GetGrowthLook(0).Mesh;
			const UStaticMesh* NextStage = Turnip->GetGrowthLook(1).Mesh;
			ASSERT_THAT(IsTrue(Seedling && NextStage && Seedling != NextStage));
			PlantAndWater(FIntPoint(2, 3), *Turnip);
			ASSERT_THAT(IsTrue(Meshes->GetInstanceCount(Seedling) == 1));

			Field->AdvanceDay();

			ASSERT_THAT(IsTrue(Meshes->GetInstanceCount(Seedling) == 0));
			ASSERT_THAT(IsTrue(Meshes->GetInstanceCount(NextStage) == 1));
		}

		TEST_METHOD(AResizeDrawsEveryTileOfTheNewSize)
		{
			Grid->Resize(FIntPoint(3, 2));

			ASSERT_THAT(IsTrue(Meshes->GetTotalInstanceCount() == 6));
			ASSERT_THAT(IsTrue(HasColor(Meshes->GetSoilCustomData(FIntPoint(2, 1)), FarmTileLooks::GrassColor)));
		}

		TEST_METHOD(ChangingTilesCreatesNoDynamicMaterial)
		{
			const UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();
			const int32 MaterialsBefore = CountDynamicMaterials();

			PlantAndWater(FIntPoint(0, 0), Crop);
			Field->Till(FIntPoint(1, 0));
			Field->AdvanceDay();

			ASSERT_THAT(IsTrue(CountDynamicMaterials() == MaterialsBefore));
		}

		/**
		 * The grid Blueprint levels place sets the art, and the placeholder material must have the instanced mesh usage saved,
		 * or the game draws the engine's default material instead. The editor sets a missing flag in memory and marks it dirty,
		 * so a dirty flag fails too.
		 */
		TEST_METHOD(TheGridBlueprintSetsTheArtAndAPlaceholderMaterialForInstances)
		{
			UClass* GridClass = LoadClass<AFarmGrid>(nullptr, TEXT("/Game/Vestige/Blueprints/BP_FarmGrid.BP_FarmGrid_C"));
			ASSERT_THAT(IsNotNull(GridClass));
			const AFarmGrid* GridDefaults = GetDefault<AFarmGrid>(GridClass);
			const FFarmTileLookSettings& BlueprintLooks = GridDefaults->GetMeshes()->GetLookSettings();
			ASSERT_THAT(IsNotNull(BlueprintLooks.TilledSoilMesh.Get()));
			ASSERT_THAT(IsNotNull(BlueprintLooks.WateredSoilMaterial.Get()));
			ASSERT_THAT(IsNotNull(BlueprintLooks.PlaceholderMaterial.Get()));

			UMaterial* BaseMaterial = BlueprintLooks.PlaceholderMaterial->GetMaterial();
			ASSERT_THAT(IsNotNull(BaseMaterial));
			ASSERT_THAT(IsTrue(BlueprintLooks.PlaceholderMaterial->GetUsageByFlag(MATUSAGE_InstancedStaticMeshes)));
			ASSERT_THAT(IsFalse(BaseMaterial->IsUsageFlagDirty(MATUSAGE_InstancedStaticMeshes)));
		}
	};
}

#endif
