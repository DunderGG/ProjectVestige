// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/ActorTestSpawner.h"
#include "Content/FarmCropData.h"
#include "Content/VestigeItemData.h"
#include "CQTest.h"
#include "Farming/FarmTileState.h"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace FarmTileStateTests
{
	/** The highest contamination level the restores below allow. */
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

	TEST_CLASS(TileState, "Vestige")
	{
		FActorTestSpawner Spawner;

		/** Tilled soil with a freshly planted crop. */
		FFarmTileState MakePlanted(const UFarmCropData& Crop)
		{
			FFarmTileState State;
			State.Till();
			State.Plant(&Crop);
			return State;
		}

		TEST_METHOD(OnlyGrassCanBeTilled)
		{
			FFarmTileState State;
			ASSERT_THAT(IsTrue(State.Till()));

			ASSERT_THAT(IsFalse(State.Till()));
			ASSERT_THAT(IsTrue(State.GetState() == EFarmTileState::Tilled));
		}

		TEST_METHOD(GrassCanNotBeWateredOrPlanted)
		{
			const UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();
			FFarmTileState State;

			ASSERT_THAT(IsFalse(State.AddWater(1, 1)));
			ASSERT_THAT(IsFalse(State.Plant(&Crop)));
			ASSERT_THAT(IsTrue(State.GetState() == EFarmTileState::Grass));
		}

		TEST_METHOD(APlantedTileCanNotBePlantedAgain)
		{
			const UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();
			FFarmTileState State = MakePlanted(Crop);

			ASSERT_THAT(IsFalse(State.Plant(&Crop)));
		}

		TEST_METHOD(WateredSoilCanNotBeWateredAgainAndDriesOvernight)
		{
			FFarmTileState State;
			State.Till();
			ASSERT_THAT(IsTrue(State.AddWater(1, 1)));

			ASSERT_THAT(IsFalse(State.AddWater(1, 1)));
			State.AdvanceDay(1);
			ASSERT_THAT(IsFalse(State.IsWatered()));
		}

		TEST_METHOD(AWateringAddsItsStepsUpToTheCap)
		{
			FFarmTileState State;
			State.Till();

			ASSERT_THAT(IsTrue(State.AddWater(2, 3)));
			ASSERT_THAT(IsTrue(State.GetWaterLevel() == 2));
			ASSERT_THAT(IsTrue(State.AddWater(2, 3)));
			ASSERT_THAT(IsTrue(State.GetWaterLevel() == 3));
		}

		TEST_METHOD(AWateringOfNoStepsOrLessIsRefusedAndChangesNothing)
		{
			FFarmTileState State;
			State.Till();

			ASSERT_THAT(IsFalse(State.AddWater(0, 2)));
			ASSERT_THAT(IsFalse(State.AddWater(-1, 2)));
			ASSERT_THAT(IsTrue(State.GetWaterLevel() == 0));
		}

		TEST_METHOD(ASoakedTileRefusesWaterAndChangesNothing)
		{
			FFarmTileState State;
			State.Till();
			State.AddWater(1, 2);
			State.AddWater(1, 2);

			ASSERT_THAT(IsFalse(State.CanAddWater(2)));
			ASSERT_THAT(IsFalse(State.AddWater(1, 2)));
			ASSERT_THAT(IsTrue(State.GetWaterLevel() == 2));
		}

		TEST_METHOD(EmptySoilAndARipeCropTakeWater)
		{
			UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();
			FFarmTileState Empty;
			Empty.Till();
			FFarmTileState Ripe;
			Ripe.Restore(SavedTile(EFarmTileState::Planted, 0, Crop.GetDaysToMature()), &Crop, 2, MaxContamination);
			ASSERT_THAT(IsTrue(Ripe.IsRipe()));

			ASSERT_THAT(IsTrue(Empty.AddWater(1, 2)));
			ASSERT_THAT(IsTrue(Ripe.AddWater(1, 2)));

			ASSERT_THAT(IsTrue(Empty.GetWaterLevel() == 1));
			ASSERT_THAT(IsTrue(Ripe.GetWaterLevel() == 1));
		}

		TEST_METHOD(TillingStartsDry)
		{
			FFarmTileState State;
			State.Till();

			ASSERT_THAT(IsTrue(State.GetWaterLevel() == 0));
			ASSERT_THAT(IsFalse(State.IsWatered()));
		}

		TEST_METHOD(PlantingAndHarvestingKeepTheWaterLevel)
		{
			UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();
			FFarmTileState State;
			State.Till();
			State.AddWater(2, 2);

			ASSERT_THAT(IsTrue(State.Plant(&Crop)));
			ASSERT_THAT(IsTrue(State.GetWaterLevel() == 2));

			FFarmTileState Ripe;
			Ripe.Restore(SavedTile(EFarmTileState::Planted, 2, Crop.GetDaysToMature()), &Crop, 2, MaxContamination);
			ASSERT_THAT(IsTrue(Ripe.Harvest()));
			ASSERT_THAT(IsTrue(Ripe.GetState() == EFarmTileState::Tilled));
			ASSERT_THAT(IsTrue(Ripe.GetWaterLevel() == 2));
		}

		TEST_METHOD(ACropWateredTwiceGrowsTwoNightsAndNotAThird)
		{
			UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();
			Crop.SetDaysToMatureForTests(5);
			FFarmTileState State = MakePlanted(Crop);
			State.AddWater(1, 2);
			State.AddWater(1, 2);

			State.AdvanceDay(1);
			ASSERT_THAT(IsTrue(State.GetGrowthDays() == 1));
			ASSERT_THAT(IsTrue(State.GetWaterLevel() == 1));

			State.AdvanceDay(1);
			ASSERT_THAT(IsTrue(State.GetGrowthDays() == 2));
			ASSERT_THAT(IsTrue(State.GetWaterLevel() == 0));

			State.AdvanceDay(1);
			ASSERT_THAT(IsTrue(State.GetGrowthDays() == 2));
		}

		TEST_METHOD(ADryCropDoesNotGrow)
		{
			UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();
			Crop.SetDaysToMatureForTests(5);
			FFarmTileState State = MakePlanted(Crop);

			State.AdvanceDay(1);

			ASSERT_THAT(IsTrue(State.GetGrowthDays() == 0));
			ASSERT_THAT(IsTrue(State.GetState() == EFarmTileState::Planted));
		}

		TEST_METHOD(TheNightDriesEmptySoilButNeverBelowZero)
		{
			FFarmTileState State;
			State.Till();
			State.AddWater(1, 2);

			State.AdvanceDay(1);
			ASSERT_THAT(IsTrue(State.GetWaterLevel() == 0));
			State.AdvanceDay(1);
			ASSERT_THAT(IsTrue(State.GetWaterLevel() == 0));
		}

		TEST_METHOD(ABiggerFallThanTheLevelStopsAtZeroAfterTheCropGrew)
		{
			UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();
			Crop.SetDaysToMatureForTests(5);
			FFarmTileState State = MakePlanted(Crop);
			State.AddWater(1, 3);

			State.AdvanceDay(3);

			ASSERT_THAT(IsTrue(State.GetWaterLevel() == 0));
			ASSERT_THAT(IsTrue(State.GetGrowthDays() == 1));
		}

		TEST_METHOD(TheFallTakesAsManyStepsAsItsNumber)
		{
			FFarmTileState State;
			State.Till();
			State.AddWater(3, 3);

			State.AdvanceDay(2);

			ASSERT_THAT(IsTrue(State.GetWaterLevel() == 1));
		}

		TEST_METHOD(RestoringClampsTheWaterLevelToTheCapAndToZero)
		{
			FFarmTileState Over;
			Over.Restore(SavedTile(EFarmTileState::Tilled, 9, 0), nullptr, 3, MaxContamination);
			FFarmTileState Under;
			Under.Restore(SavedTile(EFarmTileState::Tilled, -4, 0), nullptr, 3, MaxContamination);
			FFarmTileState Kept;
			Kept.Restore(SavedTile(EFarmTileState::Tilled, 2, 0), nullptr, 3, MaxContamination);

			ASSERT_THAT(IsTrue(Over.GetWaterLevel() == 3));
			ASSERT_THAT(IsTrue(Under.GetWaterLevel() == 0));
			ASSERT_THAT(IsTrue(Kept.GetWaterLevel() == 2));
		}

		TEST_METHOD(WaterRulesBelowOneAreLiftedToOne)
		{
			FFarmWaterRules Rules;
			Rules.MaxLevel = 0;
			Rules.StepsPerWatering = -2;
			Rules.NightlyFall = 0;

			const FFarmWaterRules Clamped = FFarmWaterRules::GetClamped(Rules);

			ASSERT_THAT(IsTrue(Clamped.MaxLevel == 1));
			ASSERT_THAT(IsTrue(Clamped.StepsPerWatering == 1));
			ASSERT_THAT(IsTrue(Clamped.NightlyFall == 1));
		}

		TEST_METHOD(WaterRulesOfOneOrMoreAreKept)
		{
			FFarmWaterRules Rules;
			Rules.MaxLevel = 4;
			Rules.StepsPerWatering = 2;
			Rules.NightlyFall = 3;

			const FFarmWaterRules Clamped = FFarmWaterRules::GetClamped(Rules);

			ASSERT_THAT(IsTrue(Clamped.MaxLevel == 4));
			ASSERT_THAT(IsTrue(Clamped.StepsPerWatering == 2));
			ASSERT_THAT(IsTrue(Clamped.NightlyFall == 3));
		}

		TEST_METHOD(TheDefaultWaterRulesAreTwoStepsOneAtATimeAndOneANight)
		{
			const FFarmWaterRules Rules;

			ASSERT_THAT(IsTrue(Rules.MaxLevel == 2));
			ASSERT_THAT(IsTrue(Rules.StepsPerWatering == 1));
			ASSERT_THAT(IsTrue(Rules.NightlyFall == 1));
		}

		TEST_METHOD(AnUnripeCropCanNotBeHarvested)
		{
			const UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();
			FFarmTileState State = MakePlanted(Crop);

			ASSERT_THAT(IsFalse(State.Harvest()));
			ASSERT_THAT(IsTrue(State.GetCrop() == &Crop));
		}

		TEST_METHOD(ARipeCropStopsGrowing)
		{
			const UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();
			FFarmTileState State = MakePlanted(Crop);
			for (int32 Day = 0; Day <= Crop.GetDaysToMature(); ++Day)
			{
				State.AddWater(1, 1);
				State.AdvanceDay(1);
			}

			ASSERT_THAT(IsTrue(State.IsRipe()));
			ASSERT_THAT(IsTrue(State.GetGrowthDays() == Crop.GetDaysToMature()));
		}

		TEST_METHOD(RestoringClampsGrowthAndKeepsGrassDry)
		{
			const UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();
			FFarmTileState Planted;
			Planted.Restore(SavedTile(EFarmTileState::Planted, 0, Crop.GetDaysToMature() + 5), &Crop, 1, MaxContamination);
			FFarmTileState Grass;
			Grass.Restore(SavedTile(EFarmTileState::Grass, 1, 3), nullptr, 1, MaxContamination);

			ASSERT_THAT(IsTrue(Planted.GetGrowthDays() == Crop.GetDaysToMature()));
			ASSERT_THAT(IsFalse(Grass.IsWatered()));
			ASSERT_THAT(IsTrue(Grass.GetGrowthDays() == 0));
		}

		/** Tilled soil with a crop that has grown for Days, watered to Level, and then withered. */
		FFarmTileState MakeWithered(const UFarmCropData& Crop, int32 Days, int32 Level)
		{
			FFarmTileState State;
			State.Restore(SavedTile(EFarmTileState::Planted, Level, Days), &Crop, 2, MaxContamination);
			State.Wither();
			return State;
		}

		TEST_METHOD(AnUnripeCropWithersKeepingItsCropGrowthAndWater)
		{
			const UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();
			FFarmTileState State;
			State.Restore(SavedTile(EFarmTileState::Planted, 2, 1), &Crop, 2, MaxContamination);
			ASSERT_THAT(IsTrue(State.CanWither()));

			ASSERT_THAT(IsTrue(State.Wither()));

			ASSERT_THAT(IsTrue(State.IsWithered()));
			ASSERT_THAT(IsTrue(State.GetState() == EFarmTileState::Withered));
			ASSERT_THAT(IsTrue(State.GetCrop() == &Crop));
			ASSERT_THAT(IsTrue(State.GetGrowthDays() == 1));
			ASSERT_THAT(IsTrue(State.GetWaterLevel() == 2));
			ASSERT_THAT(IsFalse(State.CanWither()));
		}

		TEST_METHOD(ARipeCropGrassAndEmptySoilDoNotWither)
		{
			const UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();
			FFarmTileState Ripe;
			Ripe.Restore(SavedTile(EFarmTileState::Planted, 0, Crop.GetDaysToMature()), &Crop, 2, MaxContamination);
			FFarmTileState Grass;
			FFarmTileState Empty;
			Empty.Till();

			ASSERT_THAT(IsFalse(Ripe.CanWither()));
			ASSERT_THAT(IsFalse(Ripe.Wither()));
			ASSERT_THAT(IsFalse(Grass.Wither()));
			ASSERT_THAT(IsFalse(Empty.Wither()));

			ASSERT_THAT(IsTrue(Ripe.GetState() == EFarmTileState::Planted));
			ASSERT_THAT(IsTrue(Grass.GetState() == EFarmTileState::Grass));
			ASSERT_THAT(IsTrue(Empty.GetState() == EFarmTileState::Tilled));
		}

		TEST_METHOD(AWitheredCropNeverGrowsButItsWaterStillFalls)
		{
			const UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();
			FFarmTileState State = MakeWithered(Crop, 1, 2);

			State.AdvanceDay(1);
			State.AddWater(1, 2);
			State.AdvanceDay(1);

			ASSERT_THAT(IsTrue(State.IsWithered()));
			ASSERT_THAT(IsTrue(State.GetGrowthDays() == 1));
			ASSERT_THAT(IsTrue(State.GetWaterLevel() == 1));
			ASSERT_THAT(IsFalse(State.IsRipe()));
		}

		TEST_METHOD(AWitheredTileTakesWaterAndRefusesTillingPlantingAndHarvesting)
		{
			const UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();
			FFarmTileState State = MakeWithered(Crop, 1, 0);

			ASSERT_THAT(IsTrue(State.AddWater(1, 2)));
			ASSERT_THAT(IsTrue(State.GetWaterLevel() == 1));
			ASSERT_THAT(IsFalse(State.CanTill()));
			ASSERT_THAT(IsFalse(State.Till()));
			ASSERT_THAT(IsFalse(State.CanPlant()));
			ASSERT_THAT(IsFalse(State.Plant(&Crop)));
			ASSERT_THAT(IsFalse(State.Harvest()));

			ASSERT_THAT(IsTrue(State.IsWithered()));
			ASSERT_THAT(IsTrue(State.GetCrop() == &Crop));
		}

		TEST_METHOD(ClearingAWitheredTileLeavesEmptyTilledSoilWithItsWater)
		{
			const UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();
			FFarmTileState State = MakeWithered(Crop, 1, 2);
			ASSERT_THAT(IsTrue(State.CanClearWithered()));

			ASSERT_THAT(IsTrue(State.ClearWithered()));

			ASSERT_THAT(IsTrue(State.GetState() == EFarmTileState::Tilled));
			ASSERT_THAT(IsNull(State.GetCrop()));
			ASSERT_THAT(IsTrue(State.GetGrowthDays() == 0));
			ASSERT_THAT(IsTrue(State.GetWaterLevel() == 2));
			ASSERT_THAT(IsTrue(State.CanPlant()));
		}

		TEST_METHOD(OnlyAWitheredTileCanBeCleared)
		{
			const UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();
			FFarmTileState Grass;
			FFarmTileState Empty;
			Empty.Till();
			FFarmTileState Planted = MakePlanted(Crop);

			ASSERT_THAT(IsFalse(Grass.ClearWithered()));
			ASSERT_THAT(IsFalse(Empty.ClearWithered()));
			ASSERT_THAT(IsFalse(Planted.ClearWithered()));

			ASSERT_THAT(IsTrue(Grass.GetState() == EFarmTileState::Grass));
			ASSERT_THAT(IsTrue(Empty.GetState() == EFarmTileState::Tilled));
			ASSERT_THAT(IsTrue(Planted.GetCrop() == &Crop));
		}

		TEST_METHOD(ARestoredWitheredTileKeepsItsCropGrowthAndWater)
		{
			const UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();
			FFarmTileState State;

			State.Restore(SavedTile(EFarmTileState::Withered, 1, 1), &Crop, 2, MaxContamination);

			ASSERT_THAT(IsTrue(State.IsWithered()));
			ASSERT_THAT(IsTrue(State.GetCrop() == &Crop));
			ASSERT_THAT(IsTrue(State.GetGrowthDays() == 1));
			ASSERT_THAT(IsTrue(State.GetWaterLevel() == 1));
		}

		TEST_METHOD(ARestoredWitheredTileWithoutItsCropIsEmptyTilledSoil)
		{
			FFarmTileState State;

			State.Restore(SavedTile(EFarmTileState::Withered, 1, 3), nullptr, 2, MaxContamination);

			ASSERT_THAT(IsTrue(State.GetState() == EFarmTileState::Tilled));
			ASSERT_THAT(IsNull(State.GetCrop()));
			ASSERT_THAT(IsTrue(State.GetGrowthDays() == 0));
			ASSERT_THAT(IsTrue(State.GetWaterLevel() == 1));
		}

		// Contamination (Systems/27-Contamination.md). A made-up crop keeps the C++ default response {1, 2, 0}.
		static inline const FPrimaryAssetId MutatedItemId = FPrimaryAssetId(UVestigeItemData::AssetType, TEXT("Test.MutatedCrop"));

		/** A crop of Days that can mutate into MutatedItemId. */
		UFarmCropData& MakeMutableCrop(int32 Days = 3)
		{
			UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();
			Crop.SetDaysToMatureForTests(Days);
			Crop.SetHarvestItemIdForTests(FPrimaryAssetId(UVestigeItemData::AssetType, TEXT("Test.Crop")));
			Crop.SetMutationForTests(MutatedItemId, TSoftObjectPtr<UStaticMesh>());
			return Crop;
		}

		/** A crop planted on soil at Level, freshly planted. */
		FFarmTileState MakePlantedOn(const UFarmCropData& Crop, int32 Level)
		{
			FFarmTileState State;
			State.SetContamination(Level, MaxContamination);
			State.Till();
			State.Plant(&Crop);
			return State;
		}

		/** A ripe crop at Level, restored as a save would, mutated if bMutated. */
		static FFarmTileState MakeRipeOn(const UFarmCropData& Crop, int32 Level, bool bMutated = false)
		{
			FFarmTileSaveData Saved = SavedTile(EFarmTileState::Planted, 0, Crop.GetDaysToMature());
			Saved.Contamination = Level;
			Saved.bMutated = bMutated;
			FFarmTileState State;
			State.Restore(Saved, &Crop, 2, MaxContamination);
			return State;
		}

		static void WaterAndSleep(FFarmTileState& State)
		{
			State.AddWater(1, 2);
			State.AdvanceDay(1);
		}

		TEST_METHOD(ACropOnCleanSoilGrowsEveryWateredNight)
		{
			UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();
			Crop.SetDaysToMatureForTests(5);
			FFarmTileState State = MakePlantedOn(Crop, 0);

			WaterAndSleep(State);
			WaterAndSleep(State);

			ASSERT_THAT(IsTrue(State.GetGrowthDays() == 2));
			ASSERT_THAT(IsTrue(State.GetWateredNights() == 0));
		}

		TEST_METHOD(ACropOnLightSoilGrowsEverySecondWateredNightAndDryNightsDontCount)
		{
			UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();
			Crop.SetDaysToMatureForTests(5);
			FFarmTileState State = MakePlantedOn(Crop, 1);

			WaterAndSleep(State);
			ASSERT_THAT(IsTrue(State.GetGrowthDays() == 0));
			ASSERT_THAT(IsTrue(State.GetWateredNights() == 1));

			State.AdvanceDay(1);
			State.AdvanceDay(1);
			ASSERT_THAT(IsTrue(State.GetGrowthDays() == 0));
			ASSERT_THAT(IsTrue(State.GetWateredNights() == 1));

			WaterAndSleep(State);
			ASSERT_THAT(IsTrue(State.GetGrowthDays() == 1));
			ASSERT_THAT(IsTrue(State.GetWateredNights() == 0));
		}

		TEST_METHOD(ACropOnHeavySoilNeverGrowsAndStaysPlanted)
		{
			const UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();
			FFarmTileState State = MakePlantedOn(Crop, 2);

			for (int32 Night = 0; Night < 10; ++Night)
			{
				WaterAndSleep(State);
			}

			ASSERT_THAT(IsTrue(State.GetGrowthDays() == 0));
			ASSERT_THAT(IsTrue(State.GetWateredNights() == 0));
			ASSERT_THAT(IsTrue(State.GetState() == EFarmTileState::Planted));
			ASSERT_THAT(IsTrue(State.CanWither()));
		}

		TEST_METHOD(TheLevelIsClampedAndASameLevelChangesNothing)
		{
			FFarmTileState State;

			ASSERT_THAT(IsTrue(State.SetContamination(9, MaxContamination)));
			ASSERT_THAT(IsTrue(State.GetContamination() == MaxContamination));
			ASSERT_THAT(IsFalse(State.SetContamination(MaxContamination, MaxContamination)));
			ASSERT_THAT(IsTrue(State.SetContamination(-3, MaxContamination)));
			ASSERT_THAT(IsTrue(State.GetContamination() == 0));
			ASSERT_THAT(IsFalse(State.IsContaminated()));
		}

		TEST_METHOD(EveryActionButACleaningHarvestKeepsTheLevel)
		{
			const UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();
			FFarmTileState State;
			State.SetContamination(1, MaxContamination);
			ASSERT_THAT(IsTrue(State.IsContaminated()));

			const auto ExpectLevel = [this, &State](const TCHAR* After)
			{
				ASSERT_THAT(IsTrue(State.GetContamination() == 1, After));
			};
			State.Till();
			ExpectLevel(TEXT("tilling"));
			State.Plant(&Crop);
			ExpectLevel(TEXT("planting"));
			State.AddWater(1, 2);
			ExpectLevel(TEXT("watering"));
			State.AdvanceDay(1);
			ExpectLevel(TEXT("a night"));
			ASSERT_THAT(IsTrue(State.Wither()));
			ExpectLevel(TEXT("withering"));
			ASSERT_THAT(IsTrue(State.ClearWithered()));
			ExpectLevel(TEXT("clearing"));

			FFarmTileState Ripe = MakeRipeOn(Crop, 1);
			ASSERT_THAT(IsTrue(Ripe.Harvest()));
			ASSERT_THAT(IsTrue(Ripe.GetContamination() == 1));
		}

		TEST_METHOD(ACleaningHarvestLowersTheLevelByItsAmountButNotBelowClean)
		{
			UFarmCropData& Cleaner = Spawner.SpawnObject<UFarmCropData>();
			Cleaner.SetContaminationResponseForTests({0, 1, 1}, 1, 1);
			UFarmCropData& StrongCleaner = Spawner.SpawnObject<UFarmCropData>();
			StrongCleaner.SetContaminationResponseForTests({0, 1, 1}, 1, 3);

			FFarmTileState Heavy = MakeRipeOn(Cleaner, 2);
			FFarmTileState Strong = MakeRipeOn(StrongCleaner, 2);
			ASSERT_THAT(IsTrue(Heavy.Harvest()));
			ASSERT_THAT(IsTrue(Strong.Harvest()));

			ASSERT_THAT(IsTrue(Heavy.GetContamination() == 1));
			ASSERT_THAT(IsTrue(Strong.GetContamination() == 0));
			ASSERT_THAT(IsTrue(Heavy.GetState() == EFarmTileState::Tilled));
		}

		TEST_METHOD(AdvanceDaySaysOnlyOnTheNightTheCropRipens)
		{
			UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();
			Crop.SetDaysToMatureForTests(2);
			FFarmTileState State = MakePlantedOn(Crop, 1);
			TArray<bool> Ripened;
			const auto Night = [&State, &Ripened](bool bWatered)
			{
				if (bWatered)
				{
					State.AddWater(1, 2);
				}
				Ripened.Add(State.AdvanceDay(1));
			};

			// Light soil: two watered nights per growth day, so the fourth watered night ripens it; a dry night in between.
			Night(true);
			Night(true);
			Night(false);
			Night(true);
			Night(true);
			Night(true);

			ASSERT_THAT(IsTrue(Ripened == TArray<bool>{false, false, false, false, true, false}));
			ASSERT_THAT(IsTrue(State.IsRipe()));
		}

		TEST_METHOD(OnlyARipeCropThatCanMutateMutates)
		{
			const UFarmCropData& Mutable = MakeMutableCrop();
			const UFarmCropData& Plain = Spawner.SpawnObject<UFarmCropData>();
			FFarmTileState Unripe = MakePlantedOn(Mutable, 1);
			FFarmTileState RipePlain = MakeRipeOn(Plain, 1);
			FFarmTileState Grass;
			FFarmTileState Ripe = MakeRipeOn(Mutable, 1);

			ASSERT_THAT(IsFalse(Unripe.Mutate()));
			ASSERT_THAT(IsFalse(RipePlain.Mutate()));
			ASSERT_THAT(IsFalse(Grass.Mutate()));
			ASSERT_THAT(IsTrue(Ripe.Mutate()));
			ASSERT_THAT(IsFalse(Ripe.Mutate()));

			ASSERT_THAT(IsFalse(Unripe.IsMutated()));
			ASSERT_THAT(IsFalse(RipePlain.IsMutated()));
			ASSERT_THAT(IsTrue(Ripe.IsMutated()));
			ASSERT_THAT(IsTrue(Ripe.IsRipe()));
		}

		TEST_METHOD(OnlyAMutatedCropGivesTheMutatedItem)
		{
			const UFarmCropData& Crop = MakeMutableCrop();
			FFarmTileState Ripe = MakeRipeOn(Crop, 1);
			ASSERT_THAT(IsTrue(Ripe.GetHarvestItemId() == Crop.GetHarvestItemId()));

			Ripe.Mutate();

			ASSERT_THAT(IsTrue(Ripe.GetHarvestItemId() == MutatedItemId));
			ASSERT_THAT(IsFalse(FFarmTileState().GetHarvestItemId().IsValid()));
		}

		TEST_METHOD(HarvestingWitheringClearingAndPlantingLeaveNoMutationOrWateredNights)
		{
			const UFarmCropData& Crop = MakeMutableCrop(5);
			FFarmTileState Ripe = MakeRipeOn(Crop, 1, true);
			ASSERT_THAT(IsTrue(Ripe.IsMutated()));

			ASSERT_THAT(IsTrue(Ripe.Harvest()));
			ASSERT_THAT(IsFalse(Ripe.IsMutated()));
			ASSERT_THAT(IsTrue(Ripe.Plant(&Crop)));
			ASSERT_THAT(IsFalse(Ripe.IsMutated()));
			ASSERT_THAT(IsTrue(Ripe.GetWateredNights() == 0));

			// Half a growth day on light soil, then withered and cleared.
			WaterAndSleep(Ripe);
			ASSERT_THAT(IsTrue(Ripe.GetWateredNights() == 1));
			ASSERT_THAT(IsTrue(Ripe.Wither()));
			ASSERT_THAT(IsTrue(Ripe.GetWateredNights() == 0));
			ASSERT_THAT(IsTrue(Ripe.ClearWithered()));
			ASSERT_THAT(IsTrue(Ripe.GetWateredNights() == 0));
			ASSERT_THAT(IsTrue(Ripe.Plant(&Crop)));
			ASSERT_THAT(IsTrue(Ripe.GetWateredNights() == 0));
		}

		TEST_METHOD(RestoringClampsTheLevelOnEveryTileGrassIncluded)
		{
			const UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();
			const auto RestoredLevel = [&Crop](EFarmTileState State, int32 Level)
			{
				FFarmTileSaveData Saved = SavedTile(State, 0, 0);
				Saved.Contamination = Level;
				FFarmTileState Tile;
				Tile.Restore(Saved, State == EFarmTileState::Planted ? &Crop : nullptr, 2, MaxContamination);
				return Tile.GetContamination();
			};

			ASSERT_THAT(IsTrue(RestoredLevel(EFarmTileState::Grass, 2) == 2));
			ASSERT_THAT(IsTrue(RestoredLevel(EFarmTileState::Grass, 9) == MaxContamination));
			ASSERT_THAT(IsTrue(RestoredLevel(EFarmTileState::Tilled, -1) == 0));
			ASSERT_THAT(IsTrue(RestoredLevel(EFarmTileState::Planted, 1) == 1));
		}

		TEST_METHOD(RestoringKeepsWateredNightsOnlyOnAGrowingCropAndBelowItsInterval)
		{
			UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();
			Crop.SetDaysToMatureForTests(5);
			const auto RestoredNights = [&Crop](EFarmTileState State, int32 Level, int32 GrowthDays, int32 Nights)
			{
				FFarmTileSaveData Saved = SavedTile(State, 1, GrowthDays);
				Saved.Contamination = Level;
				Saved.WateredNights = Nights;
				FFarmTileState Tile;
				Tile.Restore(Saved, State == EFarmTileState::Tilled ? nullptr : &Crop, 2, MaxContamination);
				return Tile.GetWateredNights();
			};

			// Light soil needs two nights, so one counted night is kept and more are cut to one.
			ASSERT_THAT(IsTrue(RestoredNights(EFarmTileState::Planted, 1, 1, 1) == 1));
			ASSERT_THAT(IsTrue(RestoredNights(EFarmTileState::Planted, 1, 1, 5) == 1));
			ASSERT_THAT(IsTrue(RestoredNights(EFarmTileState::Planted, 1, 1, -2) == 0));
			ASSERT_THAT(IsTrue(RestoredNights(EFarmTileState::Planted, 0, 1, 1) == 0));
			ASSERT_THAT(IsTrue(RestoredNights(EFarmTileState::Planted, 2, 1, 1) == 0));
			ASSERT_THAT(IsTrue(RestoredNights(EFarmTileState::Planted, 1, 5, 1) == 0));
			ASSERT_THAT(IsTrue(RestoredNights(EFarmTileState::Withered, 1, 1, 1) == 0));
			ASSERT_THAT(IsTrue(RestoredNights(EFarmTileState::Tilled, 1, 0, 1) == 0));
		}

		TEST_METHOD(RestoringKeepsTheMutationOnlyWhereItCanBe)
		{
			const UFarmCropData& Mutable = MakeMutableCrop();
			const UFarmCropData& Plain = Spawner.SpawnObject<UFarmCropData>();
			const auto RestoredMutation = [](EFarmTileState State, const UFarmCropData* Crop, int32 GrowthDays)
			{
				FFarmTileSaveData Saved = SavedTile(State, 0, GrowthDays);
				Saved.Contamination = 1;
				Saved.bMutated = true;
				FFarmTileState Tile;
				Tile.Restore(Saved, Crop, 2, MaxContamination);
				return Tile.IsMutated();
			};

			ASSERT_THAT(IsTrue(RestoredMutation(EFarmTileState::Planted, &Mutable, Mutable.GetDaysToMature())));
			ASSERT_THAT(IsFalse(RestoredMutation(EFarmTileState::Planted, &Mutable, 1)));
			ASSERT_THAT(IsFalse(RestoredMutation(EFarmTileState::Planted, &Plain, Plain.GetDaysToMature())));
			ASSERT_THAT(IsFalse(RestoredMutation(EFarmTileState::Withered, &Mutable, 1)));
			ASSERT_THAT(IsFalse(RestoredMutation(EFarmTileState::Planted, nullptr, Mutable.GetDaysToMature())));
		}
	};
}

#endif
