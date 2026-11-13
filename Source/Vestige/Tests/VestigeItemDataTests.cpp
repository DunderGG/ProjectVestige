// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/ActorTestSpawner.h"
#include "Content/FarmCropData.h"
#include "Content/FarmSeedFragment.h"
#include "Content/VestigeItemData.h"
#include "Content/VestigeToolData.h"
#include "Content/VestigeToolFragment.h"
#include "CQTest.h"
#include "Engine/AssetManager.h"
#include "Engine/StaticMesh.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace VestigeItemDataTests
{
	TEST_CLASS(Item, "Vestige")
	{
		FActorTestSpawner Spawner;

		// Members, not globals: the asset types are statics of other files, which may not be initialized yet when this file's
		// globals are.
		const FPrimaryAssetId TurnipCropId{UFarmCropData::AssetType, TEXT("Base.Turnip")};
		const FPrimaryAssetId TurnipItemId{UVestigeItemData::AssetType, TEXT("Base.Turnip")};
		const FPrimaryAssetId TurnipSeedsItemId{UVestigeItemData::AssetType, TEXT("Base.TurnipSeeds")};

		UVestigeItemData& MakeItem()
		{
			UVestigeItemData& Item = Spawner.SpawnObject<UVestigeItemData>();
			Item.SetContentIdForTests(TEXT("Test.Item"));
			return Item;
		}

		UFarmSeedFragment& AddSeedFragment(UVestigeItemData& Item, const FPrimaryAssetId& CropId)
		{
			UFarmSeedFragment& Seed = Spawner.SpawnObject<UFarmSeedFragment>();
			Seed.SetCropIdForTests(CropId);
			Item.AddFragmentForTests(&Seed);
			return Seed;
		}

		/** Loads a real content asset. Synchronous loading is fine in a test. */
		template <typename T> const T* LoadContent(const FPrimaryAssetId& Id)
		{
			return Cast<T>(UAssetManager::Get().GetPrimaryAssetPath(Id).TryLoad());
		}

		TEST_METHOD(FindFragmentReturnsTheFragmentOfThatClass)
		{
			UVestigeItemData& Item = MakeItem();
			const UFarmSeedFragment& Seed = AddSeedFragment(Item, TurnipCropId);

			ASSERT_THAT(IsTrue(Item.FindFragment<UFarmSeedFragment>() == &Seed));
			ASSERT_THAT(IsTrue(Item.FindFragmentByClass(UVestigeItemFragment::StaticClass()) == &Seed));
		}

		TEST_METHOD(FindFragmentReturnsNullWhenTheItemHasNone)
		{
			const UVestigeItemData& Item = MakeItem();

			ASSERT_THAT(IsNull(Item.FindFragment<UFarmSeedFragment>()));
			ASSERT_THAT(IsNull(Item.FindFragmentByClass(nullptr)));
		}

		TEST_METHOD(TheTurnipSeedsPlantTheTurnip)
		{
			const UVestigeItemData* Seeds = LoadContent<UVestigeItemData>(TurnipSeedsItemId);
			ASSERT_THAT(IsNotNull(Seeds));

			const UFarmSeedFragment* Seed = Seeds->FindFragment<UFarmSeedFragment>();
			ASSERT_THAT(IsNotNull(Seed));
			ASSERT_THAT(IsTrue(Seed->GetCropId() == TurnipCropId));
		}

		TEST_METHOD(TheTurnipCropYieldsTheTurnipItem)
		{
			const UFarmCropData* Crop = LoadContent<UFarmCropData>(TurnipCropId);
			ASSERT_THAT(IsNotNull(Crop));
			ASSERT_THAT(IsTrue(Crop->GetHarvestItemId() == TurnipItemId));

			const UVestigeItemData* Turnip = LoadContent<UVestigeItemData>(TurnipItemId);
			ASSERT_THAT(IsNotNull(Turnip));
			ASSERT_THAT(IsNull(Turnip->FindFragment<UFarmSeedFragment>()));
		}

		TEST_METHOD(ThePotatoAndCabbageSeedsPlantTheirCrops)
		{
			const TPair<const TCHAR*, const TCHAR*> Seeds[] = {{TEXT("Base.PotatoSeeds"), TEXT("Base.Potato")},
				{TEXT("Base.CabbageSeeds"), TEXT("Base.Cabbage")}};
			for (const TPair<const TCHAR*, const TCHAR*>& Seed : Seeds)
			{
				const UVestigeItemData* Item = LoadContent<UVestigeItemData>(FPrimaryAssetId(UVestigeItemData::AssetType, Seed.Key));
				ASSERT_THAT(IsNotNull(Item, Seed.Key));
				const UFarmSeedFragment* SeedFragment = Item->FindFragment<UFarmSeedFragment>();
				ASSERT_THAT(IsNotNull(SeedFragment, Seed.Key));
				ASSERT_THAT(IsTrue(SeedFragment->GetCropId() == FPrimaryAssetId(UFarmCropData::AssetType, Seed.Value), Seed.Key));
				const UVestigeToolFragment* ToolFragment = Item->FindFragment<UVestigeToolFragment>();
				ASSERT_THAT(IsNotNull(ToolFragment, Seed.Key));
				ASSERT_THAT(IsTrue(ToolFragment->GetToolId() == FPrimaryAssetId(UVestigeToolData::AssetType, TEXT("Base.Seeds")), Seed.Key));
			}
		}

#if WITH_EDITOR
		bool PassesValidation(const UObject& Object)
		{
			FDataValidationContext Context;
			const EDataValidationResult Result = Object.IsDataValid(Context);
			return Result != EDataValidationResult::Invalid && Context.GetNumErrors() == 0;
		}

		TEST_METHOD(AnItemWithAKnownSeedCropIsValid)
		{
			UVestigeItemData& Item = MakeItem();
			AddSeedFragment(Item, TurnipCropId);

			ASSERT_THAT(IsTrue(PassesValidation(Item)));
		}

		TEST_METHOD(AStackSizeBelowOneIsInvalid)
		{
			UVestigeItemData& Item = MakeItem();
			Item.SetMaxStackSizeForTests(0);

			ASSERT_THAT(IsFalse(PassesValidation(Item)));
		}

		TEST_METHOD(AnEmptyFragmentEntryIsInvalid)
		{
			UVestigeItemData& Item = MakeItem();
			Item.AddFragmentForTests(nullptr);

			ASSERT_THAT(IsFalse(PassesValidation(Item)));
		}

		TEST_METHOD(TwoFragmentsOfTheSameClassAreInvalid)
		{
			UVestigeItemData& Item = MakeItem();
			AddSeedFragment(Item, TurnipCropId);
			AddSeedFragment(Item, TurnipCropId);

			ASSERT_THAT(IsFalse(PassesValidation(Item)));
		}

		TEST_METHOD(ASeedWithoutACropIsInvalid)
		{
			UVestigeItemData& Item = MakeItem();
			AddSeedFragment(Item, FPrimaryAssetId());

			ASSERT_THAT(IsFalse(PassesValidation(Item)));
		}

		TEST_METHOD(ASeedNamingAnotherContentTypeIsInvalid)
		{
			UVestigeItemData& Item = MakeItem();
			AddSeedFragment(Item, FPrimaryAssetId(UVestigeToolData::AssetType, TEXT("Base.Hoe")));

			ASSERT_THAT(IsFalse(PassesValidation(Item)));
		}

		TEST_METHOD(ASeedNamingAnUnknownCropIsInvalid)
		{
			UVestigeItemData& Item = MakeItem();
			AddSeedFragment(Item, FPrimaryAssetId(UFarmCropData::AssetType, TEXT("Test.NoSuchCrop")));

			ASSERT_THAT(IsFalse(PassesValidation(Item)));
		}

		TEST_METHOD(AToolFragmentNeedsAKnownTool)
		{
			const FPrimaryAssetId Ids[] = {FPrimaryAssetId(), FPrimaryAssetId(UFarmCropData::AssetType, TEXT("Base.Turnip")),
				FPrimaryAssetId(UVestigeToolData::AssetType, TEXT("Test.NoSuchTool"))};
			for (const FPrimaryAssetId& Id : Ids)
			{
				UVestigeItemData& Item = MakeItem();
				UVestigeToolFragment& Tool = Spawner.SpawnObject<UVestigeToolFragment>();
				Tool.SetToolIdForTests(Id);
				Item.AddFragmentForTests(&Tool);
				ASSERT_THAT(IsFalse(PassesValidation(Item), *Id.ToString()));
			}

			UVestigeItemData& Item = MakeItem();
			UVestigeToolFragment& Tool = Spawner.SpawnObject<UVestigeToolFragment>();
			Tool.SetToolIdForTests(FPrimaryAssetId(UVestigeToolData::AssetType, TEXT("Base.Hoe")));
			Item.AddFragmentForTests(&Tool);
			ASSERT_THAT(IsTrue(PassesValidation(Item)));
		}

		TEST_METHOD(ACropWithoutAHarvestItemIsInvalid)
		{
			UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();
			Crop.SetContentIdForTests(TEXT("Test.Crop"));
			UStaticMesh& Stage = Spawner.SpawnObject<UStaticMesh>();
			UStaticMesh& Ripe = Spawner.SpawnObject<UStaticMesh>();
			Crop.SetMeshesForTests({TSoftObjectPtr<UStaticMesh>(&Stage)}, TSoftObjectPtr<UStaticMesh>(&Ripe));
			ASSERT_THAT(IsFalse(PassesValidation(Crop)));

			Crop.SetHarvestItemIdForTests(TurnipItemId);
			ASSERT_THAT(IsTrue(PassesValidation(Crop)));
		}
#endif
	};
}

#endif
