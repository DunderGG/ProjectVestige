// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/ActorTestSpawner.h"
#include "Content/FarmCropData.h"
#include "Content/VestigeCalendar.h"
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
#include "Time/VestigeClockSaveData.h"
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
			NewField.Initialize(TEXT("Base.Test"), Size, FFarmWaterRules(), FFarmContaminationRules());
			return NewField;
		}

		UFarmField& MakeFieldWithRules(const FFarmWaterRules& Rules)
		{
			UFarmField& NewField = Spawner.SpawnObject<UFarmField>();
			NewField.Initialize(TEXT("Base.Test"), FIntPoint(4, 3), Rules, FFarmContaminationRules());
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
				Field.AdvanceDay(1, 0);
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

			Field.AdvanceDay(1, 0);

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

			Field.AdvanceDay(1, 0);

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

			Field.AdvanceDay(1, 0);

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

		// Withering. The made-up calendar has two seasons of ten days on the real season tags.
		static constexpr const TCHAR* SpringTag = TEXT("Vestige.Season.Spring");
		static constexpr const TCHAR* SummerTag = TEXT("Vestige.Season.Summer");

		static FGameplayTag SeasonTag(const TCHAR* TagName) { return FGameplayTag::RequestGameplayTag(FName(TagName)); }

		static FVestigeSeason MakeSeason(const TCHAR* TagName, const TCHAR* Name)
		{
			FVestigeSeason Season;
			Season.Tag = SeasonTag(TagName);
			Season.DisplayName = FText::FromString(Name);
			Season.NameInSentence = FText::FromString(FString(Name).ToLower());
			return Season;
		}

		static FVestigeCalendar MakeCalendar()
		{
			return FVestigeCalendar::MakeForTests(10, {MakeSeason(SpringTag, TEXT("Alpha")), MakeSeason(SummerTag, TEXT("Beta"))});
		}

		/** A crop of three days that grows in the seasons with these tags (none listed: every season). */
		UFarmCropData& MakeSeasonCrop(std::initializer_list<const TCHAR*> SeasonTags)
		{
			UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();
			FGameplayTagContainer Listed;
			for (const TCHAR* Listing : SeasonTags)
			{
				Listed.AddTag(SeasonTag(Listing));
			}
			Crop.SetSeasonsForTests(Listed);
			return Crop;
		}

		void Plant(UFarmField& Field, const FIntPoint& Coordinates, const UFarmCropData& Crop)
		{
			Field.Till(Coordinates);
			Field.Plant(Coordinates, &Crop);
			Field.Water(Coordinates);
		}

		TEST_METHOD(OnlyCropsThatDontGrowInTheNewSeasonWither)
		{
			const UFarmCropData& SpringCrop = MakeSeasonCrop({SpringTag});
			const UFarmCropData& BothCrop = MakeSeasonCrop({SpringTag, SummerTag});
			const UFarmCropData& AnyCrop = MakeSeasonCrop({});
			UFarmField& Field = MakeField();
			Plant(Field, FIntPoint(0, 0), SpringCrop);
			Plant(Field, FIntPoint(1, 0), BothCrop);
			Plant(Field, FIntPoint(2, 0), AnyCrop);
			Plant(Field, FIntPoint(0, 1), SpringCrop);

			const int32 Count = Field.WitherCropsOutOfSeason(SeasonTag(SummerTag), MakeCalendar());

			ASSERT_THAT(IsTrue(Count == 2));
			ASSERT_THAT(IsTrue(Field.GetTile(FIntPoint(0, 0))->IsWithered()));
			ASSERT_THAT(IsTrue(Field.GetTile(FIntPoint(0, 1))->IsWithered()));
			ASSERT_THAT(IsTrue(Field.GetTile(FIntPoint(1, 0))->GetState() == EFarmTileState::Planted));
			ASSERT_THAT(IsTrue(Field.GetTile(FIntPoint(2, 0))->GetState() == EFarmTileState::Planted));
		}

		TEST_METHOD(AWitheredTileKeepsItsCropGrowthAndWater)
		{
			const UFarmCropData& Crop = MakeSeasonCrop({SpringTag});
			UFarmField& Field = MakeField();
			Plant(Field, Tile, Crop);
			Field.AdvanceDay(1, 0);
			Field.Water(Tile);

			Field.WitherCropsOutOfSeason(SeasonTag(SummerTag), MakeCalendar());

			ASSERT_THAT(IsTrue(Field.GetTile(Tile)->GetCrop() == &Crop));
			ASSERT_THAT(IsTrue(Field.GetTile(Tile)->GetGrowthDays() == 1));
			ASSERT_THAT(IsTrue(Field.GetTile(Tile)->GetWaterLevel() == 1));
		}

		TEST_METHOD(AFieldWithAnyWitheredCropSaysSoOnceAndNothingElseSaysNothing)
		{
			const UFarmCropData& SpringCrop = MakeSeasonCrop({SpringTag});
			UFarmField& Field = MakeField();
			Plant(Field, FIntPoint(0, 0), SpringCrop);
			Plant(Field, FIntPoint(1, 0), SpringCrop);
			int32 ManyBroadcasts = 0;
			int32 OneBroadcasts = 0;
			Field.OnTilesChanged.AddLambda(
				[&ManyBroadcasts]()
				{
					++ManyBroadcasts;
				});
			Field.OnTileChanged.AddLambda(
				[&OneBroadcasts](const FIntPoint&)
				{
					++OneBroadcasts;
				});

			ASSERT_THAT(IsTrue(Field.WitherCropsOutOfSeason(SeasonTag(SummerTag), MakeCalendar()) == 2));
			ASSERT_THAT(IsTrue(ManyBroadcasts == 1));
			ASSERT_THAT(IsTrue(Field.WitherCropsOutOfSeason(SeasonTag(SummerTag), MakeCalendar()) == 0));

			ASSERT_THAT(IsTrue(ManyBroadcasts == 1));
			ASSERT_THAT(IsTrue(OneBroadcasts == 0));
		}

		TEST_METHOD(RipeCropsDontWitherInAFieldEither)
		{
			const UFarmCropData& Crop = MakeSeasonCrop({SpringTag});
			UFarmField& Field = MakeField();
			Plant(Field, Tile, Crop);
			WaterForDays(Field, Crop.GetDaysToMature());
			ASSERT_THAT(IsTrue(Field.GetTile(Tile)->IsRipe()));

			ASSERT_THAT(IsTrue(Field.WitherCropsOutOfSeason(SeasonTag(SummerTag), MakeCalendar()) == 0));

			ASSERT_THAT(IsTrue(Field.GetTile(Tile)->IsRipe()));
		}

		TEST_METHOD(ClearingAWitheredTileBroadcastsOneTileChange)
		{
			const UFarmCropData& Crop = MakeSeasonCrop({SpringTag});
			UFarmField& Field = MakeField();
			Plant(Field, Tile, Crop);
			Field.WitherCropsOutOfSeason(SeasonTag(SummerTag), MakeCalendar());
			TArray<FIntPoint> Changed;
			Field.OnTileChanged.AddLambda(
				[&Changed](const FIntPoint& Coordinates)
				{
					Changed.Add(Coordinates);
				});

			ASSERT_THAT(IsTrue(Field.ClearWithered(Tile)));
			ASSERT_THAT(IsFalse(Field.ClearWithered(Tile)));
			ASSERT_THAT(IsFalse(Field.ClearWithered(FIntPoint(9, 9))));

			ASSERT_THAT(IsTrue(Changed == TArray<FIntPoint>{Tile}));
			ASSERT_THAT(IsTrue(Field.GetTile(Tile)->GetState() == EFarmTileState::Tilled));
			ASSERT_THAT(IsTrue(Field.GetTile(Tile)->GetWaterLevel() == 1));
		}

		// Contamination (Systems/27-Contamination.md).
		static FFarmContaminationRules MakeContaminationRules(int32 MaxLevel, TArray<float> Chances)
		{
			FFarmContaminationRules Rules;
			Rules.MaxLevel = MaxLevel;
			Rules.MutationChanceByLevel = MoveTemp(Chances);
			return Rules;
		}

		/** Tiles from Min to Max, both included, starting at Level. */
		static FFarmContaminationArea MakeArea(const FIntPoint& Min, const FIntPoint& Max, int32 Level)
		{
			FFarmContaminationArea Area;
			Area.Min = Min;
			Area.Max = Max;
			Area.Level = Level;
			return Area;
		}

		UFarmField& MakeFieldWithContaminationRules(const FFarmContaminationRules& Rules)
		{
			UFarmField& NewField = Spawner.SpawnObject<UFarmField>();
			NewField.Initialize(TEXT("Base.Test"), FIntPoint(4, 3), FFarmWaterRules(), Rules);
			return NewField;
		}

		/** A crop of Days that can mutate, with a content ID so saves can name it. */
		UFarmCropData& MakeMutableCrop(int32 Days, FName ContentId = TEXT("Test.Mutable"))
		{
			UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();
			Crop.SetContentIdForTests(ContentId);
			Crop.SetDaysToMatureForTests(Days);
			Crop.SetMutationForTests(FPrimaryAssetId(TEXT("VestigeItem"), TEXT("Test.Mutated")), TSoftObjectPtr<UStaticMesh>());
			return Crop;
		}

		/** Waters every planted tile of the field. */
		static void WaterAll(UFarmField& Field)
		{
			for (int32 Y = 0; Y < Field.GetSize().Y; ++Y)
			{
				for (int32 X = 0; X < Field.GetSize().X; ++X)
				{
					Field.Water(FIntPoint(X, Y));
				}
			}
		}

		TEST_METHOD(ACertainChanceMutatesOnlyACropThatRipenedTonightOnSoilWithAChance)
		{
			// Mutation is certain on light soil and impossible on clean soil.
			UFarmField& Field = MakeFieldWithContaminationRules(MakeContaminationRules(2, {0.f, 1.f, 0.f}));
			const UFarmCropData& Mutable = MakeMutableCrop(1);
			UFarmCropData& Plain = Spawner.SpawnObject<UFarmCropData>();
			Plain.SetDaysToMatureForTests(1);
			const FIntPoint RipensTonight(0, 0);
			const FIntPoint AlreadyRipe(1, 0);
			const FIntPoint CleanSoil(2, 0);
			const FIntPoint CantMutate(3, 0);
			Plant(Field, RipensTonight, Mutable);
			Plant(Field, AlreadyRipe, Mutable);
			Plant(Field, CleanSoil, Mutable);
			Plant(Field, CantMutate, Plain);
			for (const FIntPoint& Light : {RipensTonight, AlreadyRipe, CantMutate})
			{
				ASSERT_THAT(IsTrue(Field.SetContamination(Light, 1)));
			}
			// Ripe already: it ripened on clean soil, before its tile was contaminated.
			Field.SetContamination(AlreadyRipe, 0);
			Field.AdvanceDay(1, 0);
			ASSERT_THAT(IsTrue(Field.GetTile(AlreadyRipe)->IsRipe()));
			ASSERT_THAT(IsTrue(Field.GetTile(CleanSoil)->IsRipe()));
			Field.SetContamination(AlreadyRipe, 1);

			// The second watered night on light soil ripens the others.
			WaterAll(Field);
			Field.AdvanceDay(2, 0);

			ASSERT_THAT(IsTrue(Field.GetTile(RipensTonight)->IsRipe() && Field.GetTile(RipensTonight)->IsMutated()));
			ASSERT_THAT(IsFalse(Field.GetTile(AlreadyRipe)->IsMutated()));
			ASSERT_THAT(IsFalse(Field.GetTile(CleanSoil)->IsMutated()));
			ASSERT_THAT(IsTrue(Field.GetTile(CantMutate)->IsRipe()));
			ASSERT_THAT(IsFalse(Field.GetTile(CantMutate)->IsMutated()));

			// A ripe crop left in the ground gets no new chance.
			WaterAll(Field);
			Field.AdvanceDay(3, 0);
			ASSERT_THAT(IsFalse(Field.GetTile(AlreadyRipe)->IsMutated()));
		}

		TEST_METHOD(StartingAreasApplyWithLaterOnesOverEarlierClampedAndCut)
		{
			UFarmField& Field = MakeField();
			int32 ManyBroadcasts = 0;
			Field.OnTilesChanged.AddLambda(
				[&ManyBroadcasts]()
				{
					++ManyBroadcasts;
				});
			const TArray<FFarmContaminationArea> Areas = {MakeArea(FIntPoint(0, 0), FIntPoint(3, 2), 1),
				MakeArea(FIntPoint(2, 1), FIntPoint(9, 9), 2), MakeArea(FIntPoint(1, 0), FIntPoint(1, 0), 7)};

			Field.ApplyContaminationAreas(Areas);

			ASSERT_THAT(IsTrue(ManyBroadcasts == 1));
			for (int32 Y = 0; Y < 3; ++Y)
			{
				for (int32 X = 0; X < 4; ++X)
				{
					const bool bHeavy = (X >= 2 && Y >= 1) || (X == 1 && Y == 0);
					ASSERT_THAT(
						IsTrue(Field.GetTile(FIntPoint(X, Y))->GetContamination() == (bHeavy ? 2 : 1), *FIntPoint(X, Y).ToString()));
				}
			}
		}

		TEST_METHOD(AnAreaWithNoTileInTheFieldIsSkippedWithAWarning)
		{
			UFarmField& Field = MakeField();
			TestRunner->AddExpectedMessagePlain(
				TEXT("has no tile in field"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 2);

			const TArray<FFarmContaminationArea> Areas = {
				MakeArea(FIntPoint(5, 5), FIntPoint(6, 6), 2), MakeArea(FIntPoint(2, 2), FIntPoint(1, 1), 2)};

			Field.ApplyContaminationAreas(Areas);

			for (int32 Y = 0; Y < 3; ++Y)
			{
				for (int32 X = 0; X < 4; ++X)
				{
					ASSERT_THAT(IsFalse(Field.GetTile(FIntPoint(X, Y))->IsContaminated()));
				}
			}
		}

		TEST_METHOD(SettingALevelBroadcastsOneTileChangeOnlyWhenItChanges)
		{
			UFarmField& Field = MakeField();
			TArray<FIntPoint> Changed;
			Field.OnTileChanged.AddLambda(
				[&Changed](const FIntPoint& Coordinates)
				{
					Changed.Add(Coordinates);
				});

			ASSERT_THAT(IsTrue(Field.SetContamination(Tile, 5)));
			ASSERT_THAT(IsFalse(Field.SetContamination(Tile, 2)));
			ASSERT_THAT(IsFalse(Field.SetContamination(FIntPoint(9, 9), 1)));

			ASSERT_THAT(IsTrue(Changed == TArray<FIntPoint>{Tile}));
			ASSERT_THAT(IsTrue(Field.GetTile(Tile)->GetContamination() == Field.GetContaminationRules().MaxLevel));
		}

		TEST_METHOD(ResizingKeepsTheContaminationRulesAndLevels)
		{
			UFarmField& Field = MakeFieldWithContaminationRules(MakeContaminationRules(3, {0.f, 0.5f}));
			Field.SetContamination(Tile, 3);

			Field.Resize(FIntPoint(5, 4));

			ASSERT_THAT(IsTrue(Field.GetContaminationRules().MaxLevel == 3));
			ASSERT_THAT(IsTrue(Field.GetContaminationRules().GetMutationChance(1) == 0.5f));
			ASSERT_THAT(IsTrue(Field.GetTile(Tile)->GetContamination() == 3));
			ASSERT_THAT(IsTrue(Field.SetContamination(FIntPoint(4, 3), 3)));
		}

		TEST_METHOD(TheSaveDataKeepsContaminationWateredNightsAndTheMutation)
		{
			UFarmField& Field = MakeFieldWithContaminationRules(MakeContaminationRules(2, {0.f, 1.f, 0.f}));
			const UFarmCropData& Slow = MakeMutableCrop(5, TEXT("Test.Slow"));
			const UFarmCropData& Quick = MakeMutableCrop(1, TEXT("Test.Quick"));
			const FIntPoint Grass(0, 0);
			const FIntPoint Growing(1, 0);
			const FIntPoint Mutated(2, 0);
			Field.SetContamination(Grass, 2);
			Plant(Field, Growing, Slow);
			Plant(Field, Mutated, Quick);
			Field.SetContamination(Growing, 1);
			Field.SetContamination(Mutated, 1);
			Field.AdvanceDay(1, 0);
			WaterAll(Field);
			Field.AdvanceDay(2, 0);
			Field.Water(Growing);
			Field.AdvanceDay(3, 0);
			ASSERT_THAT(IsTrue(Field.GetTile(Growing)->GetWateredNights() == 1));
			ASSERT_THAT(IsTrue(Field.GetTile(Mutated)->IsMutated()));

			UFarmField& Other = MakeFieldWithContaminationRules(MakeContaminationRules(2, {0.f, 1.f, 0.f}));
			Other.RestoreTilesFromSaveData(Field.GetTilesSaveData(), {{TEXT("Test.Slow"), &Slow}, {TEXT("Test.Quick"), &Quick}});

			ASSERT_THAT(IsTrue(Other.GetTile(Grass)->GetContamination() == 2));
			ASSERT_THAT(IsTrue(Other.GetTile(Grass)->GetState() == EFarmTileState::Grass));
			ASSERT_THAT(IsTrue(Other.GetTile(Growing)->GetContamination() == 1));
			ASSERT_THAT(IsTrue(Other.GetTile(Growing)->GetWateredNights() == 1));
			ASSERT_THAT(IsTrue(Other.GetTile(Growing)->GetGrowthDays() == Field.GetTile(Growing)->GetGrowthDays()));
			ASSERT_THAT(IsTrue(Other.GetTile(Mutated)->IsMutated()));
			ASSERT_THAT(IsFalse(Other.GetTile(FIntPoint(3, 0))->IsContaminated()));
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
			ASSERT_THAT(IsNotNull(FindTurnip()));
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

			// From before contamination too: clean, no counted nights, no mutation, and the seed 0.
			ASSERT_THAT(IsFalse(Tile->IsContaminated()));
			ASSERT_THAT(IsTrue(Tile->GetWateredNights() == 0));
			ASSERT_THAT(IsFalse(Tile->IsMutated()));
			ASSERT_THAT(IsTrue(Fields->GetMutationSeed() == 0));
		}

		TEST_METHOD(ANewGameDropsEveryField)
		{
			Fields->FindOrAddField(FarmId, FIntPoint(8, 8)).Till(FIntPoint(1, 1));

			Fields->StartNewGame();

			ASSERT_THAT(IsNull(Fields->FindField(FarmId)));
			ASSERT_THAT(IsTrue(Fields->GetSaveData().IsEmpty()));
		}

		// Withering with the game's clock. Turnips grow in the calendar's first season only.
		int32 DaysPerSeason() const { return Clock->GetCalendar().GetDaysPerSeason(); }

		const UFarmCropData* FindTurnip()
		{
			return UVestigeContentCatalog::Get(&Spawner.GetWorld())
				->Find<UFarmCropData>(FPrimaryAssetId(UFarmCropData::AssetType, TEXT("Base.Turnip")));
		}

		/** The turnip, which BEFORE_EACH has checked is there. */
		const UFarmCropData& Turnip() { return *FindTurnip(); }

		/** Tills and plants a turnip at the start of a field's row and waters it. */
		void PlantTurnip(UFarmField& Field, const FIntPoint& Coordinates)
		{
			Field.Till(Coordinates);
			Field.Plant(Coordinates, &Turnip());
			Field.Water(Coordinates);
		}

		/** Ends the last day of the first season, so the second begins and OnSeasonStarted is broadcast. */
		void StartTheSecondSeason()
		{
			ASSERT_THAT(IsTrue(Clock->JumpToDay(DaysPerSeason())));
			Clock->StartNextDay();
		}

		TEST_METHOD(TheNextSeasonWithersEveryFieldsOutOfSeasonCropsWithoutTheirMaps)
		{
			UFarmField& Farm = Fields->FindOrAddField(FarmId, FIntPoint(8, 8));
			UFarmField& Other = Fields->FindOrAddField(TEXT("Base.Other"), FIntPoint(4, 4));
			PlantTurnip(Farm, FIntPoint(0, 0));
			PlantTurnip(Other, FIntPoint(1, 1));

			StartTheSecondSeason();

			ASSERT_THAT(IsTrue(Farm.GetTile(FIntPoint(0, 0))->IsWithered()));
			ASSERT_THAT(IsTrue(Other.GetTile(FIntPoint(1, 1))->IsWithered()));
		}

		TEST_METHOD(AWateredCropOneNightShortWithersRatherThanRipens)
		{
			UFarmField& Farm = Fields->FindOrAddField(FarmId, FIntPoint(8, 8));
			PlantTurnip(Farm, FIntPoint(0, 0));
			for (int32 Night = 1; Night < Turnip().GetDaysToMature(); ++Night)
			{
				Farm.AdvanceDay(1, 0);
				Farm.Water(FIntPoint(0, 0));
			}
			ASSERT_THAT(IsTrue(Farm.GetTile(FIntPoint(0, 0))->GetGrowthDays() == Turnip().GetDaysToMature() - 1));

			StartTheSecondSeason();

			ASSERT_THAT(IsTrue(Farm.GetTile(FIntPoint(0, 0))->IsWithered()));
			ASSERT_THAT(IsFalse(Farm.GetTile(FIntPoint(0, 0))->IsRipe()));
			ASSERT_THAT(IsTrue(Farm.GetTile(FIntPoint(0, 0))->GetGrowthDays() == Turnip().GetDaysToMature() - 1));
		}

		TEST_METHOD(ARipeCropIsStillThereWhenTheSeasonStarts)
		{
			UFarmField& Farm = Fields->FindOrAddField(FarmId, FIntPoint(8, 8));
			PlantTurnip(Farm, FIntPoint(0, 0));
			for (int32 Night = 0; Night < Turnip().GetDaysToMature(); ++Night)
			{
				Farm.AdvanceDay(1, 0);
				Farm.Water(FIntPoint(0, 0));
			}
			ASSERT_THAT(IsTrue(Farm.GetTile(FIntPoint(0, 0))->IsRipe()));

			StartTheSecondSeason();

			ASSERT_THAT(IsTrue(Farm.GetTile(FIntPoint(0, 0))->IsRipe()));
			ASSERT_THAT(IsFalse(Farm.GetTile(FIntPoint(0, 0))->IsWithered()));
		}

		TEST_METHOD(NothingWithersOnAUsualDay)
		{
			UFarmField& Farm = Fields->FindOrAddField(FarmId, FIntPoint(8, 8));
			PlantTurnip(Farm, FIntPoint(0, 0));

			Clock->StartNextDay();

			ASSERT_THAT(IsTrue(Farm.GetTile(FIntPoint(0, 0))->GetState() == EFarmTileState::Planted));
		}

		TEST_METHOD(NothingWithersWhenAClockIsRestoredOrJumpsIntoTheNextSeason)
		{
			UFarmField& Farm = Fields->FindOrAddField(FarmId, FIntPoint(8, 8));
			PlantTurnip(Farm, FIntPoint(0, 0));

			FVestigeClockSaveData Data;
			Data.Day = DaysPerSeason() + 1;
			Data.TimeOfDay = Clock->GetDayStartHour();
			Clock->RestoreFromSaveData(Data);
			ASSERT_THAT(IsTrue(Farm.GetTile(FIntPoint(0, 0))->GetState() == EFarmTileState::Planted));

			ASSERT_THAT(IsTrue(Clock->JumpToDay(2 * DaysPerSeason() + 1)));
			ASSERT_THAT(IsTrue(Farm.GetTile(FIntPoint(0, 0))->GetState() == EFarmTileState::Planted));
		}

		TEST_METHOD(NothingWithersOnANewGame)
		{
			UFarmField& Farm = Fields->FindOrAddField(FarmId, FIntPoint(8, 8));
			PlantTurnip(Farm, FIntPoint(0, 0));
			Clock->JumpToDay(DaysPerSeason() + 1);

			Clock->StartNewGame();

			ASSERT_THAT(IsTrue(Farm.GetTile(FIntPoint(0, 0))->GetState() == EFarmTileState::Planted));
		}

		TEST_METHOD(TheSaveKeepsAWitheredTile)
		{
			UFarmField& Farm = Fields->FindOrAddField(FarmId, FIntPoint(8, 8));
			PlantTurnip(Farm, FIntPoint(2, 3));
			Farm.AdvanceDay(1, 0);
			Farm.Water(FIntPoint(2, 3));
			Farm.WitherCropsOutOfSeason(Clock->GetCalendar().GetSeasons()[1].Tag, Clock->GetCalendar());
			const FVestigeSaveRecord Record = Fields->WriteSaveRecord();
			Fields->StartNewGame();
			int32 LeftOut = 0;

			ASSERT_THAT(IsTrue(Fields->ReadSaveRecord(Record, LeftOut)));

			const UFarmField* Restored = Fields->FindField(FarmId);
			ASSERT_THAT(IsNotNull(Restored));
			const FFarmTileState* Tile = Restored->GetTile(FIntPoint(2, 3));
			ASSERT_THAT(IsTrue(Tile->IsWithered()));
			ASSERT_THAT(IsTrue(Tile->GetCrop() == &Turnip()));
			ASSERT_THAT(IsTrue(Tile->GetGrowthDays() == 1));
			ASSERT_THAT(IsTrue(Tile->GetWaterLevel() == 1));
			ASSERT_THAT(IsTrue(LeftOut == 0));
		}

		// Contamination (Systems/27-Contamination.md).
		static FFarmContaminationArea MakeArea(const FIntPoint& Min, const FIntPoint& Max, int32 Level)
		{
			FFarmContaminationArea Area;
			Area.Min = Min;
			Area.Max = Max;
			Area.Level = Level;
			return Area;
		}

		/** The far two columns of an 8 x 8 field heavily contaminated. */
		static TArray<FFarmContaminationArea> HeavyFarEdge() { return {MakeArea(FIntPoint(6, 0), FIntPoint(7, 7), 2)}; }

		TEST_METHOD(AFieldTheSubsystemMakesFollowsTheContaminationSettingsAndStartsWithItsAreas)
		{
			const FFarmContaminationRules Expected = UFarmFieldSettings::Get().GetContaminationRules();

			const UFarmField& Field = Fields->FindOrAddField(FarmId, FIntPoint(8, 8), HeavyFarEdge());

			ASSERT_THAT(IsTrue(Field.GetContaminationRules().MaxLevel == Expected.MaxLevel));
			ASSERT_THAT(IsTrue(Field.GetContaminationRules().MutationChanceByLevel == Expected.MutationChanceByLevel));
			ASSERT_THAT(IsTrue(Field.GetTile(FIntPoint(6, 3))->GetContamination() == FMath::Min(2, Expected.MaxLevel)));
			ASSERT_THAT(IsFalse(Field.GetTile(FIntPoint(5, 3))->IsContaminated()));
		}

		TEST_METHOD(AFieldAskedForAgainKeepsItsCleanedTiles)
		{
			UFarmField& Field = Fields->FindOrAddField(FarmId, FIntPoint(8, 8), HeavyFarEdge());
			ASSERT_THAT(IsTrue(Field.SetContamination(FIntPoint(6, 3), 0)));

			const UFarmField& Again = Fields->FindOrAddField(FarmId, FIntPoint(8, 8), HeavyFarEdge());

			ASSERT_THAT(IsTrue(&Again == &Field));
			ASSERT_THAT(IsFalse(Again.GetTile(FIntPoint(6, 3))->IsContaminated()));
			ASSERT_THAT(IsTrue(Again.GetTile(FIntPoint(7, 3))->IsContaminated()));
		}

		TEST_METHOD(AFieldFromASaveKeepsItsTilesWhateverTheAreas)
		{
			FFarmFieldsSaveData Data;
			Data.Fields.Add({FarmId, FIntPoint(8, 8), {}});
			int32 LeftOut = 0;
			ASSERT_THAT(IsTrue(Fields->ReadSaveRecord(FVestigeSaveRecord::Write(Data), LeftOut)));

			const UFarmField& Field = Fields->FindOrAddField(FarmId, FIntPoint(8, 8), HeavyFarEdge());

			ASSERT_THAT(IsFalse(Field.GetTile(FIntPoint(6, 3))->IsContaminated()));
		}

		TEST_METHOD(TheSaveKeepsEachTilesContaminationAndTheMutationSeed)
		{
			UFarmField& Field = Fields->FindOrAddField(FarmId, FIntPoint(8, 8));
			Field.SetContamination(FIntPoint(1, 1), 1);
			Field.SetContamination(FIntPoint(2, 1), 2);
			Fields->SetMutationSeedForTests(4242);
			const FVestigeSaveRecord Record = Fields->WriteSaveRecord();
			Fields->StartNewGame();
			int32 LeftOut = 0;

			ASSERT_THAT(IsTrue(Fields->ReadSaveRecord(Record, LeftOut)));

			const UFarmField* Restored = Fields->FindField(FarmId);
			ASSERT_THAT(IsNotNull(Restored));
			ASSERT_THAT(IsTrue(Restored->GetTile(FIntPoint(1, 1))->GetContamination() == 1));
			ASSERT_THAT(IsTrue(Restored->GetTile(FIntPoint(2, 1))->GetContamination() == 2));
			ASSERT_THAT(IsFalse(Restored->GetTile(FIntPoint(3, 1))->IsContaminated()));
			ASSERT_THAT(IsTrue(Fields->GetMutationSeed() == 4242));
		}

		TEST_METHOD(ANewGamePicksANewMutationSeed)
		{
			Fields->SetMutationSeedForTests(4242);

			Fields->StartNewGame();
			const int32 First = Fields->GetMutationSeed();
			Fields->StartNewGame();

			ASSERT_THAT(IsTrue(First != 4242));
			ASSERT_THAT(IsTrue(Fields->GetMutationSeed() != First));
		}

		TEST_METHOD(TheNightRollsWithTheGamesSeedAndTheNewDay)
		{
			const FFarmContaminationRules Rules = UFarmFieldSettings::Get().GetContaminationRules();
			const float Chance = Rules.GetMutationChance(1);
			UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();
			Crop.SetDaysToMatureForTests(1);
			Crop.SetMutationForTests(FPrimaryAssetId(TEXT("VestigeItem"), TEXT("Test.Mutated")), TSoftObjectPtr<UStaticMesh>());
			UFarmField& Field = Fields->FindOrAddField(FarmId, FIntPoint(8, 8));
			Fields->SetMutationSeedForTests(31337);
			for (int32 Index = 0; Index < 64; ++Index)
			{
				const FIntPoint Coordinates(Index % 8, Index / 8);
				Field.Till(Coordinates);
				Field.Plant(Coordinates, &Crop);
				Field.Water(Coordinates);
				Field.SetContamination(Coordinates, 1);
			}

			// Light soil: the second watered night ripens every crop.
			Clock->StartNextDay();
			for (int32 Index = 0; Index < 64; ++Index)
			{
				Field.Water(FIntPoint(Index % 8, Index / 8));
			}
			Clock->StartNextDay();

			const int32 Day = Clock->GetCurrentDay();
			int32 MutatedCount = 0;
			for (int32 Index = 0; Index < 64; ++Index)
			{
				const FIntPoint Coordinates(Index % 8, Index / 8);
				const FFarmTileState* Tile = Field.GetTile(Coordinates);
				ASSERT_THAT(IsTrue(Tile->IsRipe()));
				const bool bExpected = FarmContamination::RollsMutation(31337, FarmId, Coordinates, Day, Chance);
				ASSERT_THAT(IsTrue(Tile->IsMutated() == bExpected, *Coordinates.ToString()));
				MutatedCount += Tile->IsMutated();
			}
			// At the settings' chance some crops of 64 ripen mutated and some don't, unless the chance is 0 or 1.
			ASSERT_THAT(IsTrue(Chance <= 0.f || Chance >= 1.f || (MutatedCount > 0 && MutatedCount < 64)));
		}

		// ---- The night's work is done ----

		TEST_METHOD(TheFieldsTellTheyAdvancedAfterEveryFieldsNight)
		{
			UFarmField& Farm = Fields->FindOrAddField(FarmId, FIntPoint(8, 8));
			UFarmField& Other = Fields->FindOrAddField(TEXT("Base.Other"), FIntPoint(4, 4));
			PlantTurnip(Farm, FIntPoint(0, 0));
			PlantTurnip(Other, FIntPoint(1, 1));
			TArray<int32> Days;
			TArray<int32> GrowthAtTheEvent;
			Fields->OnFieldsAdvanced.AddLambda(
				[&](int32 Day)
				{
					Days.Add(Day);
					GrowthAtTheEvent.Add(Farm.GetTile(FIntPoint(0, 0))->GetGrowthDays());
					GrowthAtTheEvent.Add(Other.GetTile(FIntPoint(1, 1))->GetGrowthDays());
				});

			Clock->StartNextDay();

			// Once, for the new day, when both fields had grown and their water had fallen.
			ASSERT_THAT(IsTrue(Days == TArray<int32>{Clock->GetCurrentDay()}));
			ASSERT_THAT(IsTrue(GrowthAtTheEvent == TArray<int32>{1, 1}));
		}

		TEST_METHOD(TheFieldsTellNothingWhenNoNightPasses)
		{
			Fields->FindOrAddField(FarmId, FIntPoint(8, 8));
			int32 Events = 0;
			Fields->OnFieldsAdvanced.AddLambda(
				[&Events](int32)
				{
					++Events;
				});

			ASSERT_THAT(IsTrue(Clock->JumpToDay(5)));

			ASSERT_THAT(IsTrue(Events == 0));
		}
	};
}

#endif
