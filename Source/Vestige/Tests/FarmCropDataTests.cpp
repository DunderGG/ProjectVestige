// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/ActorTestSpawner.h"
#include "Content/FarmCropData.h"
#include "Content/VestigeContentCatalog.h"
#include "Content/VestigeItemData.h"
#include "CQTest.h"
#include "Engine/AssetManager.h"
#include "Engine/StaticMesh.h"
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
			return Crop;
		}

		TEST_METHOD(ACropWithMeshesAndAHarvestItemIsValid) { ASSERT_THAT(IsFalse(HasErrors(MakeValidCrop()))); }

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
#endif
	};
}

#endif
