// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "Algo/Reverse.h"
#include "Components/ActorTestSpawner.h"
#include "Content/FarmCropData.h"
#include "CQTest.h"
#include "Farming/FarmField.h"
#include "Farming/FarmFieldSaveData.h"
#include "Farming/FarmFieldSubsystem.h"
#include "Save/VestigeSaveGame.h"
#include "Save/VestigeSaveSubsystem.h"

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

		/** The test game instance's farm fields, as the game has them. */
		UFarmFieldSubsystem& GetFarmFields()
		{
			if (!UFarmFieldSubsystem::Get(&Spawner.GetWorld()))
			{
				Spawner.InitializeGameSubsystems();
			}
			return *UFarmFieldSubsystem::Get(&Spawner.GetWorld());
		}

		static inline const FIntPoint FieldSize = FIntPoint(8, 8);
		static inline const FName FarmId = TEXT("Base.Farm");
		static inline const FName TownId = TEXT("Base.Town");

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

		TEST_METHOD(FieldsRoundTripByIdAndCoordinatesWithTheirOwnCrop)
		{
			const UFarmCropData& Turnip = MakeCrop(TEXT("Base.Turnip"));
			const UFarmCropData& Carrot = MakeCrop(TEXT("Base.Carrot"));
			UFarmFieldSubsystem& FarmFields = GetFarmFields();
			UFarmField& Farm = FarmFields.FindOrAddField(FarmId, FieldSize);
			Farm.Till(FIntPoint(2, 3));
			Farm.Plant(FIntPoint(2, 3), &Carrot);
			Farm.Till(FIntPoint(5, 0));
			Farm.Water(FIntPoint(5, 0));
			FarmFields.FindOrAddField(TownId, FIntPoint(3, 2)).Till(FIntPoint(1, 1));

			// Reversed, so a field and a tile can only land in the right place by their ID and coordinates.
			TArray<FFarmFieldSaveData> SavedFields = FarmFields.GetSaveData();
			Algo::Reverse(SavedFields);
			for (FFarmFieldSaveData& Field : SavedFields)
			{
				Algo::Reverse(Field.Tiles);
			}
			FarmFields.StartNewGame();
			const int32 Skipped =
				FarmFields.RestoreFromSaveData(SavedFields, {{FName(TEXT("Base.Turnip")), &Turnip}, {FName(TEXT("Base.Carrot")), &Carrot}});

			ASSERT_THAT(IsTrue(Skipped == 0));
			const UFarmField* RestoredFarm = FarmFields.FindField(FarmId);
			const UFarmField* RestoredTown = FarmFields.FindField(TownId);
			ASSERT_THAT(IsNotNull(RestoredFarm));
			ASSERT_THAT(IsNotNull(RestoredTown));
			ASSERT_THAT(IsTrue(RestoredFarm->GetSize() == FieldSize));
			ASSERT_THAT(IsTrue(RestoredFarm->GetTile(FIntPoint(2, 3))->GetCrop() == &Carrot));
			ASSERT_THAT(IsTrue(RestoredFarm->GetTile(FIntPoint(5, 0))->IsWatered()));
			ASSERT_THAT(IsTrue(RestoredFarm->GetTile(FIntPoint(0, 0))->GetState() == EFarmTileState::Grass));
			ASSERT_THAT(IsTrue(RestoredTown->GetSize() == FIntPoint(3, 2)));
			ASSERT_THAT(IsTrue(RestoredTown->GetTile(FIntPoint(1, 1))->GetState() == EFarmTileState::Tilled));
		}

		TEST_METHOD(UnknownCropsAndTilesOutsideTheFieldAreSkipped)
		{
			UFarmFieldSubsystem& FarmFields = GetFarmFields();
			UFarmField& Farm = FarmFields.FindOrAddField(FarmId, FieldSize);
			FFarmFieldSaveData Saved;
			Saved.FieldId = FarmId;
			Saved.Size = FIntPoint(50, 50);
			FFarmTileSaveData& Planted = Saved.Tiles.AddDefaulted_GetRef();
			Planted.Coordinates = FIntPoint(1, 1);
			Planted.State = EFarmTileState::Planted;
			Planted.CropId = TEXT("SomeMod.Pumpkin");
			FFarmTileSaveData& Outside = Saved.Tiles.AddDefaulted_GetRef();
			Outside.Coordinates = FIntPoint(40, 2);
			Outside.State = EFarmTileState::Tilled;

			const int32 Skipped = FarmFields.RestoreFromSaveData({Saved}, {});

			// The field already exists, so it keeps the size its grid gave it.
			ASSERT_THAT(IsTrue(Skipped == 2));
			ASSERT_THAT(IsTrue(Farm.GetSize() == FieldSize));
			ASSERT_THAT(IsTrue(Farm.GetTile(FIntPoint(1, 1))->GetState() == EFarmTileState::Tilled));
			ASSERT_THAT(IsNull(Farm.GetTile(FIntPoint(1, 1))->GetCrop()));
		}
	};
}

#endif
