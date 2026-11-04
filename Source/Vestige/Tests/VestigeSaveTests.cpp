// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "Algo/Reverse.h"
#include "Components/ActorTestSpawner.h"
#include "Content/FarmCropData.h"
#include "CQTest.h"
#include "Farming/FarmGrid.h"
#include "Farming/FarmTile.h"
#include "Save/VestigeSaveGame.h"
#include "Save/VestigeSaveSubsystem.h"
#include "Time/VestigeDayCycle.h"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace VestigeSaveTests
{
	TEST_CLASS(Save, "Vestige")
	{
		FActorTestSpawner Spawner;

		/** A slot holding a readable save of this version and sequence number. */
		FVestigeSaveSlotRead MakeSlot(int32 Version, int32 Sequence)
		{
			UVestigeSaveGame& SaveGame = Spawner.SpawnObject<UVestigeSaveGame>();
			SaveGame.SaveVersion = Version;
			SaveGame.SaveSequence = Sequence;
			return {true, &SaveGame};
		}

		static FVestigeSaveSlotRead MakeUnreadableSlot() { return {true, nullptr}; }

		static FVestigeSaveSlotRead MakeEmptySlot() { return {false, nullptr}; }

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
			const FVestigeSaveSlotRead Slots[] = {MakeEmptySlot(), MakeEmptySlot()};

			ASSERT_THAT(IsTrue(UVestigeSaveSubsystem::DecideLoad(Slots).Outcome == EVestigeLoadOutcome::NewGame));
		}

		TEST_METHOD(TheNewestReadableSaveIsLoaded)
		{
			const FVestigeSaveSlotRead Slots[] = {
				MakeSlot(UVestigeSaveGame::CurrentVersion, 4), MakeSlot(UVestigeSaveGame::CurrentVersion, 5)};

			const FVestigeLoadDecision Decision = UVestigeSaveSubsystem::DecideLoad(Slots);

			ASSERT_THAT(IsTrue(Decision.Outcome == EVestigeLoadOutcome::Load));
			ASSERT_THAT(IsTrue(Decision.Slot == 1));
		}

		TEST_METHOD(AnUnreadableSlotNextToAGoodOneLoadsTheGoodOne)
		{
			const FVestigeSaveSlotRead Slots[] = {MakeSlot(UVestigeSaveGame::CurrentVersion, 4), MakeUnreadableSlot()};

			const FVestigeLoadDecision Decision = UVestigeSaveSubsystem::DecideLoad(Slots);

			ASSERT_THAT(IsTrue(Decision.Outcome == EVestigeLoadOutcome::Load));
			ASSERT_THAT(IsTrue(Decision.Slot == 0));
		}

		TEST_METHOD(AnUnreadableSaveIsRefused)
		{
			const FVestigeSaveSlotRead Slots[] = {MakeUnreadableSlot(), MakeEmptySlot()};

			ASSERT_THAT(IsTrue(UVestigeSaveSubsystem::DecideLoad(Slots).Outcome == EVestigeLoadOutcome::RefuseUnreadable));
		}

		TEST_METHOD(ASaveFromANewerVersionIsRefusedEvenNextToAGoodOne)
		{
			const FVestigeSaveSlotRead Slots[] = {
				MakeSlot(UVestigeSaveGame::CurrentVersion, 9), MakeSlot(UVestigeSaveGame::CurrentVersion + 1, 1)};

			ASSERT_THAT(IsTrue(UVestigeSaveSubsystem::DecideLoad(Slots).Outcome == EVestigeLoadOutcome::RefuseNewer));
		}

		TEST_METHOD(ASaveFromAnOlderVersionIsRefused)
		{
			// Before the first public build there are no upgrades, so an older save can't be loaded.
			const FVestigeSaveSlotRead Slots[] = {MakeSlot(UVestigeSaveGame::CurrentVersion - 1, 3), MakeEmptySlot()};

			ASSERT_THAT(IsTrue(UVestigeSaveSubsystem::DecideLoad(Slots).Outcome == EVestigeLoadOutcome::RefuseUnreadable));
		}

		TEST_METHOD(AnOlderSaveNextToACurrentOneLoadsTheCurrentOne)
		{
			const FVestigeSaveSlotRead Slots[] = {
				MakeSlot(UVestigeSaveGame::CurrentVersion, 2), MakeSlot(UVestigeSaveGame::CurrentVersion - 1, 7)};

			const FVestigeLoadDecision Decision = UVestigeSaveSubsystem::DecideLoad(Slots);

			ASSERT_THAT(IsTrue(Decision.Outcome == EVestigeLoadOutcome::Load));
			ASSERT_THAT(IsTrue(Decision.Slot == 0));
		}

		TEST_METHOD(TheNextSaveNeverOverwritesTheNewestOne)
		{
			ASSERT_THAT(IsTrue(UVestigeSaveSubsystem::GetSlotToWrite(INDEX_NONE) == 0));
			ASSERT_THAT(IsTrue(UVestigeSaveSubsystem::GetSlotToWrite(0) == 1));
			ASSERT_THAT(IsTrue(UVestigeSaveSubsystem::GetSlotToWrite(1) == 0));
		}

		TEST_METHOD(ClockRoundTrips)
		{
			AVestigeDayCycle& Saved = Spawner.SpawnActor<AVestigeDayCycle>();
			Saved.StartNextDay();
			Saved.StartNextDay();
			AVestigeDayCycle& Restored = Spawner.SpawnActor<AVestigeDayCycle>();

			Restored.RestoreFromSaveData(Saved.GetSaveData());

			ASSERT_THAT(IsTrue(Restored.GetCurrentDay() == 3));
			ASSERT_THAT(IsTrue(Restored.GetTimeOfDay() == Saved.GetTimeOfDay()));
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
