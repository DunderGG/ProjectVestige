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
	/** How a crop looks on each growth day: which stage mesh, at what scale, and the ripe mesh only when ripe. */
	TEST_CLASS(FarmCrop, "Vestige")
	{
		FActorTestSpawner Spawner;
		TArray<TObjectPtr<UStaticMesh>> Stages;
		TObjectPtr<UStaticMesh> Ripe;

		static TSoftObjectPtr<UStaticMesh> SoftOne(UStaticMesh* Mesh) { return TSoftObjectPtr<UStaticMesh>(Mesh); }

		static TArray<TSoftObjectPtr<UStaticMesh>> Soft(std::initializer_list<UStaticMesh*> Meshes)
		{
			TArray<TSoftObjectPtr<UStaticMesh>> Result;
			for (UStaticMesh* Mesh : Meshes)
			{
				Result.Add(SoftOne(Mesh));
			}
			return Result;
		}

		/** A crop that takes Days to mature and has StageCount growth stage meshes and a ripe mesh. */
		UFarmCropData& MakeCrop(int32 Days, int32 StageCount)
		{
			UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();
			Stages.Reset();
			TArray<TSoftObjectPtr<UStaticMesh>> SoftStages;
			for (int32 Index = 0; Index < StageCount; ++Index)
			{
				UStaticMesh& Stage = Spawner.SpawnObject<UStaticMesh>();
				Stages.Add(&Stage);
				SoftStages.Add(&Stage);
			}
			Ripe = &Spawner.SpawnObject<UStaticMesh>();
			Crop.SetDaysToMatureForTests(Days);
			Crop.SetMeshesForTests(SoftStages, SoftOne(Ripe.Get()));
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

		TEST_METHOD(AsManyStagesAsDaysShowOneStagePerDay)
		{
			const UFarmCropData& Crop = MakeCrop(3, 3);

			ASSERT_THAT(IsTrue(StageSequence(Crop) == TEXT("0,1,2")));
			for (int32 Day = 0; Day < 3; ++Day)
			{
				ASSERT_THAT(IsTrue(Crop.GetGrowthLook(Day).Mesh == Stages[Day].Get()));
			}
		}

		TEST_METHOD(StagesSpreadEvenlyOverALongCrop)
		{
			ASSERT_THAT(IsTrue(StageSequence(MakeCrop(10, 4)) == TEXT("0,0,0,1,1,2,2,2,3,3")));
			ASSERT_THAT(IsTrue(StageSequence(MakeCrop(6, 3)) == TEXT("0,0,1,1,2,2")));
		}

		TEST_METHOD(EveryStageShowsInOrderWhenThereAreEnoughDays)
		{
			for (int32 Days = 1; Days <= 12; ++Days)
			{
				for (int32 StageCount = 1; StageCount <= Days; ++StageCount)
				{
					const UFarmCropData& Crop = MakeCrop(Days, StageCount);
					const FString Name = FString::Printf(TEXT("D%d N%d"), Days, StageCount);
					ASSERT_THAT(IsTrue(Crop.GetGrowthStageIndex(0) == 0, *Name));
					ASSERT_THAT(IsTrue(Crop.GetGrowthStageIndex(Days - 1) == StageCount - 1, *Name));
					for (int32 Day = 1; Day < Days; ++Day)
					{
						const int32 Step = Crop.GetGrowthStageIndex(Day) - Crop.GetGrowthStageIndex(Day - 1);
						ASSERT_THAT(IsTrue(Step == 0 || Step == 1, *Name));
					}
				}
			}
		}

		TEST_METHOD(OnlyARipeCropShowsTheRipeMesh)
		{
			const UFarmCropData& Crop = MakeCrop(4, 2);

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
				ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Crop.GetGrowthLook(Day).Scale, 1.f)));
			}
		}

		TEST_METHOD(AMissingMeshGivesNoMesh)
		{
			UFarmCropData& Crop = MakeCrop(4, 2);

			Crop.SetMeshesForTests(Soft({Stages[0].Get(), nullptr}), SoftOne(Ripe.Get()));
			ASSERT_THAT(IsTrue(Crop.GetGrowthLook(0).Mesh == Stages[0].Get()));
			ASSERT_THAT(IsNull(Crop.GetGrowthLook(2).Mesh));

			Crop.SetMeshesForTests(Soft({}), SoftOne(Ripe.Get()));
			ASSERT_THAT(IsNull(Crop.GetGrowthLook(1).Mesh));
			ASSERT_THAT(IsTrue(Crop.GetGrowthStageIndex(1) == INDEX_NONE));
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Crop.GetGrowthLook(1).Scale, 1.f)));

			Crop.SetMeshesForTests(Soft({Stages[0].Get(), Stages[1].Get()}), SoftOne(nullptr));
			ASSERT_THAT(IsNull(Crop.GetGrowthLook(4).Mesh));
			ASSERT_THAT(IsTrue(Crop.GetGrowthLook(0).Mesh == Stages[0].Get()));
		}

		TEST_METHOD(NegativeDaysShowTheFirstStage)
		{
			const UFarmCropData& Crop = MakeCrop(10, 4);

			ASSERT_THAT(IsTrue(Crop.GetGrowthStageIndex(-3) == 0));
			ASSERT_THAT(IsTrue(Crop.GetGrowthLook(-3).Mesh == Stages[0].Get()));
		}

		TEST_METHOD(AThreeDayStageGrowsFromItsStartScaleToFull)
		{
			const UFarmCropData& Crop = MakeCrop(10, 4);

			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Crop.GetGrowthLook(0).Scale, 0.8f)));
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Crop.GetGrowthLook(1).Scale, 0.9f)));
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Crop.GetGrowthLook(2).Scale, 1.f)));
		}

		TEST_METHOD(ATwoDayStageShowsItsStartScaleThenFull)
		{
			const UFarmCropData& Crop = MakeCrop(10, 4);

			ASSERT_THAT(IsTrue(Crop.GetGrowthStageIndex(3) == 1 && Crop.GetGrowthStageIndex(4) == 1));
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Crop.GetGrowthLook(3).Scale, 0.8f)));
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Crop.GetGrowthLook(4).Scale, 1.f)));
		}

		TEST_METHOD(OneDayStagesAndTheRipeCropShowFullSize)
		{
			const UFarmCropData& Crop = MakeCrop(3, 3);

			for (int32 Day = 0; Day <= 3; ++Day)
			{
				ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Crop.GetGrowthLook(Day).Scale, 1.f)));
			}
		}

		TEST_METHOD(AStageSpansTheDaysThatShowIt)
		{
			for (int32 Days = 1; Days <= 12; ++Days)
			{
				for (int32 StageCount = 1; StageCount <= Days; ++StageCount)
				{
					const UFarmCropData& Crop = MakeCrop(Days, StageCount);
					for (int32 Stage = 0; Stage < StageCount; ++Stage)
					{
						const int32 First = (Stage * Days + StageCount - 1) / StageCount;
						const int32 Last = ((Stage + 1) * Days + StageCount - 1) / StageCount - 1;
						const FString Name = FString::Printf(TEXT("D%d N%d stage %d"), Days, StageCount, Stage);
						ASSERT_THAT(IsTrue(Last >= First, *Name));
						ASSERT_THAT(IsTrue(Crop.GetGrowthStageIndex(First) == Stage, *Name));
						ASSERT_THAT(IsTrue(Crop.GetGrowthStageIndex(Last) == Stage, *Name));
						ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Crop.GetGrowthLook(Last).Scale, 1.f), *Name));
						if (Last > First)
						{
							ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Crop.GetGrowthLook(First).Scale, 0.8f), *Name));
						}
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

				TArray<const UStaticMesh*> StageMeshes;
				for (int32 Day = 0; Day < Days; ++Day)
				{
					const UStaticMesh* Mesh = Crop->GetGrowthLook(Day).Mesh;
					ASSERT_THAT(IsNotNull(Mesh, *Name));
					ASSERT_THAT(IsTrue(Mesh != RipeMesh, *Name));
					ASSERT_THAT(IsFalse(Crop->IsRipeAt(Day), *Name));
					StageMeshes.AddUnique(Mesh);
				}
				ASSERT_THAT(IsTrue(Crop->IsRipeAt(Days), *Name));

				// The art rule: a new stage at its start scale is never smaller than the end of the stage before.
				const float MinimumGrowth = 1.f / Crop->GetStageStartScale() * 0.99f;
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
						*Name, Index, Before.Z, After.Z, FMath::Max(Before.X, Before.Y), FMath::Max(After.X, After.Y), MinimumGrowth);
					ASSERT_THAT(IsTrue(After.Z >= Before.Z * MinimumGrowth, *StageName));
					ASSERT_THAT(IsTrue(FMath::Max(After.X, After.Y) >= FMath::Max(Before.X, Before.Y) * MinimumGrowth, *StageName));
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

		/** A valid crop: it names the turnip item and has two stages over four days. */
		UFarmCropData& MakeValidCrop()
		{
			UFarmCropData& Crop = MakeCrop(4, 2);
			Crop.SetContentIdForTests(TEXT("Test.Crop"));
			Crop.SetHarvestItemIdForTests(FPrimaryAssetId(UVestigeItemData::AssetType, TEXT("Base.Turnip")));
			return Crop;
		}

		TEST_METHOD(ACropWithMeshesAndAHarvestItemIsValid)
		{
			ASSERT_THAT(IsFalse(HasErrors(MakeValidCrop())));
		}

		TEST_METHOD(ACropWithoutARipeMeshIsInvalid)
		{
			UFarmCropData& Crop = MakeValidCrop();
			Crop.SetMeshesForTests(Soft({Stages[0].Get(), Stages[1].Get()}), SoftOne(nullptr));

			ASSERT_THAT(IsTrue(HasErrors(Crop)));
		}

		TEST_METHOD(ACropWithoutGrowthStageMeshesIsInvalid)
		{
			UFarmCropData& Crop = MakeValidCrop();
			Crop.SetMeshesForTests(Soft({}), SoftOne(Ripe.Get()));

			ASSERT_THAT(IsTrue(HasErrors(Crop)));
		}

		TEST_METHOD(AnEmptyGrowthStageMeshIsInvalid)
		{
			UFarmCropData& Crop = MakeValidCrop();
			Crop.SetMeshesForTests(Soft({Stages[0].Get(), nullptr}), SoftOne(Ripe.Get()));

			ASSERT_THAT(IsTrue(HasErrors(Crop)));
		}

		TEST_METHOD(TheRipeMeshAmongTheGrowthStagesIsInvalid)
		{
			UFarmCropData& Crop = MakeValidCrop();
			Crop.SetMeshesForTests(Soft({Stages[0].Get(), Ripe.Get()}), SoftOne(Ripe.Get()));

			ASSERT_THAT(IsTrue(HasErrors(Crop)));
		}

		TEST_METHOD(MoreStagesThanDaysIsOnlyAWarning)
		{
			UFarmCropData& Crop = MakeValidCrop();
			Crop.SetDaysToMatureForTests(1);

			FDataValidationContext Context;
			const EDataValidationResult Result = Crop.IsDataValid(Context);

			ASSERT_THAT(IsTrue(Result != EDataValidationResult::Invalid));
			ASSERT_THAT(IsTrue(Context.GetNumErrors() == 0));
			ASSERT_THAT(IsTrue(Context.GetNumWarnings() == 1));
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
