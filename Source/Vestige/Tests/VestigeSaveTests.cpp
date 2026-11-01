// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "Algo/Reverse.h"
#include "Components/ActorTestSpawner.h"
#include "Content/FarmCropData.h"
#include "CQTest.h"
#include "Farming/FarmGrid.h"
#include "Farming/FarmTile.h"
#include "Kismet/GameplayStatics.h"
#include "Save/VestigeSaveGame.h"
#include "Save/VestigeSaveSubsystem.h"
#include "Serialization/MemoryWriter.h"
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

		/** Text as a save file writes it: names are written as strings, each with its length first. */
		static TArray<uint8> SerializeAsSaved(const FString& Text)
		{
			TArray<uint8> Bytes;
			FMemoryWriter Writer(Bytes);
			FString Copy = Text;
			Writer << Copy;
			return Bytes;
		}

		/** Replaces every name OldName in a written save with NewName, and returns how many it replaced. */
		static int32 ReplaceSavedName(TArray<uint8>& SaveBytes, const FString& OldName, const FString& NewName)
		{
			const TArray<uint8> OldBytes = SerializeAsSaved(OldName);
			const TArray<uint8> NewBytes = SerializeAsSaved(NewName);
			TArray<uint8> Result;
			int32 Replaced = 0;
			int32 Index = 0;
			while (Index < SaveBytes.Num())
			{
				const bool bMatches = Index + OldBytes.Num() <= SaveBytes.Num()
					&& FMemory::Memcmp(SaveBytes.GetData() + Index, OldBytes.GetData(), OldBytes.Num()) == 0;
				if (bMatches)
				{
					Result.Append(NewBytes);
					Index += OldBytes.Num();
					++Replaced;
				}
				else
				{
					Result.Add(SaveBytes[Index]);
					++Index;
				}
			}
			SaveBytes = MoveTemp(Result);
			return Replaced;
		}

		/** A save of day 6 at 13:15, written by this build. */
		TArray<uint8> WriteSaveOfDay6()
		{
			UVestigeSaveGame& Saved = Spawner.SpawnObject<UVestigeSaveGame>();
			Saved.SaveVersion = UVestigeSaveGame::CurrentVersion;
			Saved.Clock.Day = 6;
			Saved.Clock.TimeOfDay = 13.25f;
			TArray<uint8> Bytes;
			UGameplayStatics::SaveGameToMemory(&Saved, Bytes);
			return Bytes;
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

		TEST_METHOD(TheNextSaveNeverOverwritesTheNewestOne)
		{
			ASSERT_THAT(IsTrue(UVestigeSaveSubsystem::GetSlotToWrite(INDEX_NONE) == 0));
			ASSERT_THAT(IsTrue(UVestigeSaveSubsystem::GetSlotToWrite(0) == 1));
			ASSERT_THAT(IsTrue(UVestigeSaveSubsystem::GetSlotToWrite(1) == 0));
		}

		TEST_METHOD(ANewerVersionIsNotUpgraded)
		{
			UVestigeSaveGame& SaveGame = Spawner.SpawnObject<UVestigeSaveGame>();
			SaveGame.SaveVersion = UVestigeSaveGame::CurrentVersion + 1;

			ASSERT_THAT(IsFalse(UVestigeSaveSubsystem::UpgradeToCurrentVersion(SaveGame, 8)));
		}

		TEST_METHOD(Version0UpgradesToTheCurrentLayout)
		{
			UVestigeSaveGame& SaveGame = Spawner.SpawnObject<UVestigeSaveGame>();
			SaveGame.Day = 4;
			SaveGame.TimeOfDay = 9.5f;
			SaveGame.Seeds = 7;
			SaveGame.HarvestedCrops = 2;
			SaveGame.CurrentToolId = TEXT("Base.WateringCan");
			SaveGame.Tiles.SetNum(10);
			SaveGame.Tiles[9].State = EFarmTileState::Planted;

			ASSERT_THAT(IsTrue(UVestigeSaveSubsystem::UpgradeToCurrentVersion(SaveGame, 8)));

			ASSERT_THAT(IsTrue(SaveGame.SaveVersion == UVestigeSaveGame::CurrentVersion));
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

		TEST_METHOD(Version2UpgradesWithoutStamina)
		{
			UVestigeSaveGame& SaveGame = Spawner.SpawnObject<UVestigeSaveGame>();
			SaveGame.SaveVersion = 2;

			ASSERT_THAT(IsTrue(UVestigeSaveSubsystem::UpgradeToCurrentVersion(SaveGame, 8)));

			ASSERT_THAT(IsTrue(SaveGame.SaveVersion == UVestigeSaveGame::CurrentVersion));
			ASSERT_THAT(IsFalse(SaveGame.Stamina.bHasStamina));
		}

		TEST_METHOD(Version0ToolEnumMapsToTheBaseToolIds)
		{
			const TPair<EFarmTool, FName> Expected[] = {{EFarmTool::Hoe, TEXT("Base.Hoe")}, {EFarmTool::Seeds, TEXT("Base.Seeds")},
				{EFarmTool::WateringCan, TEXT("Base.WateringCan")}, {EFarmTool::Hand, TEXT("Base.Hand")}};
			for (const TPair<EFarmTool, FName>& Pair : Expected)
			{
				UVestigeSaveGame& SaveGame = Spawner.SpawnObject<UVestigeSaveGame>();
				SaveGame.CurrentTool = Pair.Key;

				ASSERT_THAT(IsTrue(UVestigeSaveSubsystem::UpgradeToCurrentVersion(SaveGame, 8)));
				ASSERT_THAT(IsTrue(SaveGame.Farming.CurrentToolId == Pair.Value));
			}
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

		TEST_METHOD(ASaveNamingTheOldFarmTypesLoads)
		{
			// Saves made before 2026-11-01 name the save game and clock types by their Farm names. UFarmSaveGame keeps the
			// class name loading, and [CoreRedirects] in DefaultEngine.ini maps the struct.
			TArray<uint8> Bytes = WriteSaveOfDay6();
			ASSERT_THAT(
				IsTrue(ReplaceSavedName(Bytes, UVestigeSaveGame::StaticClass()->GetPathName(), TEXT("/Script/Vestige.FarmSaveGame")) == 1));
			ASSERT_THAT(IsTrue(ReplaceSavedName(Bytes, TEXT("VestigeClockSaveData"), TEXT("FarmClockSaveData")) > 0));

			const UVestigeSaveGame* Loaded = Cast<UVestigeSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));

			ASSERT_THAT(IsNotNull(Loaded));
			ASSERT_THAT(IsTrue(Loaded->Clock.Day == 6));
			ASSERT_THAT(IsTrue(Loaded->Clock.TimeOfDay == 13.25f));
		}

		TEST_METHOD(ASaveNamingTheFarmSimModuleLoads)
		{
			// The module was called FarmSim before 2026-10-28, when the save game was FarmSim.FarmSaveGame.
			TArray<uint8> Bytes = WriteSaveOfDay6();
			ASSERT_THAT(
				IsTrue(ReplaceSavedName(Bytes, UVestigeSaveGame::StaticClass()->GetPathName(), TEXT("/Script/FarmSim.FarmSaveGame")) == 1));

			const UVestigeSaveGame* Loaded = Cast<UVestigeSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));

			ASSERT_THAT(IsNotNull(Loaded));
			ASSERT_THAT(IsTrue(Loaded->Clock.Day == 6));
		}

		TEST_METHOD(Version1SeedsAndHarvestBecomeTurnipItems)
		{
			UVestigeSaveGame& SaveGame = Spawner.SpawnObject<UVestigeSaveGame>();
			SaveGame.SaveVersion = 1;
			SaveGame.Farming.Seeds = 5;
			SaveGame.Farming.HarvestedCrops = 3;

			ASSERT_THAT(IsTrue(UVestigeSaveSubsystem::UpgradeToCurrentVersion(SaveGame, 8)));

			ASSERT_THAT(IsTrue(SaveGame.SaveVersion == UVestigeSaveGame::CurrentVersion));
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
			UVestigeSaveGame& SaveGame = Spawner.SpawnObject<UVestigeSaveGame>();
			SaveGame.SaveVersion = 1;
			SaveGame.Farming.HarvestedCrops = -2;

			ASSERT_THAT(IsTrue(UVestigeSaveSubsystem::UpgradeToCurrentVersion(SaveGame, 8)));
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
