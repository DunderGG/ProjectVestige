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
#include "Farming/FarmFieldSettings.h"
#include "Farming/FarmFieldSubsystem.h"
#include "Farming/FarmGrid.h"
#include "Serialization/MemoryReader.h"
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
			NewField.Initialize(TEXT("Base.Test"), Size, FFarmWaterRules());
			return NewField;
		}

		UFarmField& MakeFieldWithRules(const FFarmWaterRules& Rules)
		{
			UFarmField& NewField = Spawner.SpawnObject<UFarmField>();
			NewField.Initialize(TEXT("Base.Test"), FIntPoint(4, 3), Rules);
			return NewField;
		}

		static FFarmWaterRules MakeRules(int32 MaxLevel, int32 StepsPerWatering, int32 NightlyFall)
		{
			FFarmWaterRules Rules;
			Rules.MaxLevel = MaxLevel;
			Rules.StepsPerWatering = StepsPerWatering;
			Rules.NightlyFall = NightlyFall;
			return Rules;
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

		TEST_METHOD(AFieldWatersAndDriesByItsOwnRules)
		{
			UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();
			Crop.SetDaysToMatureForTests(5);
			UFarmField& Field = MakeFieldWithRules(MakeRules(3, 2, 1));
			Field.Till(Tile);
			Field.Plant(Tile, &Crop);

			ASSERT_THAT(IsTrue(Field.Water(Tile)));
			ASSERT_THAT(IsTrue(Field.GetTile(Tile)->GetWaterLevel() == 2));
			ASSERT_THAT(IsTrue(Field.Water(Tile)));
			ASSERT_THAT(IsTrue(Field.GetTile(Tile)->GetWaterLevel() == 3));
			ASSERT_THAT(IsFalse(Field.Water(Tile)));

			Field.AdvanceDay();

			ASSERT_THAT(IsTrue(Field.GetTile(Tile)->GetGrowthDays() == 1));
			ASSERT_THAT(IsTrue(Field.GetTile(Tile)->GetWaterLevel() == 2));
		}

		TEST_METHOD(ASoakedTileWateredAgainSaysNothingChanged)
		{
			UFarmField& Field = MakeField();
			Field.Till(Tile);
			Field.Water(Tile);
			Field.Water(Tile);
			int32 Changes = 0;
			Field.OnTileChanged.AddLambda(
				[&Changes](const FIntPoint&)
				{
					++Changes;
				});

			ASSERT_THAT(IsFalse(Field.Water(Tile)));

			ASSERT_THAT(IsTrue(Changes == 0));
			ASSERT_THAT(IsTrue(Field.GetTile(Tile)->GetWaterLevel() == 2));
		}

		TEST_METHOD(RulesBelowOneAreLiftedWhenAFieldIsMade)
		{
			UFarmField& Field = MakeFieldWithRules(MakeRules(0, 0, -1));
			Field.Till(Tile);

			ASSERT_THAT(IsTrue(Field.GetWaterRules().MaxLevel == 1));
			ASSERT_THAT(IsTrue(Field.GetWaterRules().StepsPerWatering == 1));
			ASSERT_THAT(IsTrue(Field.GetWaterRules().NightlyFall == 1));
			ASSERT_THAT(IsTrue(Field.Water(Tile)));
			ASSERT_THAT(IsFalse(Field.Water(Tile)));
		}

		TEST_METHOD(ResizingKeepsTheWaterRulesAndLevels)
		{
			UFarmField& Field = MakeFieldWithRules(MakeRules(3, 2, 1));
			Field.Till(Tile);
			Field.Water(Tile);

			Field.Resize(FIntPoint(5, 4));

			ASSERT_THAT(IsTrue(Field.GetWaterRules().MaxLevel == 3));
			ASSERT_THAT(IsTrue(Field.GetWaterRules().StepsPerWatering == 2));
			ASSERT_THAT(IsTrue(Field.GetTile(Tile)->GetWaterLevel() == 2));
			ASSERT_THAT(IsTrue(Field.Water(Tile)));
			ASSERT_THAT(IsTrue(Field.GetTile(Tile)->GetWaterLevel() == 3));
		}

		TEST_METHOD(ANewDayLowersEveryTilesWaterByOneStep)
		{
			UFarmField& Field = MakeField();
			Field.Till(FIntPoint(0, 0));
			Field.Till(FIntPoint(1, 0));
			Field.Water(FIntPoint(0, 0));
			Field.Water(FIntPoint(1, 0));
			Field.Water(FIntPoint(1, 0));

			Field.AdvanceDay();

			ASSERT_THAT(IsTrue(Field.GetTile(FIntPoint(0, 0))->GetWaterLevel() == 0));
			ASSERT_THAT(IsTrue(Field.GetTile(FIntPoint(1, 0))->GetWaterLevel() == 1));
		}

		TEST_METHOD(ARestoredWaterLevelAboveTheCapIsClamped)
		{
			UFarmField& Field = MakeField();
			FFarmTileSaveData Saved;
			Saved.Coordinates = Tile;
			Saved.State = EFarmTileState::Tilled;
			Saved.WaterLevel = 9;

			Field.RestoreTilesFromSaveData({Saved}, {});

			ASSERT_THAT(IsTrue(Field.GetTile(Tile)->GetWaterLevel() == Field.GetWaterRules().MaxLevel));
		}

		TEST_METHOD(ASavedWaterLevelComesBackAsSaved)
		{
			UFarmField& Field = MakeField();
			Field.Till(Tile);
			Field.Water(Tile);
			Field.Till(FIntPoint(0, 0));
			Field.Water(FIntPoint(0, 0));
			Field.Water(FIntPoint(0, 0));

			UFarmField& Other = MakeField();
			Other.RestoreTilesFromSaveData(Field.GetTilesSaveData(), {});

			ASSERT_THAT(IsTrue(Other.GetTile(Tile)->GetWaterLevel() == 1));
			ASSERT_THAT(IsTrue(Other.GetTile(FIntPoint(0, 0))->GetWaterLevel() == 2));
			ASSERT_THAT(IsTrue(Other.GetTile(FIntPoint(1, 0))->GetWaterLevel() == 0));
		}

		TEST_METHOD(RestoredPlantedTileWithoutACropIsTilled)
		{
			UFarmField& Field = MakeField();
			FFarmTileSaveData Saved;
			Saved.Coordinates = Tile;
			Saved.State = EFarmTileState::Planted;
			Saved.WaterLevel = 1;
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

		TEST_METHOD(AFieldTheSubsystemMakesFollowsTheFarmFieldSettings)
		{
			const FFarmWaterRules Expected = UFarmFieldSettings::Get().GetWaterRules();

			const FFarmWaterRules& Rules = Fields->FindOrAddField(FarmId, FIntPoint(8, 8)).GetWaterRules();

			ASSERT_THAT(IsTrue(Rules.MaxLevel == Expected.MaxLevel));
			ASSERT_THAT(IsTrue(Rules.StepsPerWatering == Expected.StepsPerWatering));
			ASSERT_THAT(IsTrue(Rules.NightlyFall == Expected.NightlyFall));
			ASSERT_THAT(IsTrue(Rules.MaxLevel >= 1 && Rules.StepsPerWatering >= 1 && Rules.NightlyFall >= 1));
		}

		TEST_METHOD(TheSaveKeepsEachTilesWaterLevel)
		{
			UFarmField& Field = Fields->FindOrAddField(FarmId, FIntPoint(8, 8));
			Field.Till(FIntPoint(1, 1));
			Field.Water(FIntPoint(1, 1));
			Field.Till(FIntPoint(2, 1));
			Field.Water(FIntPoint(2, 1));
			Field.Water(FIntPoint(2, 1));
			Field.Till(FIntPoint(3, 1));
			const FVestigeSaveRecord Record = Fields->WriteSaveRecord();
			Fields->StartNewGame();
			int32 LeftOut = 0;

			ASSERT_THAT(IsTrue(Fields->ReadSaveRecord(Record, LeftOut)));

			const UFarmField* Restored = Fields->FindField(FarmId);
			ASSERT_THAT(IsNotNull(Restored));
			ASSERT_THAT(IsTrue(Restored->GetTile(FIntPoint(1, 1))->GetWaterLevel() == 1));
			ASSERT_THAT(IsTrue(Restored->GetTile(FIntPoint(2, 1))->GetWaterLevel() == 2));
			ASSERT_THAT(IsTrue(Restored->GetTile(FIntPoint(3, 1))->GetWaterLevel() == 0));
		}

		/**
		 * A real record of the layout from before tiles held a water level (a bool bWatered, true here), captured before the
		 * struct changed: field Base.Test 4 x 3, tile (2, 1) planted with Base.Parsnip, growth 2 days.
		 */
		TEST_METHOD(ASaveFromBeforeWaterLevelsLoadsDryWithItsCropAndGrowth)
		{
			// The captured crop is not a base crop, so the catalogue gets a made-up one with its ID.
			UFarmCropData& Parsnip = Spawner.SpawnObject<UFarmCropData>();
			Parsnip.SetContentIdForTests(TEXT("Base.Parsnip"));
			ASSERT_THAT(IsTrue(UVestigeContentCatalog::Get(&Spawner.GetWorld())->AddForTests(Parsnip)));

			// clang-format off
			static const uint8 OldTileRecordBytes[] = {
				0x01, 0x00, 0x00, 0x00, 0x0A, 0x02, 0x00, 0x00, 0xFA, 0x03, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0xFB, 0x02, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
				0x46, 0x69, 0x65, 0x6C, 0x64, 0x73, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x41,
				0x72, 0x72, 0x61, 0x79, 0x50, 0x72, 0x6F, 0x70, 0x65, 0x72, 0x74, 0x79,
				0x00, 0x01, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x53, 0x74, 0x72,
				0x75, 0x63, 0x74, 0x50, 0x72, 0x6F, 0x70, 0x65, 0x72, 0x74, 0x79, 0x00,
				0x01, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x46, 0x61, 0x72, 0x6D,
				0x46, 0x69, 0x65, 0x6C, 0x64, 0x53, 0x61, 0x76, 0x65, 0x44, 0x61, 0x74,
				0x61, 0x00, 0x01, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x2F, 0x53,
				0x63, 0x72, 0x69, 0x70, 0x74, 0x2F, 0x56, 0x65, 0x73, 0x74, 0x69, 0x67,
				0x65, 0x00, 0x00, 0x00, 0x00, 0x00, 0x83, 0x02, 0x00, 0x00, 0x00, 0x01,
				0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x46, 0x69, 0x65, 0x6C, 0x64,
				0x49, 0x64, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x4E, 0x61, 0x6D, 0x65, 0x50,
				0x72, 0x6F, 0x70, 0x65, 0x72, 0x74, 0x79, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x0E, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x42, 0x61, 0x73,
				0x65, 0x2E, 0x54, 0x65, 0x73, 0x74, 0x00, 0x05, 0x00, 0x00, 0x00, 0x53,
				0x69, 0x7A, 0x65, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x53, 0x74, 0x72, 0x75,
				0x63, 0x74, 0x50, 0x72, 0x6F, 0x70, 0x65, 0x72, 0x74, 0x79, 0x00, 0x01,
				0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x49, 0x6E, 0x74, 0x50, 0x6F,
				0x69, 0x6E, 0x74, 0x00, 0x01, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00,
				0x2F, 0x53, 0x63, 0x72, 0x69, 0x70, 0x74, 0x2F, 0x43, 0x6F, 0x72, 0x65,
				0x55, 0x4F, 0x62, 0x6A, 0x65, 0x63, 0x74, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x08, 0x00, 0x00, 0x00, 0x08, 0x04, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00,
				0x00, 0x06, 0x00, 0x00, 0x00, 0x54, 0x69, 0x6C, 0x65, 0x73, 0x00, 0x0E,
				0x00, 0x00, 0x00, 0x41, 0x72, 0x72, 0x61, 0x79, 0x50, 0x72, 0x6F, 0x70,
				0x65, 0x72, 0x74, 0x79, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00,
				0x00, 0x53, 0x74, 0x72, 0x75, 0x63, 0x74, 0x50, 0x72, 0x6F, 0x70, 0x65,
				0x72, 0x74, 0x79, 0x00, 0x01, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00,
				0x46, 0x61, 0x72, 0x6D, 0x54, 0x69, 0x6C, 0x65, 0x53, 0x61, 0x76, 0x65,
				0x44, 0x61, 0x74, 0x61, 0x00, 0x01, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00,
				0x00, 0x2F, 0x53, 0x63, 0x72, 0x69, 0x70, 0x74, 0x2F, 0x56, 0x65, 0x73,
				0x74, 0x69, 0x67, 0x65, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7B, 0x01, 0x00,
				0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x43, 0x6F,
				0x6F, 0x72, 0x64, 0x69, 0x6E, 0x61, 0x74, 0x65, 0x73, 0x00, 0x0F, 0x00,
				0x00, 0x00, 0x53, 0x74, 0x72, 0x75, 0x63, 0x74, 0x50, 0x72, 0x6F, 0x70,
				0x65, 0x72, 0x74, 0x79, 0x00, 0x01, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00,
				0x00, 0x49, 0x6E, 0x74, 0x50, 0x6F, 0x69, 0x6E, 0x74, 0x00, 0x01, 0x00,
				0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x2F, 0x53, 0x63, 0x72, 0x69, 0x70,
				0x74, 0x2F, 0x43, 0x6F, 0x72, 0x65, 0x55, 0x4F, 0x62, 0x6A, 0x65, 0x63,
				0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x08, 0x02,
				0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x53,
				0x74, 0x61, 0x74, 0x65, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x45, 0x6E, 0x75,
				0x6D, 0x50, 0x72, 0x6F, 0x70, 0x65, 0x72, 0x74, 0x79, 0x00, 0x02, 0x00,
				0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x45, 0x46, 0x61, 0x72, 0x6D, 0x54,
				0x69, 0x6C, 0x65, 0x53, 0x74, 0x61, 0x74, 0x65, 0x00, 0x01, 0x00, 0x00,
				0x00, 0x10, 0x00, 0x00, 0x00, 0x2F, 0x53, 0x63, 0x72, 0x69, 0x70, 0x74,
				0x2F, 0x56, 0x65, 0x73, 0x74, 0x69, 0x67, 0x65, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x0D, 0x00, 0x00, 0x00, 0x42, 0x79, 0x74, 0x65, 0x50, 0x72, 0x6F,
				0x70, 0x65, 0x72, 0x74, 0x79, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1C, 0x00,
				0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x45, 0x46, 0x61, 0x72, 0x6D,
				0x54, 0x69, 0x6C, 0x65, 0x53, 0x74, 0x61, 0x74, 0x65, 0x3A, 0x3A, 0x50,
				0x6C, 0x61, 0x6E, 0x74, 0x65, 0x64, 0x00, 0x09, 0x00, 0x00, 0x00, 0x62,
				0x57, 0x61, 0x74, 0x65, 0x72, 0x65, 0x64, 0x00, 0x0D, 0x00, 0x00, 0x00,
				0x42, 0x6F, 0x6F, 0x6C, 0x50, 0x72, 0x6F, 0x70, 0x65, 0x72, 0x74, 0x79,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x0B, 0x00,
				0x00, 0x00, 0x47, 0x72, 0x6F, 0x77, 0x74, 0x68, 0x44, 0x61, 0x79, 0x73,
				0x00, 0x0C, 0x00, 0x00, 0x00, 0x49, 0x6E, 0x74, 0x50, 0x72, 0x6F, 0x70,
				0x65, 0x72, 0x74, 0x79, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00,
				0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x43, 0x72,
				0x6F, 0x70, 0x49, 0x64, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x4E, 0x61, 0x6D,
				0x65, 0x50, 0x72, 0x6F, 0x70, 0x65, 0x72, 0x74, 0x79, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x42,
				0x61, 0x73, 0x65, 0x2E, 0x50, 0x61, 0x72, 0x73, 0x6E, 0x69, 0x70, 0x00,
				0x05, 0x00, 0x00, 0x00, 0x4E, 0x6F, 0x6E, 0x65, 0x00, 0x05, 0x00, 0x00,
				0x00, 0x4E, 0x6F, 0x6E, 0x65, 0x00, 0x05, 0x00, 0x00, 0x00, 0x4E, 0x6F,
				0x6E, 0x65, 0x00,
			};
			// clang-format on
			const TArray<uint8> Bytes(OldTileRecordBytes, UE_ARRAY_COUNT(OldTileRecordBytes));
			FMemoryReader Reader(Bytes);
			FVestigeSaveRecord Record;
			ASSERT_THAT(IsTrue(Record.Serialize(Reader)));
			int32 LeftOut = 0;

			ASSERT_THAT(IsTrue(Fields->ReadSaveRecord(Record, LeftOut)));

			const UFarmField* Restored = Fields->FindField(TEXT("Base.Test"));
			ASSERT_THAT(IsNotNull(Restored));
			const FFarmTileState* Tile = Restored->GetTile(FIntPoint(2, 1));
			ASSERT_THAT(IsNotNull(Tile));
			ASSERT_THAT(IsTrue(Tile->GetState() == EFarmTileState::Planted));
			ASSERT_THAT(IsTrue(Tile->GetGrowthDays() == 2));
			ASSERT_THAT(IsTrue(Tile->GetCrop() && Tile->GetCrop()->GetContentId() == TEXT("Base.Parsnip")));
			ASSERT_THAT(IsTrue(Tile->GetWaterLevel() == 0));
			ASSERT_THAT(IsTrue(LeftOut == 0));
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
