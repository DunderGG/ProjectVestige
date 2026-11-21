// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/ActorTestSpawner.h"
#include "Content/FarmCropData.h"
#include "CQTest.h"
#include "Farming/FarmTileState.h"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace FarmTileStateTests
{
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
			Ripe.Restore(EFarmTileState::Planted, 0, Crop.GetDaysToMature(), &Crop, 2);
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
			Ripe.Restore(EFarmTileState::Planted, 2, Crop.GetDaysToMature(), &Crop, 2);
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
			Over.Restore(EFarmTileState::Tilled, 9, 0, nullptr, 3);
			FFarmTileState Under;
			Under.Restore(EFarmTileState::Tilled, -4, 0, nullptr, 3);
			FFarmTileState Kept;
			Kept.Restore(EFarmTileState::Tilled, 2, 0, nullptr, 3);

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
			Planted.Restore(EFarmTileState::Planted, 0, Crop.GetDaysToMature() + 5, &Crop, 1);
			FFarmTileState Grass;
			Grass.Restore(EFarmTileState::Grass, 1, 3, nullptr, 1);

			ASSERT_THAT(IsTrue(Planted.GetGrowthDays() == Crop.GetDaysToMature()));
			ASSERT_THAT(IsFalse(Grass.IsWatered()));
			ASSERT_THAT(IsTrue(Grass.GetGrowthDays() == 0));
		}
	};
}

#endif
