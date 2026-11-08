// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/ActorTestSpawner.h"
#include "Content/FarmCropData.h"
#include "Content/VestigeContentCatalog.h"
#include "Core/VestigeSaveRecord.h"
#include "CQTest.h"
#include "Farming/FarmField.h"
#include "Farming/FarmFieldMeshesComponent.h"
#include "Farming/FarmFieldSaveData.h"
#include "Farming/FarmFieldSubsystem.h"
#include "Farming/FarmGrid.h"
#include "Time/VestigeClockSubsystem.h"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace FarmFieldTests
{
	/** The rules of one field, without a level or a game instance. */
	TEST_CLASS(FarmField, "Vestige")
	{
		FActorTestSpawner Spawner;

		static inline const FIntPoint Tile = FIntPoint(2, 1);

		UFarmField& MakeField(const FIntPoint& Size = FIntPoint(4, 3))
		{
			UFarmField& NewField = Spawner.SpawnObject<UFarmField>();
			NewField.Initialize(TEXT("Base.Test"), Size);
			return NewField;
		}

		/** Waters the tile and starts a new day, Days times over. */
		static void WaterForDays(UFarmField& Field, int32 Days)
		{
			for (int32 Day = 0; Day < Days; ++Day)
			{
				Field.Water(Tile);
				Field.AdvanceDay();
			}
		}

		TEST_METHOD(ANewFieldIsGrassOfItsSize)
		{
			const UFarmField& Field = MakeField();

			ASSERT_THAT(IsTrue(Field.GetSize() == FIntPoint(4, 3)));
			ASSERT_THAT(IsTrue(Field.GetTile(FIntPoint(3, 2))->GetState() == EFarmTileState::Grass));
			ASSERT_THAT(IsNull(Field.GetTile(FIntPoint(4, 0))));
			ASSERT_THAT(IsNull(Field.GetTile(FIntPoint(0, -1))));
		}

		TEST_METHOD(ActionsChangeOnlyTheirTileAndSaySo)
		{
			UFarmField& Field = MakeField();
			TArray<FIntPoint> Changed;
			Field.OnTileChanged.AddLambda(
				[&Changed](const FIntPoint& Coordinates)
				{
					Changed.Add(Coordinates);
				});

			ASSERT_THAT(IsTrue(Field.Till(Tile)));
			ASSERT_THAT(IsFalse(Field.Till(Tile)));

			ASSERT_THAT(IsTrue(Field.GetTile(Tile)->GetState() == EFarmTileState::Tilled));
			ASSERT_THAT(IsTrue(Field.GetTile(FIntPoint(1, 1))->GetState() == EFarmTileState::Grass));
			ASSERT_THAT(IsTrue(Changed == TArray<FIntPoint>{Tile}));
		}

		TEST_METHOD(ActionsOutsideTheFieldChangeNothing)
		{
			UFarmField& Field = MakeField();

			ASSERT_THAT(IsFalse(Field.Till(FIntPoint(4, 0))));
			ASSERT_THAT(IsFalse(Field.Water(FIntPoint(-1, 0))));
		}

		TEST_METHOD(PlantingNeedsACrop)
		{
			UFarmField& Field = MakeField();
			ASSERT_THAT(IsTrue(Field.Till(Tile)));

			ASSERT_THAT(IsFalse(Field.Plant(Tile, nullptr)));
			ASSERT_THAT(IsTrue(Field.GetTile(Tile)->GetState() == EFarmTileState::Tilled));
		}

		TEST_METHOD(WateredCropRipensAfterItsDaysToMature)
		{
			const UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();
			UFarmField& Field = MakeField();
			ASSERT_THAT(IsTrue(Field.Till(Tile)));
			ASSERT_THAT(IsTrue(Field.Plant(Tile, &Crop)));

			WaterForDays(Field, Crop.GetDaysToMature() - 1);
			ASSERT_THAT(IsFalse(Field.GetTile(Tile)->IsRipe()));

			WaterForDays(Field, 1);
			ASSERT_THAT(IsTrue(Field.GetTile(Tile)->IsRipe()));
		}

		TEST_METHOD(HarvestingClearsTheCrop)
		{
			const UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();
			UFarmField& Field = MakeField();
			ASSERT_THAT(IsTrue(Field.Till(Tile)));
			ASSERT_THAT(IsTrue(Field.Plant(Tile, &Crop)));
			WaterForDays(Field, Crop.GetDaysToMature());

			ASSERT_THAT(IsTrue(Field.Harvest(Tile)));
			ASSERT_THAT(IsTrue(Field.GetTile(Tile)->GetState() == EFarmTileState::Tilled));
			ASSERT_THAT(IsNull(Field.GetTile(Tile)->GetCrop()));
		}

		TEST_METHOD(ANewDayChangesEveryTileAndSaysSoOnce)
		{
			UFarmField& Field = MakeField();
			Field.Till(Tile);
			Field.Water(Tile);
			int32 Broadcasts = 0;
			Field.OnTilesChanged.AddLambda(
				[&Broadcasts]()
				{
					++Broadcasts;
				});

			Field.AdvanceDay();

			ASSERT_THAT(IsFalse(Field.GetTile(Tile)->IsWatered()));
			ASSERT_THAT(IsTrue(Broadcasts == 1));
		}

		TEST_METHOD(ResizingKeepsTheTilesThatFit)
		{
			UFarmField& Field = MakeField();
			Field.Till(Tile);
			Field.Till(FIntPoint(3, 2));

			Field.Resize(FIntPoint(3, 5));

			ASSERT_THAT(IsTrue(Field.GetSize() == FIntPoint(3, 5)));
			ASSERT_THAT(IsTrue(Field.GetTile(Tile)->GetState() == EFarmTileState::Tilled));
			ASSERT_THAT(IsTrue(Field.GetTile(FIntPoint(2, 4))->GetState() == EFarmTileState::Grass));
			ASSERT_THAT(IsNull(Field.GetTile(FIntPoint(3, 2))));
		}

		TEST_METHOD(RestoredPlantedTileWithoutACropIsTilled)
		{
			UFarmField& Field = MakeField();
			FFarmTileSaveData Saved;
			Saved.Coordinates = Tile;
			Saved.State = EFarmTileState::Planted;
			Saved.bWatered = true;
			Saved.GrowthDays = 1;
			Saved.CropId = TEXT("Base.Turnip");

			const int32 Skipped = Field.RestoreTilesFromSaveData({Saved}, {});

			ASSERT_THAT(IsTrue(Skipped == 1));
			ASSERT_THAT(IsTrue(Field.GetTile(Tile)->GetState() == EFarmTileState::Tilled));
			ASSERT_THAT(IsTrue(Field.GetTile(Tile)->GetGrowthDays() == 0));
		}
	};

	/** The fields of the whole game, which outlive their maps, and the grids that show them. */
	TEST_CLASS(FarmFields, "Vestige")
	{
		FActorTestSpawner Spawner;
		UFarmFieldSubsystem* Fields = nullptr;
		UVestigeClockSubsystem* Clock = nullptr;

		static inline const FName FarmId = TEXT("Base.Farm");

		BEFORE_EACH()
		{
			Spawner.InitializeGameSubsystems();
			Fields = UFarmFieldSubsystem::Get(&Spawner.GetWorld());
			Clock = UVestigeClockSubsystem::Get(&Spawner.GetWorld());
			ASSERT_THAT(IsNotNull(Fields));
			ASSERT_THAT(IsNotNull(Clock));
		}

		/** A default 8 x 8 grid showing the field Base.Farm, as placed in L_Farm. */
		AFarmGrid& OpenGrid()
		{
			AFarmGrid& Grid = Spawner.SpawnActor<AFarmGrid>();
			Grid.DispatchBeginPlay();
			return Grid;
		}

		TEST_METHOD(AGridShowsItsFieldWithASoilInstancePerTile)
		{
			AFarmGrid& Grid = OpenGrid();

			ASSERT_THAT(IsTrue(Grid.GetField() == Fields->FindField(FarmId)));
			ASSERT_THAT(IsTrue(Grid.GetField()->GetSize() == FIntPoint(8, 8)));
			ASSERT_THAT(IsTrue(Grid.GetMeshes()->GetTotalInstanceCount() == 64));
			ASSERT_THAT(IsTrue(Grid.GetTile(FIntPoint(5, 6)) == Grid.GetField()->GetTile(FIntPoint(5, 6))));
			ASSERT_THAT(IsNull(Grid.GetTile(FIntPoint(8, 0))));
		}

		TEST_METHOD(AFieldGrowsOnANewDayWithoutItsMap)
		{
			const UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();
			UFarmField& Field = Fields->FindOrAddField(FarmId, FIntPoint(8, 8));
			Field.Till(FIntPoint(0, 0));
			Field.Plant(FIntPoint(0, 0), &Crop);
			Field.Water(FIntPoint(0, 0));

			Clock->StartNextDay();

			ASSERT_THAT(IsTrue(Field.GetTile(FIntPoint(0, 0))->GetGrowthDays() == 1));
			ASSERT_THAT(IsFalse(Field.GetTile(FIntPoint(0, 0))->IsWatered()));
		}

		TEST_METHOD(AMapOpenedAgainShowsItsFieldAsItWas)
		{
			AFarmGrid& FirstVisit = OpenGrid();
			FirstVisit.GetField()->Till(FIntPoint(3, 4));
			FirstVisit.Destroy();

			const AFarmGrid& SecondVisit = OpenGrid();

			ASSERT_THAT(IsTrue(SecondVisit.GetTile(FIntPoint(3, 4))->GetState() == EFarmTileState::Tilled));
		}

		TEST_METHOD(AFieldOfAnotherSizeTakesItsGridsSize)
		{
			UFarmField& Field = Fields->FindOrAddField(FarmId, FIntPoint(20, 20));
			Field.Till(FIntPoint(1, 1));
			Field.Till(FIntPoint(15, 15));

			const AFarmGrid& Grid = OpenGrid();

			ASSERT_THAT(IsTrue(Field.GetSize() == FIntPoint(8, 8)));
			ASSERT_THAT(IsTrue(Grid.GetTile(FIntPoint(1, 1))->GetState() == EFarmTileState::Tilled));
		}

		TEST_METHOD(ASavedCropIsFoundInTheCatalogue)
		{
			FFarmTileSaveData Turnip;
			Turnip.Coordinates = FIntPoint(1, 1);
			Turnip.State = EFarmTileState::Planted;
			Turnip.CropId = TEXT("Base.Turnip");
			FFarmTileSaveData Pumpkin = Turnip;
			Pumpkin.Coordinates = FIntPoint(2, 1);
			Pumpkin.CropId = TEXT("SomeMod.Pumpkin");
			FFarmFieldsSaveData Data;
			Data.Fields.Add({FarmId, FIntPoint(8, 8), {Turnip, Pumpkin}});
			int32 LeftOut = 0;

			ASSERT_THAT(IsTrue(Fields->ReadSaveRecord(FVestigeSaveRecord::Write(Data), LeftOut)));

			const UVestigeContentCatalog* Catalog = UVestigeContentCatalog::Get(&Spawner.GetWorld());
			const UFarmCropData* CatalogTurnip =
				Catalog->Find<UFarmCropData>(FPrimaryAssetId(UFarmCropData::AssetType, TEXT("Base.Turnip")));
			const UFarmField* Field = Fields->FindField(FarmId);
			ASSERT_THAT(IsNotNull(Field));
			ASSERT_THAT(IsNotNull(CatalogTurnip));
			ASSERT_THAT(IsTrue(Field->GetTile(FIntPoint(1, 1))->GetCrop() == CatalogTurnip));
			ASSERT_THAT(IsNull(Field->GetTile(FIntPoint(2, 1))->GetCrop()));
			ASSERT_THAT(IsTrue(LeftOut == 1));
		}

		TEST_METHOD(ANewGameDropsEveryField)
		{
			Fields->FindOrAddField(FarmId, FIntPoint(8, 8)).Till(FIntPoint(1, 1));

			Fields->StartNewGame();

			ASSERT_THAT(IsNull(Fields->FindField(FarmId)));
			ASSERT_THAT(IsTrue(Fields->GetSaveData().IsEmpty()));
		}
	};
}

#endif
