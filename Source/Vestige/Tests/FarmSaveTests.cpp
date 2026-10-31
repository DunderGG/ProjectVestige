// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "Algo/Reverse.h"
#include "Components/ActorTestSpawner.h"
#include "Content/FarmCropData.h"
#include "CQTest.h"
#include "Farming/FarmGrid.h"
#include "Farming/FarmTile.h"
#include "Save/FarmSaveGame.h"
#include "Save/FarmSaveSubsystem.h"
#include "Time/FarmDayCycle.h"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace FarmSaveTests
{
	TEST_CLASS(Save, "Vestige")
	{
		FActorTestSpawner Spawner;

		/** A slot holding a readable save of this version and sequence number. */
		FFarmSaveSlotRead MakeSlot(int32 Version, int32 Sequence)
		{
			UFarmSaveGame& SaveGame = Spawner.SpawnObject<UFarmSaveGame>();
			SaveGame.SaveVersion = Version;
			SaveGame.SaveSequence = Sequence;
			return {true, &SaveGame};
		}

		static FFarmSaveSlotRead MakeUnreadableSlot() { return {true, nullptr}; }

		static FFarmSaveSlotRead MakeEmptySlot() { return {false, nullptr}; }

		/** A default 8 x 8 grid that has spawned its tiles. */
		AFarmGrid& MakeGrid()
		{
			AFarmGrid& Grid = Spawner.SpawnActor<AFarmGrid>();
			Grid.DispatchBeginPlay();
			return Grid;
		}

		const UFarmCropData& MakeCrop(FName ContentId)
		{
			UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();
			Crop.SetContentIdForTests(ContentId);
			return Crop;
		}

		TEST_METHOD(NoSaveFilesStartANewGame)
		{
			const FFarmSaveSlotRead Slots[] = {MakeEmptySlot(), MakeEmptySlot()};

			ASSERT_THAT(IsTrue(UFarmSaveSubsystem::DecideLoad(Slots).Outcome == EFarmLoadOutcome::NewGame));
		}

		TEST_METHOD(TheNewestReadableSaveIsLoaded)
		{
			const FFarmSaveSlotRead Slots[] = {MakeSlot(UFarmSaveGame::CurrentVersion, 4), MakeSlot(UFarmSaveGame::CurrentVersion, 5)};

			const FFarmLoadDecision Decision = UFarmSaveSubsystem::DecideLoad(Slots);

			ASSERT_THAT(IsTrue(Decision.Outcome == EFarmLoadOutcome::Load));
			ASSERT_THAT(IsTrue(Decision.Slot == 1));
		}

		TEST_METHOD(AnUnreadableSlotNextToAGoodOneLoadsTheGoodOne)
		{
			const FFarmSaveSlotRead Slots[] = {MakeSlot(UFarmSaveGame::CurrentVersion, 4), MakeUnreadableSlot()};

			const FFarmLoadDecision Decision = UFarmSaveSubsystem::DecideLoad(Slots);

			ASSERT_THAT(IsTrue(Decision.Outcome == EFarmLoadOutcome::Load));
			ASSERT_THAT(IsTrue(Decision.Slot == 0));
		}

		TEST_METHOD(AnUnreadableSaveIsRefused)
		{
			const FFarmSaveSlotRead Slots[] = {MakeUnreadableSlot(), MakeEmptySlot()};

			ASSERT_THAT(IsTrue(UFarmSaveSubsystem::DecideLoad(Slots).Outcome == EFarmLoadOutcome::RefuseUnreadable));
		}

		TEST_METHOD(ASaveFromANewerVersionIsRefusedEvenNextToAGoodOne)
		{
			const FFarmSaveSlotRead Slots[] = {MakeSlot(UFarmSaveGame::CurrentVersion, 9), MakeSlot(UFarmSaveGame::CurrentVersion + 1, 1)};

			ASSERT_THAT(IsTrue(UFarmSaveSubsystem::DecideLoad(Slots).Outcome == EFarmLoadOutcome::RefuseNewer));
		}

		TEST_METHOD(TheNextSaveNeverOverwritesTheNewestOne)
		{
			ASSERT_THAT(IsTrue(UFarmSaveSubsystem::GetSlotToWrite(INDEX_NONE) == 0));
			ASSERT_THAT(IsTrue(UFarmSaveSubsystem::GetSlotToWrite(0) == 1));
			ASSERT_THAT(IsTrue(UFarmSaveSubsystem::GetSlotToWrite(1) == 0));
		}

		TEST_METHOD(ANewerVersionIsNotUpgraded)
		{
			UFarmSaveGame& SaveGame = Spawner.SpawnObject<UFarmSaveGame>();
			SaveGame.SaveVersion = UFarmSaveGame::CurrentVersion + 1;

			ASSERT_THAT(IsFalse(UFarmSaveSubsystem::UpgradeToCurrentVersion(SaveGame, 8)));
		}

		TEST_METHOD(Version0UpgradesToTheCurrentLayout)
		{
			UFarmSaveGame& SaveGame = Spawner.SpawnObject<UFarmSaveGame>();
			SaveGame.Day = 4;
			SaveGame.TimeOfDay = 9.5f;
			SaveGame.Seeds = 7;
			SaveGame.HarvestedCrops = 2;
			SaveGame.CurrentToolId = TEXT("Base.WateringCan");
			SaveGame.Tiles.SetNum(10);
			SaveGame.Tiles[9].State = EFarmTileState::Planted;

			ASSERT_THAT(IsTrue(UFarmSaveSubsystem::UpgradeToCurrentVersion(SaveGame, 8)));

			ASSERT_THAT(IsTrue(SaveGame.SaveVersion == UFarmSaveGame::CurrentVersion));
			ASSERT_THAT(IsTrue(SaveGame.Clock.Day == 4));
			ASSERT_THAT(IsTrue(SaveGame.Clock.TimeOfDay == 9.5f));
			ASSERT_THAT(IsTrue(SaveGame.Inventory.Slots.Num() == 2));
			ASSERT_THAT(IsTrue(SaveGame.Inventory.Slots[0].Count == 7));
			ASSERT_THAT(IsTrue(SaveGame.Inventory.Slots[1].Count == 2));
			ASSERT_THAT(IsTrue(SaveGame.Farming.CurrentToolId == TEXT("Base.WateringCan")));
			ASSERT_THAT(IsTrue(SaveGame.Tiles[9].Coordinates == FIntPoint(1, 1)));
			ASSERT_THAT(IsTrue(SaveGame.Tiles[9].CropId == TEXT("Base.Turnip")));
			ASSERT_THAT(IsTrue(SaveGame.Tiles[0].CropId.IsNone()));
		}

		TEST_METHOD(Version0ToolEnumMapsToTheBaseToolIds)
		{
			const TPair<EFarmTool, FName> Expected[] = {{EFarmTool::Hoe, TEXT("Base.Hoe")}, {EFarmTool::Seeds, TEXT("Base.Seeds")},
				{EFarmTool::WateringCan, TEXT("Base.WateringCan")}, {EFarmTool::Hand, TEXT("Base.Hand")}};
			for (const TPair<EFarmTool, FName>& Pair : Expected)
			{
				UFarmSaveGame& SaveGame = Spawner.SpawnObject<UFarmSaveGame>();
				SaveGame.CurrentTool = Pair.Key;

				ASSERT_THAT(IsTrue(UFarmSaveSubsystem::UpgradeToCurrentVersion(SaveGame, 8)));
				ASSERT_THAT(IsTrue(SaveGame.Farming.CurrentToolId == Pair.Value));
			}
		}

		TEST_METHOD(ClockRoundTrips)
		{
			AFarmDayCycle& Saved = Spawner.SpawnActor<AFarmDayCycle>();
			Saved.StartNextDay();
			Saved.StartNextDay();
			AFarmDayCycle& Restored = Spawner.SpawnActor<AFarmDayCycle>();

			Restored.RestoreFromSaveData(Saved.GetSaveData());

			ASSERT_THAT(IsTrue(Restored.GetCurrentDay() == 3));
			ASSERT_THAT(IsTrue(Restored.GetTimeOfDay() == Saved.GetTimeOfDay()));
		}

		TEST_METHOD(Version1SeedsAndHarvestBecomeTurnipItems)
		{
			UFarmSaveGame& SaveGame = Spawner.SpawnObject<UFarmSaveGame>();
			SaveGame.SaveVersion = 1;
			SaveGame.Farming.Seeds = 5;
			SaveGame.Farming.HarvestedCrops = 3;

			ASSERT_THAT(IsTrue(UFarmSaveSubsystem::UpgradeToCurrentVersion(SaveGame, 8)));

			ASSERT_THAT(IsTrue(SaveGame.SaveVersion == UFarmSaveGame::CurrentVersion));
			ASSERT_THAT(IsTrue(SaveGame.Inventory.Slots.Num() == 2));
			ASSERT_THAT(IsTrue(SaveGame.Inventory.Slots[0].Slot == 0));
			ASSERT_THAT(IsTrue(SaveGame.Inventory.Slots[0].ItemId == TEXT("Base.TurnipSeeds")));
			ASSERT_THAT(IsTrue(SaveGame.Inventory.Slots[0].Count == 5));
			ASSERT_THAT(IsTrue(SaveGame.Inventory.Slots[1].Slot == 1));
			ASSERT_THAT(IsTrue(SaveGame.Inventory.Slots[1].ItemId == TEXT("Base.Turnip")));
			ASSERT_THAT(IsTrue(SaveGame.Inventory.Slots[1].Count == 3));
		}

		TEST_METHOD(Version1WithoutSeedsOrHarvestUpgradesToAnEmptyBag)
		{
			UFarmSaveGame& SaveGame = Spawner.SpawnObject<UFarmSaveGame>();
			SaveGame.SaveVersion = 1;
			SaveGame.Farming.HarvestedCrops = -2;

			ASSERT_THAT(IsTrue(UFarmSaveSubsystem::UpgradeToCurrentVersion(SaveGame, 8)));
			ASSERT_THAT(IsTrue(SaveGame.Inventory.Slots.IsEmpty()));
		}

		TEST_METHOD(TilesRoundTripByCoordinatesWithTheirOwnCrop)
		{
			const UFarmCropData& Turnip = MakeCrop(TEXT("Base.Turnip"));
			const UFarmCropData& Carrot = MakeCrop(TEXT("Base.Carrot"));
			AFarmGrid& Saved = MakeGrid();
			Saved.GetTileAt(FIntPoint(2, 3))->Till();
			Saved.GetTileAt(FIntPoint(2, 3))->Plant(&Carrot);
			Saved.GetTileAt(FIntPoint(5, 0))->Till();
			Saved.GetTileAt(FIntPoint(5, 0))->Water();

			// Reversed, so a tile can only land in the right place by its coordinates.
			TArray<FFarmTileSaveData> SavedTiles = Saved.GetTilesSaveData();
			Algo::Reverse(SavedTiles);
			AFarmGrid& Restored = MakeGrid();
			const int32 Skipped = Restored.RestoreTilesFromSaveData(
				SavedTiles, {{FName(TEXT("Base.Turnip")), &Turnip}, {FName(TEXT("Base.Carrot")), &Carrot}});

			ASSERT_THAT(IsTrue(Skipped == 0));
			ASSERT_THAT(IsTrue(Restored.GetTileAt(FIntPoint(2, 3))->GetCrop() == &Carrot));
			ASSERT_THAT(IsTrue(Restored.GetTileAt(FIntPoint(5, 0))->IsWatered()));
			ASSERT_THAT(IsTrue(Restored.GetTileAt(FIntPoint(0, 0))->GetState() == EFarmTileState::Grass));
		}

		TEST_METHOD(UnknownCropsAndTilesOutsideTheGridAreSkipped)
		{
			AFarmGrid& Grid = MakeGrid();
			FFarmTileSaveData Planted;
			Planted.Coordinates = FIntPoint(1, 1);
			Planted.State = EFarmTileState::Planted;
			Planted.CropId = TEXT("SomeMod.Pumpkin");
			FFarmTileSaveData Outside;
			Outside.Coordinates = FIntPoint(40, 2);
			Outside.State = EFarmTileState::Tilled;

			const int32 Skipped = Grid.RestoreTilesFromSaveData({Planted, Outside}, {});

			ASSERT_THAT(IsTrue(Skipped == 2));
			ASSERT_THAT(IsTrue(Grid.GetTileAt(FIntPoint(1, 1))->GetState() == EFarmTileState::Tilled));
			ASSERT_THAT(IsNull(Grid.GetTileAt(FIntPoint(1, 1))->GetCrop()));
		}
	};
}

#endif
