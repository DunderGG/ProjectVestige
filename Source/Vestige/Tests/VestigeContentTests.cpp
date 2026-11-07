// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/ActorTestSpawner.h"
#include "Content/FarmCropData.h"
#include "Content/VestigeAssetManager.h"
#include "Content/VestigeToolData.h"
#include "CQTest.h"
#include "Engine/AssetManager.h"
#include "Farming/VestigeGameplayAbility_UseTool.h"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace VestigeContentTests
{
	TEST_CLASS(Content, "Vestige")
	{
		FActorTestSpawner Spawner;

		TEST_METHOD(TheProjectAssetManagerIsInUse)
		{
			// Set by AssetManagerClassName in Config/DefaultEngine.ini. Without it, duplicate content IDs go unreported.
			ASSERT_THAT(IsTrue(UAssetManager::Get().IsA<UVestigeAssetManager>()));
		}

		TEST_METHOD(WellFormedContentIdsAreAccepted)
		{
			ASSERT_THAT(IsTrue(UVestigeContentData::IsWellFormedContentId(TEXT("Base.Turnip"))));
			ASSERT_THAT(IsTrue(UVestigeContentData::IsWellFormedContentId(TEXT("Base.WateringCan"))));
			ASSERT_THAT(IsTrue(UVestigeContentData::IsWellFormedContentId(TEXT("MyMod2.BlueCorn3"))));
		}

		TEST_METHOD(BadlyFormedContentIdsAreRejected)
		{
			const TCHAR* const BadIds[] = {TEXT(""), TEXT("Turnip"), TEXT("Base."), TEXT(".Turnip"), TEXT("Base.Turnip.Big"),
				TEXT("base.Turnip"), TEXT("Base.turnip"), TEXT("Base.Watering Can"), TEXT("Base.Watering_Can"), TEXT("Base.1Turnip")};
			for (const TCHAR* BadId : BadIds)
			{
				ASSERT_THAT(IsFalse(UVestigeContentData::IsWellFormedContentId(BadId)));
			}
		}

		TEST_METHOD(PrimaryAssetIdIsBuiltFromTheTypeAndContentId)
		{
			UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();
			ASSERT_THAT(IsFalse(Crop.GetPrimaryAssetId().IsValid()));

			Crop.SetContentIdForTests(TEXT("Base.Turnip"));

			ASSERT_THAT(IsTrue(Crop.GetPrimaryAssetId() == FPrimaryAssetId(UFarmCropData::AssetType, TEXT("Base.Turnip"))));
		}

		TEST_METHOD(DuplicateContentIdsAreFoundWithEveryPackage)
		{
			const FPrimaryAssetId Turnip(UFarmCropData::AssetType, TEXT("Base.Turnip"));
			const FPrimaryAssetId Hoe(UVestigeToolData::AssetType, TEXT("Base.Hoe"));
			const TPair<FPrimaryAssetId, FName> Assets[] = {{Turnip, TEXT("/Game/Vestige/Crops/DA_Crop_Turnip")},
				{Hoe, TEXT("/Game/Vestige/Tools/DA_Tool_Hoe")}, {Turnip, TEXT("/Game/Vestige/Crops/DA_Crop_TurnipCopy")}};

			const TArray<FVestigeDuplicateContentId> Duplicates = UVestigeAssetManager::FindDuplicateContentIds(Assets);

			ASSERT_THAT(IsTrue(Duplicates.Num() == 1));
			ASSERT_THAT(IsTrue(Duplicates[0].Id == Turnip));
			ASSERT_THAT(IsTrue(Duplicates[0].Packages.Num() == 2));
		}

		TEST_METHOD(TheSameNameInTwoContentTypesIsNotADuplicate)
		{
			const TPair<FPrimaryAssetId, FName> Assets[] = {
				{FPrimaryAssetId(UFarmCropData::AssetType, TEXT("Base.Seeds")), TEXT("/Game/A")},
				{FPrimaryAssetId(UVestigeToolData::AssetType, TEXT("Base.Seeds")), TEXT("/Game/B")}};

			ASSERT_THAT(IsTrue(UVestigeAssetManager::FindDuplicateContentIds(Assets).IsEmpty()));
		}

		TEST_METHOD(EveryToolNamesAToolAbility)
		{
			TArray<FPrimaryAssetId> ToolIds;
			UAssetManager::Get().GetPrimaryAssetIdList(UVestigeToolData::AssetType, ToolIds);
			ASSERT_THAT(IsFalse(ToolIds.IsEmpty()));

			for (const FPrimaryAssetId& ToolId : ToolIds)
			{
				const UVestigeToolData* Tool = Cast<UVestigeToolData>(UVestigeContentData::LoadWithVisuals(ToolId));
				ASSERT_THAT(IsNotNull(Tool, *ToolId.ToString()));
				const UClass* Ability = Tool->GetAbility();
				ASSERT_THAT(IsNotNull(Ability, *ToolId.ToString()));
				ASSERT_THAT(IsTrue(Ability->IsChildOf<UVestigeGameplayAbility_UseTool>(), *ToolId.ToString()));
				ASSERT_THAT(IsFalse(Ability->HasAnyClassFlags(CLASS_Abstract), *ToolId.ToString()));
			}
		}
	};
}

#endif
