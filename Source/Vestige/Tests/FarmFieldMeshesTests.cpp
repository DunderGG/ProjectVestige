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
#include "Materials/MaterialExpressionMaterialFunctionCall.h"
#include "Materials/MaterialExpressionPerInstanceCustomData.h"
#include "Materials/MaterialFunctionInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Time/VestigeClockSubsystem.h"
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
			return CustomData.Num() == FarmTileLooks::NumCustomDataFloats && FMath::IsNearlyEqual(CustomData[0], Color.R)
				&& FMath::IsNearlyEqual(CustomData[1], Color.G) && FMath::IsNearlyEqual(CustomData[2], Color.B);
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
			Field->Water(FIntPoint(1, 2));
			ASSERT_THAT(IsTrue(HasColor(Meshes->GetSoilCustomData(FIntPoint(1, 2)), FarmTileLooks::WateredColor)));
			ASSERT_THAT(IsTrue(HasColor(Meshes->GetSoilCustomData(FIntPoint(0, 0)), FarmTileLooks::GrassColor)));
			ASSERT_THAT(IsTrue(Meshes->GetTotalInstanceCount() == 64));
		}

		TEST_METHOD(WateringChangesOnlyTheSoilsWaterCustomDataAndKeepsItsSet)
		{
			const FIntPoint Tile(1, 2);
			Field->Till(Tile);
			const int32 SoilBefore = PlaceholderCount(Looks().PlaceholderSoilMesh);
			ASSERT_THAT(IsTrue(Meshes->GetSoilCustomData(Tile)[FarmTileLooks::WaterCustomDataIndex] == 0.f));

			Field->Water(Tile);
			ASSERT_THAT(IsTrue(Meshes->GetSoilCustomData(Tile)[FarmTileLooks::WaterCustomDataIndex] == 0.5f));
			Field->Water(Tile);
			ASSERT_THAT(IsTrue(Meshes->GetSoilCustomData(Tile)[FarmTileLooks::WaterCustomDataIndex] == 1.f));

			ASSERT_THAT(IsTrue(PlaceholderCount(Looks().PlaceholderSoilMesh) == SoilBefore));
			ASSERT_THAT(IsTrue(Meshes->GetTotalInstanceCount() == 64));
			ASSERT_THAT(IsTrue(Meshes->GetSoilCustomData(FIntPoint(0, 0))[FarmTileLooks::WaterCustomDataIndex] == 0.f));
		}

		TEST_METHOD(ANightLowersTheSoilsWaterCustomData)
		{
			const FIntPoint Tile(1, 2);
			Field->Till(Tile);
			Field->Water(Tile);
			Field->Water(Tile);

			Field->AdvanceDay(1, 0);

			ASSERT_THAT(IsTrue(Meshes->GetSoilCustomData(Tile)[FarmTileLooks::WaterCustomDataIndex] == 0.5f));
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
				Field->AdvanceDay(1, 0);
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

			Field->AdvanceDay(1, 0);

			ASSERT_THAT(IsTrue(Meshes->GetInstanceCount(Seedling) == 0));
			ASSERT_THAT(IsTrue(Meshes->GetInstanceCount(NextStage) == 1));
		}

		/** A second grid, from the Blueprint levels place, showing the same field with the real art and withered material. */
		AFarmGrid* SpawnBlueprintGrid()
		{
			UClass* GridClass = LoadClass<AFarmGrid>(nullptr, TEXT("/Game/Vestige/Blueprints/BP_FarmGrid.BP_FarmGrid_C"));
			if (!GridClass)
			{
				return nullptr;
			}
			AFarmGrid& BlueprintGrid = Spawner.SpawnActor<AFarmGrid>(FActorSpawnParameters(), GridClass);
			BlueprintGrid.DispatchBeginPlay();
			return &BlueprintGrid;
		}

		TEST_METHOD(WitheringMovesTheCropToTheWitheredSetAndClearingRemovesIt)
		{
			const FPrimaryAssetId TurnipId(UFarmCropData::AssetType, TEXT("Base.Turnip"));
			const UFarmCropData* Turnip = UVestigeContentCatalog::Get(&Spawner.GetWorld())->Find<UFarmCropData>(TurnipId);
			ASSERT_THAT(IsNotNull(Turnip));
			const UVestigeClockSubsystem* Clock = UVestigeClockSubsystem::Get(&Spawner.GetWorld());
			ASSERT_THAT(IsNotNull(Clock));
			const AFarmGrid* BlueprintGrid = SpawnBlueprintGrid();
			ASSERT_THAT(IsNotNull(BlueprintGrid));
			const UFarmFieldMeshesComponent& BlueprintMeshes = *BlueprintGrid->GetMeshes();
			UMaterialInterface* WitheredMaterial = BlueprintMeshes.GetLookSettings().WitheredCropMaterial;
			ASSERT_THAT(IsNotNull(WitheredMaterial));
			const UStaticMesh* Stage = Turnip->GetGrowthLook(0).Mesh;
			ASSERT_THAT(IsNotNull(Stage));
			PlantAndWater(FIntPoint(2, 3), *Turnip);
			ASSERT_THAT(IsTrue(BlueprintMeshes.GetInstanceCount(Stage) == 1));
			const int32 TotalBefore = BlueprintMeshes.GetTotalInstanceCount();

			const int32 Withered = Field->WitherCropsOutOfSeason(Clock->GetCalendar().GetSeasons()[1].Tag, Clock->GetCalendar());

			ASSERT_THAT(IsTrue(Withered == 1));
			ASSERT_THAT(IsTrue(BlueprintMeshes.GetInstanceCount(Stage) == 0));
			ASSERT_THAT(IsTrue(BlueprintMeshes.GetInstanceCount(Stage, WitheredMaterial) == 1));
			ASSERT_THAT(IsTrue(BlueprintMeshes.GetTotalInstanceCount() == TotalBefore));

			ASSERT_THAT(IsTrue(Field->ClearWithered(FIntPoint(2, 3))));
			ASSERT_THAT(IsTrue(BlueprintMeshes.GetInstanceCount(Stage, WitheredMaterial) == 0));
			ASSERT_THAT(IsTrue(BlueprintMeshes.GetTotalInstanceCount() == TotalBefore - 1));
		}

		TEST_METHOD(TheGridBlueprintsWitheredMaterialIsAnInstanceOfTheCropMaterialForInstancedMeshes)
		{
			const AFarmGrid* BlueprintGrid = SpawnBlueprintGrid();
			ASSERT_THAT(IsNotNull(BlueprintGrid));
			UMaterialInterface* Withered = BlueprintGrid->GetMeshes()->GetLookSettings().WitheredCropMaterial;
			ASSERT_THAT(IsNotNull(Withered));

			ASSERT_THAT(IsTrue(Withered->GetUsageByFlag(MATUSAGE_InstancedStaticMeshes)));
			UMaterial* BaseMaterial = Withered->GetMaterial();
			ASSERT_THAT(IsNotNull(BaseMaterial));
			ASSERT_THAT(IsTrue(BaseMaterial->GetName() == TEXT("M_Crop")));
			ASSERT_THAT(IsFalse(BaseMaterial->IsUsageFlagDirty(MATUSAGE_InstancedStaticMeshes)));
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
			Field->AdvanceDay(1, 0);

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
			ASSERT_THAT(IsNotNull(BlueprintLooks.PlaceholderMaterial.Get()));

			UMaterial* BaseMaterial = BlueprintLooks.PlaceholderMaterial->GetMaterial();
			ASSERT_THAT(IsNotNull(BaseMaterial));
			ASSERT_THAT(IsTrue(BlueprintLooks.PlaceholderMaterial->GetUsageByFlag(MATUSAGE_InstancedStaticMeshes)));
			ASSERT_THAT(IsFalse(BaseMaterial->IsUsageFlagDirty(MATUSAGE_InstancedStaticMeshes)));
		}

		/**
		 * The soil's material is an instance of M_FarmSoil, which reads the tile's water from custom data index
		 * WaterCustomDataIndex, and every material on the mesh has the instanced mesh usage saved.
		 */
		TEST_METHOD(TheTilledSoilMaterialReadsTheWaterFromTheCustomData)
		{
			UClass* GridClass = LoadClass<AFarmGrid>(nullptr, TEXT("/Game/Vestige/Blueprints/BP_FarmGrid.BP_FarmGrid_C"));
			ASSERT_THAT(IsNotNull(GridClass));
			const UStaticMesh* SoilMesh = GetDefault<AFarmGrid>(GridClass)->GetMeshes()->GetLookSettings().TilledSoilMesh.Get();
			ASSERT_THAT(IsNotNull(SoilMesh));
			ASSERT_THAT(IsFalse(SoilMesh->GetStaticMaterials().IsEmpty()));

			for (const FStaticMaterial& Slot : SoilMesh->GetStaticMaterials())
			{
				const UMaterialInterface* Material = Slot.MaterialInterface;
				ASSERT_THAT(IsNotNull(Material));
				ASSERT_THAT(IsTrue(Material->GetUsageByFlag(MATUSAGE_InstancedStaticMeshes)));
				const UMaterial* BaseMaterial = Material->GetMaterial();
				ASSERT_THAT(IsNotNull(BaseMaterial));
				ASSERT_THAT(IsTrue(BaseMaterial->GetName() == TEXT("M_FarmSoil")));
#if WITH_EDITORONLY_DATA
				bool bReadsWater = false;
				for (const UMaterialExpression* Expression : BaseMaterial->GetExpressions())
				{
					const UMaterialExpressionPerInstanceCustomData* CustomData = Cast<UMaterialExpressionPerInstanceCustomData>(Expression);
					bReadsWater |= CustomData && CustomData->DataIndex == FarmTileLooks::WaterCustomDataIndex;
				}
				ASSERT_THAT(IsTrue(bReadsWater));
#endif
			}
		}

		// Contamination (Systems/27-Contamination.md).
		TEST_METHOD(ALevelChangeUpdatesTheSoilsContaminationCustomDataInPlace)
		{
			const FIntPoint Tile(1, 2);
			Field->Till(Tile);
			const int32 SoilBefore = PlaceholderCount(Looks().PlaceholderSoilMesh);
			ASSERT_THAT(IsTrue(Meshes->GetSoilCustomData(Tile).Num() == 5));
			ASSERT_THAT(IsTrue(Meshes->GetSoilCustomData(Tile)[FarmTileLooks::ContaminationCustomDataIndex] == 0.f));

			Field->SetContamination(Tile, 1);
			ASSERT_THAT(IsTrue(Meshes->GetSoilCustomData(Tile)[FarmTileLooks::ContaminationCustomDataIndex] == 0.5f));
			Field->SetContamination(Tile, 2);
			ASSERT_THAT(IsTrue(Meshes->GetSoilCustomData(Tile)[FarmTileLooks::ContaminationCustomDataIndex] == 1.f));
			Field->SetContamination(FIntPoint(0, 0), 1);
			ASSERT_THAT(IsTrue(Meshes->GetSoilCustomData(FIntPoint(0, 0))[FarmTileLooks::ContaminationCustomDataIndex] == 0.5f));

			ASSERT_THAT(IsTrue(PlaceholderCount(Looks().PlaceholderSoilMesh) == SoilBefore));
			ASSERT_THAT(IsTrue(Meshes->GetTotalInstanceCount() == 64));
			ASSERT_THAT(IsTrue(Meshes->GetSoilCustomData(FIntPoint(3, 3))[FarmTileLooks::ContaminationCustomDataIndex] == 0.f));
			ASSERT_THAT(IsTrue(HasColor(Meshes->GetSoilCustomData(Tile), FarmTileLooks::TilledColor)));
		}

		TEST_METHOD(StartingAreasRedrawTheWholeField)
		{
			FFarmContaminationArea Area;
			Area.Min = FIntPoint(6, 0);
			Area.Max = FIntPoint(7, 7);
			Area.Level = 2;

			Field->ApplyContaminationAreas(TArray<FFarmContaminationArea>{Area});

			ASSERT_THAT(IsTrue(Meshes->GetSoilCustomData(FIntPoint(7, 7))[FarmTileLooks::ContaminationCustomDataIndex] == 1.f));
			ASSERT_THAT(IsTrue(Meshes->GetSoilCustomData(FIntPoint(5, 7))[FarmTileLooks::ContaminationCustomDataIndex] == 0.f));
			ASSERT_THAT(IsTrue(Meshes->GetTotalInstanceCount() == 64));
		}

		/** Whether Expressions, or a material function they call, read per-instance custom data Index. */
		static bool ReadsCustomData(TConstArrayView<TObjectPtr<UMaterialExpression>> Expressions, int32 Index, int32 Depth = 0)
		{
#if WITH_EDITORONLY_DATA
			for (const UMaterialExpression* Expression : Expressions)
			{
				const UMaterialExpressionPerInstanceCustomData* CustomData = Cast<UMaterialExpressionPerInstanceCustomData>(Expression);
				if (CustomData && CustomData->DataIndex == static_cast<uint32>(Index))
				{
					return true;
				}
				const UMaterialExpressionMaterialFunctionCall* Call = Cast<UMaterialExpressionMaterialFunctionCall>(Expression);
				if (Call && Call->MaterialFunction && Depth < 4
					&& ReadsCustomData(Call->MaterialFunction->GetExpressions(), Index, Depth + 1))
				{
					return true;
				}
			}
#endif
			return false;
		}

		/** The soil's and the placeholder's base materials read the contamination, directly or through MF_ContaminationCracks. */
		TEST_METHOD(TheSoilAndPlaceholderMaterialsReadTheContaminationFromTheCustomData)
		{
			UClass* GridClass = LoadClass<AFarmGrid>(nullptr, TEXT("/Game/Vestige/Blueprints/BP_FarmGrid.BP_FarmGrid_C"));
			ASSERT_THAT(IsNotNull(GridClass));
			const FFarmTileLookSettings& BlueprintLooks = GetDefault<AFarmGrid>(GridClass)->GetMeshes()->GetLookSettings();
			ASSERT_THAT(IsNotNull(BlueprintLooks.TilledSoilMesh.Get()));
			ASSERT_THAT(IsNotNull(BlueprintLooks.PlaceholderMaterial.Get()));

			TArray<const UMaterial*> Materials = {BlueprintLooks.PlaceholderMaterial->GetMaterial()};
			for (const FStaticMaterial& Slot : BlueprintLooks.TilledSoilMesh->GetStaticMaterials())
			{
				Materials.Add(Slot.MaterialInterface ? Slot.MaterialInterface->GetMaterial() : nullptr);
			}
			for (const UMaterial* BaseMaterial : Materials)
			{
				ASSERT_THAT(IsNotNull(BaseMaterial));
#if WITH_EDITORONLY_DATA
				ASSERT_THAT(IsTrue(ReadsCustomData(BaseMaterial->GetExpressions(), FarmTileLooks::ContaminationCustomDataIndex),
					*BaseMaterial->GetName()));
#endif
			}
		}

		/** Fails until MI_Crop_Mutated is imported and set on BP_FarmGrid (the artist's part 3, waiting for the mutated meshes). */
		TEST_METHOD(TheGridBlueprintsMutatedMaterialIsAnInstanceOfTheCropMaterialForInstancedMeshes)
		{
			UClass* GridClass = LoadClass<AFarmGrid>(nullptr, TEXT("/Game/Vestige/Blueprints/BP_FarmGrid.BP_FarmGrid_C"));
			ASSERT_THAT(IsNotNull(GridClass));
			UMaterialInterface* Mutated = GetDefault<AFarmGrid>(GridClass)->GetMeshes()->GetLookSettings().MutatedCropMaterial;
			ASSERT_THAT(IsNotNull(Mutated));

			ASSERT_THAT(IsTrue(Mutated->GetUsageByFlag(MATUSAGE_InstancedStaticMeshes)));
			UMaterial* BaseMaterial = Mutated->GetMaterial();
			ASSERT_THAT(IsNotNull(BaseMaterial));
			ASSERT_THAT(IsTrue(BaseMaterial->GetName() == TEXT("M_Crop")));
			ASSERT_THAT(IsFalse(BaseMaterial->IsUsageFlagDirty(MATUSAGE_InstancedStaticMeshes)));
			ASSERT_THAT(IsTrue(Mutated != GetDefault<AFarmGrid>(GridClass)->GetMeshes()->GetLookSettings().WitheredCropMaterial));
		}
	};
}

#endif
