// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/ActorTestSpawner.h"
#include "Content/FarmCropData.h"
#include "Content/FarmSeedFragment.h"
#include "Content/VestigeAssetManager.h"
#include "Content/VestigeContentCatalog.h"
#include "Content/VestigeItemData.h"
#include "Content/VestigeToolData.h"
#include "Content/VestigeToolFragment.h"
#include "Content/VestigeShopData.h"
#include "Components/StaticMeshComponent.h"
#include "CQTest.h"
#include "Engine/AssetManager.h"
#include "Engine/StaticMesh.h"
#include "Farming/FarmShopDispenser.h"
#include "Farming/VestigeGameplayAbility_UseTool.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

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
			Spawner.InitializeGameSubsystems();
			const UVestigeContentCatalog* Catalog = UVestigeContentCatalog::Get(&Spawner.GetWorld());
			TArray<FPrimaryAssetId> ToolIds;
			UAssetManager::Get().GetPrimaryAssetIdList(UVestigeToolData::AssetType, ToolIds);
			ASSERT_THAT(IsFalse(ToolIds.IsEmpty()));

			for (const FPrimaryAssetId& ToolId : ToolIds)
			{
				const UVestigeToolData* Tool = Catalog->Find<UVestigeToolData>(ToolId);
				ASSERT_THAT(IsNotNull(Tool, *ToolId.ToString()));
				const UClass* Ability = Tool->GetAbility();
				ASSERT_THAT(IsNotNull(Ability, *ToolId.ToString()));
				ASSERT_THAT(IsTrue(Ability->IsChildOf<UVestigeGameplayAbility_UseTool>(), *ToolId.ToString()));
				ASSERT_THAT(IsFalse(Ability->HasAnyClassFlags(CLASS_Abstract), *ToolId.ToString()));
			}
		}

		/** A seed without a tool fragment would harvest instead of plant when held. */
		TEST_METHOD(EverySeedItemNamesATool)
		{
			TArray<FPrimaryAssetId> ItemIds;
			UAssetManager::Get().GetPrimaryAssetIdList(UVestigeItemData::AssetType, ItemIds);
			ASSERT_THAT(IsFalse(ItemIds.IsEmpty()));

			for (const FPrimaryAssetId& ItemId : ItemIds)
			{
				const UVestigeItemData* Item = Cast<UVestigeItemData>(UAssetManager::Get().GetPrimaryAssetPath(ItemId).TryLoad());
				ASSERT_THAT(IsNotNull(Item, *ItemId.ToString()));
				if (Item->FindFragment<UFarmSeedFragment>())
				{
					ASSERT_THAT(IsNotNull(Item->FindFragment<UVestigeToolFragment>(), *ItemId.ToString()));
				}
			}
		}

		/**
		 * Every crop can be planted: some seed item names it. A crop without one is unreachable. A later roadmap item (mutated
		 * crops, found rather than bought) may add a named exception here.
		 */
		TEST_METHOD(EveryCropHasASeedItem)
		{
			TArray<FPrimaryAssetId> CropIds;
			UAssetManager::Get().GetPrimaryAssetIdList(UFarmCropData::AssetType, CropIds);
			ASSERT_THAT(IsFalse(CropIds.IsEmpty()));
			TArray<FPrimaryAssetId> ItemIds;
			UAssetManager::Get().GetPrimaryAssetIdList(UVestigeItemData::AssetType, ItemIds);

			TSet<FPrimaryAssetId> PlantedCrops;
			for (const FPrimaryAssetId& ItemId : ItemIds)
			{
				const UVestigeItemData* Item = Cast<UVestigeItemData>(UAssetManager::Get().GetPrimaryAssetPath(ItemId).TryLoad());
				ASSERT_THAT(IsNotNull(Item, *ItemId.ToString()));
				if (const UFarmSeedFragment* Seed = Item->FindFragment<UFarmSeedFragment>())
				{
					PlantedCrops.Add(Seed->GetCropId());
				}
			}

			for (const FPrimaryAssetId& CropId : CropIds)
			{
				ASSERT_THAT(IsTrue(PlantedCrops.Contains(CropId), *CropId.ToString()));
			}
		}

		/** The slice's one shop sells the three seeds at GameDesign.md's prices, in the order a shop screen shows them. */
		TEST_METHOD(TheFarmShopSellsTheThreeSeedsAtTheirPrices)
		{
			Spawner.InitializeGameSubsystems();
			const UVestigeContentCatalog* Catalog = UVestigeContentCatalog::Get(&Spawner.GetWorld());
			const UVestigeShopData* Shop =
				Catalog->Find<UVestigeShopData>(FPrimaryAssetId(UVestigeShopData::AssetType, TEXT("Base.FarmShop")));
			ASSERT_THAT(IsNotNull(Shop));

			const TPair<const TCHAR*, int32> Expected[] = {
				{TEXT("Base.TurnipSeeds"), 10}, {TEXT("Base.PotatoSeeds"), 20}, {TEXT("Base.CabbageSeeds"), 40}};
			const TArray<FVestigeShopOffer>& Offers = Shop->GetOffers();
			ASSERT_THAT(AreEqual(Offers.Num(), 3));
			for (int32 Index = 0; Index < 3; ++Index)
			{
				const FPrimaryAssetId ItemId(UVestigeItemData::AssetType, Expected[Index].Key);
				ASSERT_THAT(IsTrue(Offers[Index].ItemId == ItemId, Expected[Index].Key));
				ASSERT_THAT(AreEqual(Offers[Index].Price, Expected[Index].Value, Expected[Index].Key));
				ASSERT_THAT(IsNotNull(Catalog->Find<UVestigeItemData>(ItemId), Expected[Index].Key));
				ASSERT_THAT(IsTrue(Shop->FindOffer(ItemId) == &Offers[Index], Expected[Index].Key));
			}
			// An item the shop doesn't sell has no offer, never a free one.
			ASSERT_THAT(IsNull(Shop->FindOffer(FPrimaryAssetId(UVestigeItemData::AssetType, TEXT("Base.Turnip")))));
		}

		/** Each dispenser Blueprint names the farm shop and one seed, and has a mesh (the stand-in cube until the art). */
		TEST_METHOD(TheDispenserBlueprintsOfferTheThreeSeeds)
		{
			const FPrimaryAssetId FarmShop(UVestigeShopData::AssetType, TEXT("Base.FarmShop"));
			const TPair<const TCHAR*, const TCHAR*> Expected[] = {
				{TEXT("/Game/Vestige/Blueprints/BP_ShopDispenser_TurnipSeeds.BP_ShopDispenser_TurnipSeeds_C"), TEXT("Base.TurnipSeeds")},
				{TEXT("/Game/Vestige/Blueprints/BP_ShopDispenser_PotatoSeeds.BP_ShopDispenser_PotatoSeeds_C"), TEXT("Base.PotatoSeeds")},
				{TEXT("/Game/Vestige/Blueprints/BP_ShopDispenser_CabbageSeeds.BP_ShopDispenser_CabbageSeeds_C"),
					TEXT("Base.CabbageSeeds")}};
			for (const TPair<const TCHAR*, const TCHAR*>& Entry : Expected)
			{
				const UClass* DispenserClass = LoadClass<AFarmShopDispenser>(nullptr, Entry.Key);
				ASSERT_THAT(IsNotNull(DispenserClass, Entry.Key));
				const AFarmShopDispenser* Dispenser = GetDefault<AFarmShopDispenser>(DispenserClass);

				ASSERT_THAT(IsTrue(Dispenser->GetShopId() == FarmShop, Entry.Key));
				ASSERT_THAT(IsTrue(Dispenser->GetItemId() == FPrimaryAssetId(UVestigeItemData::AssetType, Entry.Value), Entry.Key));
				const UStaticMeshComponent* Mesh = Dispenser->FindComponentByClass<UStaticMeshComponent>();
				ASSERT_THAT(IsNotNull(Mesh, Entry.Key));
				ASSERT_THAT(IsNotNull(Mesh->GetStaticMesh().Get(), Entry.Key));
			}
		}

#if WITH_EDITOR
		/** Only the first offer of an item is ever found, so a second one's price would silently do nothing. */
		TEST_METHOD(AShopOfferingAnItemTwiceIsInvalid)
		{
			FVestigeShopOffer Offer;
			Offer.ItemId = FPrimaryAssetId(UVestigeItemData::AssetType, TEXT("Base.TurnipSeeds"));
			Offer.Price = 10;
			UVestigeShopData& Shop = Spawner.SpawnObject<UVestigeShopData>();
			Shop.SetContentIdForTests(TEXT("Test.Shop"));
			Shop.AddOfferForTests(Offer);
			FDataValidationContext Once;
			ASSERT_THAT(IsTrue(Shop.IsDataValid(Once) != EDataValidationResult::Invalid));
			ASSERT_THAT(AreEqual(Once.GetNumErrors(), 0));

			Offer.Price = 12;
			Shop.AddOfferForTests(Offer);
			FDataValidationContext Twice;

			ASSERT_THAT(IsTrue(Shop.IsDataValid(Twice) == EDataValidationResult::Invalid));
			ASSERT_THAT(AreEqual(Twice.GetNumErrors(), 1));
		}

		TEST_METHOD(AShopOfferWithoutAKnownItemOrAPriceIsInvalid)
		{
			const FPrimaryAssetId Bad[] = {FPrimaryAssetId(), FPrimaryAssetId(UFarmCropData::AssetType, TEXT("Base.Turnip")),
				FPrimaryAssetId(UVestigeItemData::AssetType, TEXT("Gone.Seeds"))};
			for (const FPrimaryAssetId& ItemId : Bad)
			{
				UVestigeShopData& Shop = Spawner.SpawnObject<UVestigeShopData>();
				Shop.SetContentIdForTests(TEXT("Test.Shop"));
				FVestigeShopOffer Offer;
				Offer.ItemId = ItemId;
				Offer.Price = 10;
				Shop.AddOfferForTests(Offer);
				FDataValidationContext Context;
				ASSERT_THAT(IsTrue(Shop.IsDataValid(Context) == EDataValidationResult::Invalid, ItemId.ToString()));
			}

			UVestigeShopData& Free = Spawner.SpawnObject<UVestigeShopData>();
			Free.SetContentIdForTests(TEXT("Test.Shop"));
			FVestigeShopOffer FreeOffer;
			FreeOffer.ItemId = FPrimaryAssetId(UVestigeItemData::AssetType, TEXT("Base.TurnipSeeds"));
			FreeOffer.Price = 0;
			Free.AddOfferForTests(FreeOffer);
			FDataValidationContext Context;
			ASSERT_THAT(IsTrue(Free.IsDataValid(Context) == EDataValidationResult::Invalid));
		}

		/** What the editor's validation reports when content is saved: no crop, item or shop may fail it. */
		TEST_METHOD(EveryCropAndItemPassesValidation)
		{
			for (const FPrimaryAssetType& Type : {UFarmCropData::AssetType, UVestigeItemData::AssetType, UVestigeShopData::AssetType})
			{
				TArray<FPrimaryAssetId> Ids;
				UAssetManager::Get().GetPrimaryAssetIdList(Type, Ids);
				ASSERT_THAT(IsFalse(Ids.IsEmpty(), *Type.ToString()));
				for (const FPrimaryAssetId& Id : Ids)
				{
					const UObject* Content = UAssetManager::Get().GetPrimaryAssetPath(Id).TryLoad();
					ASSERT_THAT(IsNotNull(Content, *Id.ToString()));
					FDataValidationContext Context;
					const EDataValidationResult Result = Content->IsDataValid(Context);
					ASSERT_THAT(IsTrue(Result != EDataValidationResult::Invalid && Context.GetNumErrors() == 0, *Id.ToString()));
				}
			}
		}
#endif
	};
}

#endif
