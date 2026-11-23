// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/ActorTestSpawner.h"
#include "Content/FarmCropData.h"
#include "Content/VestigeCalendar.h"
#include "Content/VestigeContentCatalog.h"
#include "Content/VestigeGameData.h"
#include "Content/VestigeItemData.h"
#include "CQTest.h"
#include "Engine/AssetManager.h"
#include "Engine/StaticMesh.h"
#include "GameplayTagContainer.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "Misc/DataValidation.h"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace FarmCropDataTests
{
	// Docs/GameDesign.md, section 6 (crops): a seedling is at least about 20 cm across on the day it is planted, and no look is
	// wider than about 60 cm. Sizes here are in centimetres, the engine's unit, as drawn (mesh bounds times the look's scale).
	constexpr float MinimumSeedlingSpread = 20.f;
	constexpr float MaximumSpread = 60.f;
	constexpr float Tolerance = 0.01f;

	/** How a crop looks on each growth day: which stage mesh, at what scale, and the ripe mesh only when ripe. */
	TEST_CLASS(FarmCrop, "Vestige")
	{
		FActorTestSpawner Spawner;
		TArray<TObjectPtr<UStaticMesh>> Stages;
		TObjectPtr<UStaticMesh> Ripe;

		static TSoftObjectPtr<UStaticMesh> SoftOne(UStaticMesh* Mesh) { return TSoftObjectPtr<UStaticMesh>(Mesh); }

		static FFarmCropGrowthStage MakeStage(int32 StartDay, UStaticMesh* Mesh)
		{
			FFarmCropGrowthStage Stage;
			Stage.StartDay = StartDay;
			Stage.Mesh = SoftOne(Mesh);
			return Stage;
		}

		/** A crop that takes Days to mature, with a growth stage starting on each of StartDays, and a ripe mesh. */
		UFarmCropData& MakeCrop(int32 Days, std::initializer_list<int32> StartDays)
		{
			UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();
			Stages.Reset();
			TArray<FFarmCropGrowthStage> GrowthStages;
			for (const int32 StartDay : StartDays)
			{
				UStaticMesh& Stage = Spawner.SpawnObject<UStaticMesh>();
				Stages.Add(&Stage);
				GrowthStages.Add(MakeStage(StartDay, &Stage));
			}
			Ripe = &Spawner.SpawnObject<UStaticMesh>();
			Crop.SetDaysToMatureForTests(Days);
			Crop.SetGrowthStagesForTests(GrowthStages, SoftOne(Ripe.Get()));
			return Crop;
		}

		/** The stage indexes a crop shows on days 0 to Days - 1, as a string like "0,0,1". */
		static FString StageSequence(const UFarmCropData& Crop)
		{
			TArray<FString> Parts;
			for (int32 Day = 0; Day < Crop.GetDaysToMature(); ++Day)
			{
				Parts.Add(FString::FromInt(Crop.GetGrowthStageIndex(Day)));
			}
			return FString::Join(Parts, TEXT(","));
		}

		static bool IsScale(const UFarmCropData& Crop, int32 Day, float Scale)
		{
			return FMath::IsNearlyEqual(Crop.GetGrowthLook(Day).Scale, Scale);
		}

		TEST_METHOD(StartDaysOneApartShowOneStagePerDay)
		{
			const UFarmCropData& Crop = MakeCrop(3, {0, 1, 2});

			ASSERT_THAT(IsTrue(StageSequence(Crop) == TEXT("0,1,2")));
			for (int32 Day = 0; Day < 3; ++Day)
			{
				ASSERT_THAT(IsTrue(Crop.GetGrowthLook(Day).Mesh == Stages[Day].Get()));
			}
		}

		TEST_METHOD(EachStageShowsFromItsStartDayUntilTheNext)
		{
			ASSERT_THAT(IsTrue(StageSequence(MakeCrop(10, {0, 1, 3, 5, 8})) == TEXT("0,1,1,2,2,3,3,3,4,4")));
			ASSERT_THAT(IsTrue(StageSequence(MakeCrop(6, {0, 1, 3})) == TEXT("0,1,1,2,2,2")));
		}

		TEST_METHOD(AStageStartingAtOrAfterRipeNeverShows)
		{
			// The cabbage's start days on a 7-day crop: its last stage would start on day 8, after it ripens.
			const UFarmCropData& Crop = MakeCrop(7, {0, 1, 3, 5, 8});

			ASSERT_THAT(IsTrue(StageSequence(Crop) == TEXT("0,1,1,2,2,3,3")));
			ASSERT_THAT(IsTrue(IsScale(Crop, 6, 1.f)));
			for (int32 Day = 0; Day <= 9; ++Day)
			{
				ASSERT_THAT(IsTrue(Crop.GetGrowthLook(Day).Mesh != Stages[4].Get()));
			}
		}

		TEST_METHOD(OnlyARipeCropShowsTheRipeMesh)
		{
			const UFarmCropData& Crop = MakeCrop(4, {0, 2});

			for (int32 Day = 0; Day < 4; ++Day)
			{
				ASSERT_THAT(IsFalse(Crop.IsRipeAt(Day)));
				ASSERT_THAT(IsTrue(Crop.GetGrowthLook(Day).Mesh != Ripe.Get()));
			}
			for (int32 Day : {4, 5, 7})
			{
				ASSERT_THAT(IsTrue(Crop.IsRipeAt(Day)));
				ASSERT_THAT(IsTrue(Crop.GetGrowthStageIndex(Day) == INDEX_NONE));
				ASSERT_THAT(IsTrue(Crop.GetGrowthLook(Day).Mesh == Ripe.Get()));
				ASSERT_THAT(IsTrue(IsScale(Crop, Day, 1.f)));
			}
		}

		TEST_METHOD(AMissingMeshGivesNoMesh)
		{
			UFarmCropData& Crop = MakeCrop(4, {0, 2});

			Crop.SetGrowthStagesForTests({MakeStage(0, Stages[0].Get()), MakeStage(2, nullptr)}, SoftOne(Ripe.Get()));
			ASSERT_THAT(IsTrue(Crop.GetGrowthLook(0).Mesh == Stages[0].Get()));
			ASSERT_THAT(IsNull(Crop.GetGrowthLook(2).Mesh));

			Crop.SetGrowthStagesForTests({}, SoftOne(Ripe.Get()));
			ASSERT_THAT(IsNull(Crop.GetGrowthLook(1).Mesh));
			ASSERT_THAT(IsTrue(Crop.GetGrowthStageIndex(1) == INDEX_NONE));
			ASSERT_THAT(IsTrue(IsScale(Crop, 1, 1.f)));

			Crop.SetGrowthStagesForTests({MakeStage(0, Stages[0].Get()), MakeStage(2, Stages[1].Get())}, SoftOne(nullptr));
			ASSERT_THAT(IsNull(Crop.GetGrowthLook(4).Mesh));
			ASSERT_THAT(IsTrue(Crop.GetGrowthLook(0).Mesh == Stages[0].Get()));
		}

		TEST_METHOD(NegativeDaysShowTheFirstStage)
		{
			const UFarmCropData& Crop = MakeCrop(10, {0, 1, 3, 5});

			ASSERT_THAT(IsTrue(Crop.GetGrowthStageIndex(-3) == 0));
			ASSERT_THAT(IsTrue(Crop.GetGrowthLook(-3).Mesh == Stages[0].Get()));
		}

		TEST_METHOD(AThreeDayStageGrowsFromItsStartScaleToFull)
		{
			// The cabbage's fourth stage, days 5 to 7.
			const UFarmCropData& Crop = MakeCrop(10, {0, 1, 3, 5, 8});

			ASSERT_THAT(IsTrue(IsScale(Crop, 5, 0.8f)));
			ASSERT_THAT(IsTrue(IsScale(Crop, 6, 0.9f)));
			ASSERT_THAT(IsTrue(IsScale(Crop, 7, 1.f)));
		}

		TEST_METHOD(ATwoDayStageShowsItsStartScaleThenFull)
		{
			// The cabbage's second stage, days 1 and 2.
			const UFarmCropData& Crop = MakeCrop(10, {0, 1, 3, 5, 8});

			ASSERT_THAT(IsTrue(Crop.GetGrowthStageIndex(1) == 1 && Crop.GetGrowthStageIndex(2) == 1));
			ASSERT_THAT(IsTrue(IsScale(Crop, 1, 0.8f)));
			ASSERT_THAT(IsTrue(IsScale(Crop, 2, 1.f)));
		}

		TEST_METHOD(OneDayStagesAndTheRipeCropShowFullSize)
		{
			const UFarmCropData& Turnip = MakeCrop(3, {0, 1, 2});
			for (int32 Day = 0; Day <= 3; ++Day)
			{
				ASSERT_THAT(IsTrue(IsScale(Turnip, Day, 1.f)));
			}

			// The cabbage's first stage is a one-day stage too.
			const UFarmCropData& Cabbage = MakeCrop(10, {0, 1, 3, 5, 8});
			ASSERT_THAT(IsTrue(IsScale(Cabbage, 0, 1.f)));
			ASSERT_THAT(IsTrue(IsScale(Cabbage, 10, 1.f)));
		}

		TEST_METHOD(EveryStageReachesFullSizeOnItsLastDay)
		{
			struct FCase
			{
				int32 Days;
				TArray<int32> StartDays;
			};
			const TArray<FCase> Cases = {{3, {0, 1, 2}}, {6, {0, 1, 3}}, {10, {0, 1, 3, 5, 8}}, {10, {0}}, {7, {0, 4}}, {12, {0, 2, 3, 9}}};
			for (const FCase& Case : Cases)
			{
				UFarmCropData& Crop = MakeCrop(Case.Days, {});
				TArray<FFarmCropGrowthStage> GrowthStages;
				for (const int32 StartDay : Case.StartDays)
				{
					UStaticMesh& Mesh = Spawner.SpawnObject<UStaticMesh>();
					GrowthStages.Add(MakeStage(StartDay, &Mesh));
				}
				Crop.SetGrowthStagesForTests(GrowthStages, SoftOne(Ripe.Get()));

				for (int32 Stage = 0; Stage < Case.StartDays.Num(); ++Stage)
				{
					const int32 First = Case.StartDays[Stage];
					const int32 End = Stage + 1 < Case.StartDays.Num() ? Case.StartDays[Stage + 1] : Case.Days;
					const int32 Last = End - 1;
					const FString Name = FString::Printf(TEXT("D%d stage %d (days %d to %d)"), Case.Days, Stage, First, Last);
					ASSERT_THAT(IsTrue(Crop.GetGrowthStageIndex(First) == Stage, *Name));
					ASSERT_THAT(IsTrue(Crop.GetGrowthStageIndex(Last) == Stage, *Name));
					ASSERT_THAT(IsTrue(IsScale(Crop, Last, 1.f), *Name));
					if (Last > First)
					{
						ASSERT_THAT(IsTrue(IsScale(Crop, First, 0.8f), *Name));
					}
				}
			}
		}

		TEST_METHOD(TheTurnipShowsItsThreeStagesAndThenRipe)
		{
			Spawner.InitializeGameSubsystems();
			const FPrimaryAssetId TurnipId(UFarmCropData::AssetType, TEXT("Base.Turnip"));
			const UFarmCropData* Turnip = UVestigeContentCatalog::Get(&Spawner.GetWorld())->Find<UFarmCropData>(TurnipId);
			ASSERT_THAT(IsNotNull(Turnip));

			TArray<FString> Names;
			for (int32 Day = 0; Day <= Turnip->GetDaysToMature(); ++Day)
			{
				const UStaticMesh* Mesh = Turnip->GetGrowthLook(Day).Mesh;
				ASSERT_THAT(IsNotNull(Mesh));
				Names.AddUnique(Mesh->GetName());
			}

			ASSERT_THAT(IsTrue(Names.Num() == 4));
			ASSERT_THAT(IsTrue(Names[0] == TEXT("SM_Crop_Turnip_Stage0")));
			ASSERT_THAT(IsTrue(Names[1] == TEXT("SM_Crop_Turnip_Stage1")));
			ASSERT_THAT(IsTrue(Names[2] == TEXT("SM_Crop_Turnip_Stage2")));
			ASSERT_THAT(IsTrue(Names[3] == TEXT("SM_Crop_Turnip_Ripe")));
		}

		static FVector GetSize(const UStaticMesh& Mesh) { return Mesh.GetBounds().BoxExtent * 2.0; }

		static float GetSpread(const UStaticMesh& Mesh)
		{
			const FVector Size = GetSize(Mesh);
			return FMath::Max(Size.X, Size.Y);
		}

		/** Every material slot of Mesh is set and works with instanced drawing, with the usage flag saved. */
		static bool HasInstancedMaterials(const UStaticMesh& Mesh)
		{
			const TArray<FStaticMaterial>& Slots = Mesh.GetStaticMaterials();
			if (Slots.IsEmpty())
			{
				return false;
			}
			for (const FStaticMaterial& Slot : Slots)
			{
				UMaterialInterface* Material = Slot.MaterialInterface;
				if (!Material || !Material->GetUsageByFlag(MATUSAGE_InstancedStaticMeshes))
				{
					return false;
				}
				UMaterial* Base = Material->GetMaterial();
				if (!Base || Base->IsUsageFlagDirty(MATUSAGE_InstancedStaticMeshes))
				{
					return false;
				}
			}
			return true;
		}

		TEST_METHOD(EveryBaseCropHasALookForEachDayAndRipensVisibly)
		{
			Spawner.InitializeGameSubsystems();
			const UVestigeContentCatalog* Catalog = UVestigeContentCatalog::Get(&Spawner.GetWorld());
			ASSERT_THAT(IsNotNull(Catalog));
			TArray<FPrimaryAssetId> Ids;
			UAssetManager::Get().GetPrimaryAssetIdList(UFarmCropData::AssetType, Ids);
			ASSERT_THAT(IsFalse(Ids.IsEmpty()));

			for (const FPrimaryAssetId& Id : Ids)
			{
				const FString Name = Id.ToString();
				const UFarmCropData* Crop = Catalog->Find<UFarmCropData>(Id);
				ASSERT_THAT(IsNotNull(Crop, *Name));
				const int32 Days = Crop->GetDaysToMature();
				const UStaticMesh* RipeMesh = Crop->GetGrowthLook(Days).Mesh;
				ASSERT_THAT(IsNotNull(RipeMesh, *Name));
				ASSERT_THAT(IsTrue(HasInstancedMaterials(*RipeMesh), *Name));
				ASSERT_THAT(IsTrue(Crop->IsRipeAt(Days), *Name));

				// Every day shows a mesh, only the ripe day shows the ripe mesh, and what is drawn stays within the design's sizes.
				TArray<const UStaticMesh*> StageMeshes;
				for (int32 Day = 0; Day <= Days; ++Day)
				{
					const FFarmCropGrowthLook Look = Crop->GetGrowthLook(Day);
					const FString DayName = FString::Printf(TEXT("%s day %d"), *Name, Day);
					ASSERT_THAT(IsNotNull(Look.Mesh, *DayName));
					ASSERT_THAT(IsTrue((Look.Mesh == RipeMesh) == (Day == Days), *DayName));
					const float ShownSpread = GetSpread(*Look.Mesh) * Look.Scale;
					ASSERT_THAT(IsTrue(
						ShownSpread <= MaximumSpread * (1.f + Tolerance), *FString::Printf(TEXT("%s spread %.1f"), *DayName, ShownSpread)));
					if (Day == 0)
					{
						ASSERT_THAT(IsTrue(ShownSpread >= MinimumSeedlingSpread * (1.f - Tolerance),
							*FString::Printf(TEXT("%s spread %.1f"), *DayName, ShownSpread)));
					}
					if (Day < Days)
					{
						StageMeshes.AddUnique(Look.Mesh);
					}
				}

				// The first change shows after the first night.
				if (StageMeshes.Num() > 1)
				{
					ASSERT_THAT(IsTrue(Crop->GetGrowthLook(1).Mesh == StageMeshes[1], *Name));
				}

				// The art rule: a new stage at its start scale is never smaller than the end of the stage before.
				const float MinimumGrowth = 1.f / Crop->GetStageStartScale() * (1.f - Tolerance);
				for (int32 Index = 0; Index < StageMeshes.Num(); ++Index)
				{
					ASSERT_THAT(IsTrue(HasInstancedMaterials(*StageMeshes[Index]), *Name));
					if (Index == 0)
					{
						continue;
					}
					const FVector Before = GetSize(*StageMeshes[Index - 1]);
					const FVector After = GetSize(*StageMeshes[Index]);
					const FString StageName = FString::Printf(TEXT("%s stage %d height %.1f -> %.1f, spread %.1f -> %.1f, needs x%.3f"),
						*Name, Index, Before.Z, After.Z, GetSpread(*StageMeshes[Index - 1]), GetSpread(*StageMeshes[Index]), MinimumGrowth);
					ASSERT_THAT(IsTrue(After.Z >= Before.Z * MinimumGrowth, *StageName));
					ASSERT_THAT(IsTrue(GetSpread(*StageMeshes[Index]) >= GetSpread(*StageMeshes[Index - 1]) * MinimumGrowth, *StageName));
				}
			}
		}

		// Seasons. The made-up calendar has four seasons of ten days, named Alpha to Delta, on the real season tags.
		static constexpr int32 SeasonDays = 10;
		static constexpr const TCHAR* Spring = TEXT("Vestige.Season.Spring");
		static constexpr const TCHAR* Summer = TEXT("Vestige.Season.Summer");
		static constexpr const TCHAR* Autumn = TEXT("Vestige.Season.Autumn");
		static constexpr const TCHAR* Winter = TEXT("Vestige.Season.Winter");

		static FGameplayTag Tag(const TCHAR* TagName) { return FGameplayTag::RequestGameplayTag(FName(TagName)); }

		static FVestigeSeason MakeSeason(const TCHAR* TagName, const TCHAR* Name)
		{
			FVestigeSeason Season;
			Season.Tag = Tag(TagName);
			Season.DisplayName = FText::FromString(Name);
			Season.NameInSentence = FText::FromString(FString(Name).ToLower());
			return Season;
		}

		static FVestigeCalendar MakeCalendar()
		{
			return FVestigeCalendar::MakeForTests(SeasonDays,
				{MakeSeason(Spring, TEXT("Alpha")), MakeSeason(Summer, TEXT("Beta")), MakeSeason(Autumn, TEXT("Gamma")),
					MakeSeason(Winter, TEXT("Delta"))});
		}

		/** A crop that takes Days to ripen and grows in the seasons of these tags. */
		UFarmCropData& MakeSeasonCrop(int32 Days, std::initializer_list<const TCHAR*> SeasonTags)
		{
			UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();
			Crop.SetDaysToMatureForTests(Days);
			FGameplayTagContainer Listed;
			for (const TCHAR* SeasonTag : SeasonTags)
			{
				Listed.AddTag(Tag(SeasonTag));
			}
			Crop.SetSeasonsForTests(Listed);
			return Crop;
		}

		TEST_METHOD(ACropFitsWhenItRipensOnTheSeasonsLastDay)
		{
			const FVestigeCalendar Calendar = MakeCalendar();
			// Day 7 has 3 days left in season one.
			const FFarmSeasonFit Fits = MakeSeasonCrop(3, {Spring}).GetSeasonFit(Calendar, 7);
			ASSERT_THAT(IsTrue(Fits.Fit == EFarmSeasonFit::Fits));
			ASSERT_THAT(IsTrue(Fits.DaysToDeadline == 3));

			const FFarmSeasonFit Late = MakeSeasonCrop(4, {Spring}).GetSeasonFit(Calendar, 7);
			ASSERT_THAT(IsTrue(Late.Fit == EFarmSeasonFit::TooLate));
			ASSERT_THAT(IsTrue(Late.DaysToDeadline == 3));
			ASSERT_THAT(IsTrue(Late.DaysToRipe == 4));
			ASSERT_THAT(IsTrue(Late.DeadlineSeasonIndex == 1));
		}

		TEST_METHOD(ACropIsOutOfSeasonInASeasonItDoesntList)
		{
			const FVestigeCalendar Calendar = MakeCalendar();
			const FFarmSeasonFit Fit = MakeSeasonCrop(3, {Spring}).GetSeasonFit(Calendar, SeasonDays + 1);
			ASSERT_THAT(IsTrue(Fit.Fit == EFarmSeasonFit::OutOfSeason));
			ASSERT_THAT(IsTrue(Fit.PlantSeasonIndex == 0));
		}

		TEST_METHOD(TheSeasonToPlantIsTheNextOneTheCropListsIntoTheNextYear)
		{
			const FVestigeCalendar Calendar = MakeCalendar();
			const UFarmCropData& Crop = MakeSeasonCrop(3, {Spring, Autumn});
			ASSERT_THAT(IsTrue(Crop.GetSeasonFit(Calendar, SeasonDays + 1).PlantSeasonIndex == 2));
			ASSERT_THAT(IsTrue(Crop.GetSeasonFit(Calendar, 3 * SeasonDays + 1).PlantSeasonIndex == 0));
		}

		TEST_METHOD(AFollowingListedSeasonAddsItsDays)
		{
			const FVestigeCalendar Calendar = MakeCalendar();
			// Day 8 has 2 days left of spring, and summer adds ten more.
			const FFarmSeasonFit Fits = MakeSeasonCrop(12, {Spring, Summer}).GetSeasonFit(Calendar, 8);
			ASSERT_THAT(IsTrue(Fits.Fit == EFarmSeasonFit::Fits));
			ASSERT_THAT(IsTrue(Fits.DaysToDeadline == 12));
			ASSERT_THAT(IsTrue(Fits.DeadlineSeasonIndex == 2));

			ASSERT_THAT(IsTrue(MakeSeasonCrop(13, {Spring, Summer}).GetSeasonFit(Calendar, 8).Fit == EFarmSeasonFit::TooLate));
		}

		TEST_METHOD(ACropOfEverySeasonIsNeverTooLate)
		{
			const FVestigeCalendar Calendar = MakeCalendar();
			const UFarmCropData& Crop = MakeSeasonCrop(1000, {Spring, Summer, Autumn, Winter});
			for (const int32 Day : {1, 10, 25, 4 * SeasonDays})
			{
				const FFarmSeasonFit Fit = Crop.GetSeasonFit(Calendar, Day);
				ASSERT_THAT(IsTrue(Fit.Fit == EFarmSeasonFit::Fits));
				ASSERT_THAT(IsTrue(Fit.DeadlineSeasonIndex == INDEX_NONE));
			}
		}

		TEST_METHOD(ACropListingNoSeasonGrowsInEverySeason)
		{
			const FVestigeCalendar Calendar = MakeCalendar();
			const UFarmCropData& Crop = MakeSeasonCrop(1000, {});
			for (const FVestigeSeason& Season : Calendar.GetSeasons())
			{
				ASSERT_THAT(IsTrue(Crop.GrowsIn(Season.Tag, Calendar)));
			}
			ASSERT_THAT(IsTrue(Crop.GetSeasonFit(Calendar, SeasonDays + 1).Fit == EFarmSeasonFit::Fits));
		}

		TEST_METHOD(ACropListingOnlyUnknownSeasonsGrowsInEverySeason)
		{
			const FVestigeCalendar Calendar = MakeCalendar();
			const UFarmCropData& Crop = MakeSeasonCrop(1000, {TEXT("Vestige.Feedback.Farm")});
			for (const FVestigeSeason& Season : Calendar.GetSeasons())
			{
				ASSERT_THAT(IsTrue(Crop.GrowsIn(Season.Tag, Calendar)));
			}
			ASSERT_THAT(IsTrue(Crop.GetSeasonFit(Calendar, 1).Fit == EFarmSeasonFit::Fits));
		}

		TEST_METHOD(WithoutSeasonsEveryCropFits)
		{
			const FVestigeCalendar Calendar = FVestigeCalendar::MakeForTests(SeasonDays, {});
			const UFarmCropData& Crop = MakeSeasonCrop(1000, {Spring});
			ASSERT_THAT(IsTrue(Crop.GetSeasonFit(Calendar, 1).Fit == EFarmSeasonFit::Fits));
			ASSERT_THAT(IsTrue(Crop.GetSeasonFit(Calendar, 57).Fit == EFarmSeasonFit::Fits));
		}

		TEST_METHOD(OnTheLastDayNoDaysAreLeftBeforeTheDeadline)
		{
			const FVestigeCalendar Calendar = MakeCalendar();
			const FFarmSeasonFit Late = MakeSeasonCrop(2, {Spring}).GetSeasonFit(Calendar, SeasonDays);
			ASSERT_THAT(IsTrue(Late.Fit == EFarmSeasonFit::TooLate));
			ASSERT_THAT(IsTrue(Late.DaysToDeadline == 0));
		}

		TEST_METHOD(SeasonsMatchExactly)
		{
			// The feedback tags are used only for their parent/child relation: Farm.TooLate is a child of Farm, which an inexact
			// match would take for the season Farm. MakeForTests doesn't check that its tags are seasons.
			FVestigeSeason First;
			First.Tag = Tag(TEXT("Vestige.Feedback.Farm"));
			FVestigeSeason Second;
			Second.Tag = Tag(TEXT("Vestige.Feedback.Tool"));
			const FVestigeCalendar Calendar = FVestigeCalendar::MakeForTests(SeasonDays, {First, Second});
			const UFarmCropData& Crop = MakeSeasonCrop(3, {TEXT("Vestige.Feedback.Farm.TooLate"), TEXT("Vestige.Feedback.Tool")});

			const FFarmSeasonFit Fit = Crop.GetSeasonFit(Calendar, 1);
			ASSERT_THAT(IsTrue(Fit.Fit == EFarmSeasonFit::OutOfSeason));
			ASSERT_THAT(IsTrue(Fit.PlantSeasonIndex == 1));
			ASSERT_THAT(IsFalse(Crop.GrowsIn(First.Tag, Calendar)));
			ASSERT_THAT(IsTrue(Crop.GrowsIn(Second.Tag, Calendar)));
		}

		/** The text of the value named Key, or an empty string when there is none. */
		static FString TextOf(const FFormatNamedArguments& Values, const TCHAR* Key)
		{
			const FFormatArgumentValue* Value = Values.Find(Key);
			return Value && Value->GetType() == EFormatArgumentType::Text ? Value->GetTextValue().ToString() : FString();
		}

		static int32 NumberOf(const FFormatNamedArguments& Values, const TCHAR* Key)
		{
			const FFormatArgumentValue* Value = Values.Find(Key);
			return Value && Value->GetType() == EFormatArgumentType::Int ? static_cast<int32>(Value->GetIntValue()) : INDEX_NONE;
		}

		TEST_METHOD(TheSeasonFitValuesNameTheSeasons)
		{
			const FVestigeCalendar Calendar = MakeCalendar();

			// Out of season on the first day of summer: the next season is spring of next year.
			const UFarmCropData& SpringCrop = MakeSeasonCrop(5, {Spring});
			const int32 SummerDay = SeasonDays + 1;
			const FFormatNamedArguments Out = SpringCrop.GetSeasonFit(Calendar, SummerDay).GetReportValues(Calendar, SummerDay);
			ASSERT_THAT(IsTrue(TextOf(Out, TEXT("PlantSeason")) == TEXT("Alpha")));
			ASSERT_THAT(IsTrue(TextOf(Out, TEXT("PlantSeasonInSentence")) == TEXT("alpha")));
			ASSERT_THAT(IsTrue(TextOf(Out, TEXT("SeasonInSentence")) == TEXT("beta")));
			ASSERT_THAT(IsTrue(NumberOf(Out, TEXT("DaysToRipe")) == 5));
			ASSERT_THAT(IsFalse(Out.Contains(TEXT("DaysToDeadline"))));

			// Too late with 2 days left in spring.
			const FFormatNamedArguments Late = SpringCrop.GetSeasonFit(Calendar, 8).GetReportValues(Calendar, 8);
			ASSERT_THAT(IsTrue(NumberOf(Late, TEXT("DaysToRipe")) == 5));
			ASSERT_THAT(IsTrue(NumberOf(Late, TEXT("DaysToDeadline")) == 2));
			ASSERT_THAT(IsTrue(TextOf(Late, TEXT("DeadlineSeason")) == TEXT("Beta")));
			ASSERT_THAT(IsTrue(TextOf(Late, TEXT("DeadlineSeasonInSentence")) == TEXT("beta")));
			ASSERT_THAT(IsFalse(Late.Contains(TEXT("PlantSeason"))));

			// A crop that fits names only its days to ripen.
			const FFormatNamedArguments Fits = MakeSeasonCrop(2, {Spring}).GetSeasonFit(Calendar, 8).GetReportValues(Calendar, 8);
			ASSERT_THAT(IsTrue(NumberOf(Fits, TEXT("DaysToRipe")) == 2));
			ASSERT_THAT(IsFalse(Fits.Contains(TEXT("PlantSeason"))));
			ASSERT_THAT(IsFalse(Fits.Contains(TEXT("DeadlineSeason"))));
		}

		// Contamination (Systems/27-Contamination.md).
		TEST_METHOD(ASlowerTileRateMultipliesTheDaysToRipeAndCanMakeItTooLate)
		{
			const FVestigeCalendar Calendar = MakeCalendar();
			const UFarmCropData& Crop = MakeSeasonCrop(3, {Spring});

			// Day 7 has 3 days left in spring: clean soil ripens on the last day, light soil needs six.
			const FFarmSeasonFit Clean = Crop.GetSeasonFit(Calendar, 7);
			const FFarmSeasonFit RateOne = Crop.GetSeasonFit(Calendar, 7, 1);
			const FFarmSeasonFit Light = Crop.GetSeasonFit(Calendar, 7, 2);

			ASSERT_THAT(IsTrue(Clean.Fit == EFarmSeasonFit::Fits && Clean.DaysToRipe == 3));
			ASSERT_THAT(IsTrue(RateOne.Fit == EFarmSeasonFit::Fits && RateOne.DaysToRipe == 3));
			ASSERT_THAT(IsTrue(Light.Fit == EFarmSeasonFit::TooLate));
			ASSERT_THAT(IsTrue(Light.DaysToRipe == 6));
			ASSERT_THAT(IsTrue(Light.DaysToDeadline == 3));
			ASSERT_THAT(IsTrue(Crop.GetSeasonFit(Calendar, 7, 0).DaysToRipe == 3));
		}

		TEST_METHOD(TheRateOfALevelPastTheListIsItsLastAndAnEmptyListGrowsEveryNight)
		{
			UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();
			ASSERT_THAT(IsTrue(Crop.GetWateredNightsPerGrowthDay(0) == 1));
			ASSERT_THAT(IsTrue(Crop.GetWateredNightsPerGrowthDay(1) == 2));
			ASSERT_THAT(IsTrue(Crop.GetWateredNightsPerGrowthDay(2) == 0));

			Crop.SetContaminationResponseForTests({1, 3}, 0, 0);
			ASSERT_THAT(IsTrue(Crop.GetWateredNightsPerGrowthDay(1) == 3));
			ASSERT_THAT(IsTrue(Crop.GetWateredNightsPerGrowthDay(5) == 3));
			ASSERT_THAT(IsTrue(Crop.GetWateredNightsPerGrowthDay(-1) == 1));

			Crop.SetContaminationResponseForTests({}, 0, 0);
			for (const int32 Level : {0, 1, 2, 7})
			{
				ASSERT_THAT(IsTrue(Crop.GetWateredNightsPerGrowthDay(Level) == 1));
			}
		}

		TEST_METHOD(OnlyACropWithAMutatedItemCanMutate)
		{
			UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();
			ASSERT_THAT(IsFalse(Crop.CanMutate()));

			Crop.SetMutationForTests(FPrimaryAssetId(UVestigeItemData::AssetType, TEXT("Test.Mutated")), SoftOne(nullptr));

			ASSERT_THAT(IsTrue(Crop.CanMutate()));
		}

		TEST_METHOD(TheMutatedRipeMeshFallsBackToTheRipeMesh)
		{
			UFarmCropData& Crop = MakeCrop(4, {0, 2});
			ASSERT_THAT(IsTrue(Crop.GetMutatedRipeMesh() == Ripe.Get()));

			UStaticMesh& Mutated = Spawner.SpawnObject<UStaticMesh>();
			Crop.SetMutationForTests(FPrimaryAssetId(UVestigeItemData::AssetType, TEXT("Test.Mutated")), SoftOne(&Mutated));

			ASSERT_THAT(IsTrue(Crop.GetMutatedRipeMesh() == &Mutated));
		}

		/** A base content asset of this type by ContentId, from the catalogue. */
		template <typename T> const T* FindBase(FName ContentId)
		{
			return UVestigeContentCatalog::Get(&Spawner.GetWorld())->Find<T>(FPrimaryAssetId(T::AssetType, ContentId));
		}

		TEST_METHOD(TheSpringCropsSlowOnLightSoilStopOnHeavyAndMutateIntoItemsWorthTwice)
		{
			Spawner.InitializeGameSubsystems();
			for (const TCHAR* CropId : {TEXT("Base.Turnip"), TEXT("Base.Potato"), TEXT("Base.Cabbage")})
			{
				const UFarmCropData* Crop = FindBase<UFarmCropData>(CropId);
				ASSERT_THAT(IsNotNull(Crop, CropId));
				ASSERT_THAT(IsTrue(Crop->GetWateredNightsPerGrowthDay(0) == 1, CropId));
				ASSERT_THAT(IsTrue(Crop->GetWateredNightsPerGrowthDay(1) == 2, CropId));
				ASSERT_THAT(IsTrue(Crop->GetWateredNightsPerGrowthDay(2) == 0, CropId));
				ASSERT_THAT(IsTrue(Crop->GetMinPlantingContamination() == 0, CropId));
				ASSERT_THAT(IsTrue(Crop->GetContaminationCleanedPerHarvest() == 0, CropId));
				ASSERT_THAT(IsTrue(Crop->CanMutate(), CropId));

				const UVestigeItemData* Normal = FindBase<UVestigeItemData>(Crop->GetHarvestItemId().PrimaryAssetName);
				const UVestigeItemData* Mutated = FindBase<UVestigeItemData>(Crop->GetMutatedHarvestItemId().PrimaryAssetName);
				ASSERT_THAT(IsTrue(Crop->GetMutatedHarvestItemId().PrimaryAssetType == UVestigeItemData::AssetType, CropId));
				ASSERT_THAT(IsNotNull(Normal, CropId));
				ASSERT_THAT(IsNotNull(Mutated, CropId));
				ASSERT_THAT(IsTrue(Mutated != Normal, CropId));
				ASSERT_THAT(IsTrue(Normal->GetSellPrice() > 0, CropId));
				ASSERT_THAT(IsTrue(Mutated->GetSellPrice() == 2 * Normal->GetSellPrice(), CropId));
			}
		}

		TEST_METHOD(TheCleaningCropGrowsOnlyOnContaminatedSoilCleansOneLevelAndNeverMutates)
		{
			Spawner.InitializeGameSubsystems();
			const UFarmCropData* Crop = FindBase<UFarmCropData>(TEXT("Base.CleanupCrop"));
			ASSERT_THAT(IsNotNull(Crop));

			ASSERT_THAT(IsTrue(Crop->GetWateredNightsPerGrowthDay(0) == 0));
			ASSERT_THAT(IsTrue(Crop->GetWateredNightsPerGrowthDay(1) == 1));
			ASSERT_THAT(IsTrue(Crop->GetWateredNightsPerGrowthDay(2) == 1));
			ASSERT_THAT(IsTrue(Crop->GetMinPlantingContamination() == 1));
			ASSERT_THAT(IsTrue(Crop->GetContaminationCleanedPerHarvest() == 1));
			ASSERT_THAT(IsFalse(Crop->CanMutate()));
			ASSERT_THAT(IsTrue(Crop->GetDaysToMature() == 5));
			ASSERT_THAT(IsTrue(Crop->GetYield() == 1));
			ASSERT_THAT(IsTrue(Crop->GetHarvestItemId() == FPrimaryAssetId(UVestigeItemData::AssetType, TEXT("Base.CleanupCrop"))));
			const UVestigeItemData* Harvest = FindBase<UVestigeItemData>(TEXT("Base.CleanupCrop"));
			ASSERT_THAT(IsNotNull(Harvest));
			ASSERT_THAT(IsTrue(Harvest->GetSellPrice() == 30));
		}

#if WITH_EDITOR
		static bool HasErrors(const UObject& Object)
		{
			FDataValidationContext Context;
			const EDataValidationResult Result = Object.IsDataValid(Context);
			return Result == EDataValidationResult::Invalid || Context.GetNumErrors() > 0;
		}

		/** A valid crop: it names the turnip item and, unless told otherwise, has two stages (days 0 and 2) over four days. */
		UFarmCropData& MakeValidCrop(std::initializer_list<int32> StartDays = {0, 2})
		{
			UFarmCropData& Crop = MakeCrop(4, StartDays);
			Crop.SetContentIdForTests(TEXT("Test.Crop"));
			Crop.SetHarvestItemIdForTests(FPrimaryAssetId(UVestigeItemData::AssetType, TEXT("Base.Turnip")));
			Crop.SetSeasonsForTests(FGameplayTagContainer(UVestigeGameData::Get().GetCalendar().GetSeasons()[0].Tag));
			return Crop;
		}

		TEST_METHOD(ACropWithMeshesAndAHarvestItemIsValid) { ASSERT_THAT(IsFalse(HasErrors(MakeValidCrop()))); }

		TEST_METHOD(ACropWithoutSeasonsIsInvalid)
		{
			UFarmCropData& Crop = MakeValidCrop();
			Crop.SetSeasonsForTests(FGameplayTagContainer());

			ASSERT_THAT(IsTrue(HasErrors(Crop)));
		}

		TEST_METHOD(ASeasonNotInTheCalendarIsInvalid)
		{
			UFarmCropData& Crop = MakeValidCrop();
			// The parent tag of the seasons is no season of the calendar.
			Crop.SetSeasonsForTests(FGameplayTagContainer(FGameplayTag::RequestGameplayTag(TEXT("Vestige.Season"))));

			ASSERT_THAT(IsTrue(HasErrors(Crop)));
		}

		TEST_METHOD(ACropWithoutARipeMeshIsInvalid)
		{
			UFarmCropData& Crop = MakeValidCrop();
			Crop.SetGrowthStagesForTests({MakeStage(0, Stages[0].Get()), MakeStage(2, Stages[1].Get())}, SoftOne(nullptr));

			ASSERT_THAT(IsTrue(HasErrors(Crop)));
		}

		TEST_METHOD(ACropWithoutGrowthStagesIsInvalid)
		{
			UFarmCropData& Crop = MakeValidCrop();
			Crop.SetGrowthStagesForTests({}, SoftOne(Ripe.Get()));

			ASSERT_THAT(IsTrue(HasErrors(Crop)));
		}

		TEST_METHOD(AStageWithoutAMeshIsInvalid)
		{
			UFarmCropData& Crop = MakeValidCrop();
			Crop.SetGrowthStagesForTests({MakeStage(0, Stages[0].Get()), MakeStage(2, nullptr)}, SoftOne(Ripe.Get()));

			ASSERT_THAT(IsTrue(HasErrors(Crop)));
		}

		TEST_METHOD(TheRipeMeshAmongTheGrowthStagesIsInvalid)
		{
			UFarmCropData& Crop = MakeValidCrop();
			Crop.SetGrowthStagesForTests({MakeStage(0, Stages[0].Get()), MakeStage(2, Ripe.Get())}, SoftOne(Ripe.Get()));

			ASSERT_THAT(IsTrue(HasErrors(Crop)));
		}

		TEST_METHOD(AFirstStageAfterDayZeroIsInvalid) { ASSERT_THAT(IsTrue(HasErrors(MakeValidCrop({1, 2})))); }

		TEST_METHOD(StartDaysThatDontIncreaseAreInvalid)
		{
			ASSERT_THAT(IsTrue(HasErrors(MakeValidCrop({0, 1, 1}))));
			ASSERT_THAT(IsTrue(HasErrors(MakeValidCrop({0, 2, 1}))));
		}

		TEST_METHOD(AStageStartingAtOrAfterRipeIsInvalid)
		{
			// The crop takes 4 days: a stage starting on day 4 or later would never show.
			ASSERT_THAT(IsTrue(HasErrors(MakeValidCrop({0, 4}))));
			ASSERT_THAT(IsTrue(HasErrors(MakeValidCrop({0, 2, 5}))));
			ASSERT_THAT(IsFalse(HasErrors(MakeValidCrop({0, 1, 2, 3}))));
		}

		TEST_METHOD(ADaysToMatureBelowOneIsInvalid)
		{
			UFarmCropData& Crop = MakeValidCrop();
			Crop.SetDaysToMatureForTests(0);

			ASSERT_THAT(IsTrue(HasErrors(Crop)));
		}

		TEST_METHOD(AStageStartScaleOutOfRangeIsInvalid)
		{
			UFarmCropData& Crop = MakeValidCrop();
			for (const float Scale : {0.f, -0.5f, 1.1f})
			{
				Crop.SetStageStartScaleForTests(Scale);
				ASSERT_THAT(IsTrue(HasErrors(Crop), *FString::SanitizeFloat(Scale)));
			}
			Crop.SetStageStartScaleForTests(1.f);
			ASSERT_THAT(IsFalse(HasErrors(Crop)));
		}

		TEST_METHOD(AnEmptyOrNegativeContaminationResponseIsInvalid)
		{
			UFarmCropData& Crop = MakeValidCrop();
			Crop.SetContaminationResponseForTests({}, 0, 0);
			ASSERT_THAT(IsTrue(HasErrors(Crop)));

			Crop.SetContaminationResponseForTests({1, -1, 0}, 0, 0);
			ASSERT_THAT(IsTrue(HasErrors(Crop)));

			Crop.SetContaminationResponseForTests({1, 2}, 0, 0);
			ASSERT_THAT(IsFalse(HasErrors(Crop)));
		}

		TEST_METHOD(ACropThatGrowsNowhereItsSeedsMayGoIsInvalid)
		{
			UFarmCropData& Crop = MakeValidCrop();
			// The spring crops' response with seeds only for heavy soil, and past the list, where the last entry repeats.
			Crop.SetContaminationResponseForTests({1, 2, 0}, 2, 0);
			ASSERT_THAT(IsTrue(HasErrors(Crop)));
			Crop.SetContaminationResponseForTests({1, 2, 0}, 3, 0);
			ASSERT_THAT(IsTrue(HasErrors(Crop)));
			Crop.SetContaminationResponseForTests({0, 0, 0}, 0, 0);
			ASSERT_THAT(IsTrue(HasErrors(Crop)));

			// The cleaning crop's: only on contaminated soil, where it grows. A short list's last entry covers the levels after it.
			Crop.SetContaminationResponseForTests({0, 1, 1}, 1, 1);
			ASSERT_THAT(IsFalse(HasErrors(Crop)));
			Crop.SetContaminationResponseForTests({0, 1}, 2, 1);
			ASSERT_THAT(IsFalse(HasErrors(Crop)));
		}

		TEST_METHOD(AMutatedItemOfAnotherTypeOrNamingNoItemIsInvalid)
		{
			UFarmCropData& Crop = MakeValidCrop();
			Crop.SetMutationForTests(FPrimaryAssetId(UFarmCropData::AssetType, TEXT("Base.Turnip")), SoftOne(nullptr));
			ASSERT_THAT(IsTrue(HasErrors(Crop)));

			Crop.SetMutationForTests(FPrimaryAssetId(UVestigeItemData::AssetType, TEXT("Test.NoSuchItem")), SoftOne(nullptr));
			ASSERT_THAT(IsTrue(HasErrors(Crop)));

			Crop.SetMutationForTests(FPrimaryAssetId(UVestigeItemData::AssetType, TEXT("Base.MutatedTurnip")), SoftOne(nullptr));
			ASSERT_THAT(IsFalse(HasErrors(Crop)));
		}

		TEST_METHOD(AMutatedRipeMeshAmongTheGrowthStagesIsInvalid)
		{
			UFarmCropData& Crop = MakeValidCrop();
			Crop.SetMutationForTests(FPrimaryAssetId(UVestigeItemData::AssetType, TEXT("Base.MutatedTurnip")), SoftOne(Stages[1].Get()));
			ASSERT_THAT(IsTrue(HasErrors(Crop)));

			UStaticMesh& Mutated = Spawner.SpawnObject<UStaticMesh>();
			Crop.SetMutationForTests(FPrimaryAssetId(UVestigeItemData::AssetType, TEXT("Base.MutatedTurnip")), SoftOne(&Mutated));
			ASSERT_THAT(IsFalse(HasErrors(Crop)));
		}

		TEST_METHOD(TheCleaningCropIsValid)
		{
			Spawner.InitializeGameSubsystems();
			const UFarmCropData* Crop = FindBase<UFarmCropData>(TEXT("Base.CleanupCrop"));
			ASSERT_THAT(IsNotNull(Crop));

			ASSERT_THAT(IsFalse(HasErrors(*Crop)));
		}
#endif
	};
}

#endif
