// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/ActorTestSpawner.h"
#include "Content/FarmCropData.h"
#include "Content/VestigeContentCatalog.h"
#include "Content/VestigeContentData.h"
#include "Content/VestigeGameData.h"
#include "Content/VestigeItemData.h"
#include "Content/VestigeToolData.h"
#include "CQTest.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "TestGameInstance.h"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace VestigeContentCatalogTests
{
	TEST_CLASS(ContentCatalog, "Vestige")
	{
		FActorTestSpawner Spawner;
		UVestigeContentCatalog* Catalog = nullptr;

		BEFORE_EACH()
		{
			Spawner.InitializeGameSubsystems();
			Catalog = UVestigeContentCatalog::Get(&Spawner.GetWorld());
			ASSERT_THAT(IsNotNull(Catalog));
		}

		static TArray<FPrimaryAssetId> IdsOfType(const FPrimaryAssetType& Type)
		{
			TArray<FPrimaryAssetId> Ids;
			UAssetManager::Get().GetPrimaryAssetIdList(Type, Ids);
			return Ids;
		}

		TEST_METHOD(KnowsEveryContentAsset)
		{
			const TPair<FPrimaryAssetType, UClass*> Types[] = {
				{UFarmCropData::AssetType, UFarmCropData::StaticClass()},
				{UVestigeToolData::AssetType, UVestigeToolData::StaticClass()},
				{UVestigeItemData::AssetType, UVestigeItemData::StaticClass()},
			};
			for (const TPair<FPrimaryAssetType, UClass*>& Type : Types)
			{
				const TArray<FPrimaryAssetId> Ids = IdsOfType(Type.Key);
				ASSERT_THAT(IsFalse(Ids.IsEmpty(), *Type.Key.ToString()));
				for (const FPrimaryAssetId& Id : Ids)
				{
					const UVestigeContentData* Content = Catalog->Find(Id);
					ASSERT_THAT(IsNotNull(Content, *Id.ToString()));
					ASSERT_THAT(IsTrue(Content->IsA(Type.Value), *Id.ToString()));
				}
			}

			// The game data is a primary asset too, but not content.
			const UVestigeGameData& GameData = UVestigeGameData::Get();
			ASSERT_THAT(IsTrue(GameData.GetPrimaryAssetId().IsValid()));
			ASSERT_THAT(IsNull(Catalog->Find(GameData.GetPrimaryAssetId())));
		}

		TEST_METHOD(LoadsTheVisualsOfCropsAndTools)
		{
			const TSharedPtr<FStreamableHandle> Handle = Catalog->GetLoadHandleForTests();
			ASSERT_THAT(IsTrue(Handle.IsValid()));
			TArray<FSoftObjectPath> Requested;
			Handle->GetRequestedAssets(Requested);

			int32 VisualsCount = 0;
			for (const FPrimaryAssetType& Type : {UFarmCropData::AssetType, UVestigeToolData::AssetType})
			{
				for (const FPrimaryAssetId& Id : IdsOfType(Type))
				{
					const FAssetBundleEntry Visuals = UAssetManager::Get().GetAssetBundleEntry(Id, UVestigeContentData::VisualsBundle);
					for (const FTopLevelAssetPath& Path : Visuals.AssetPaths)
					{
						ASSERT_THAT(IsTrue(Requested.Contains(FSoftObjectPath(Path)), *Path.ToString()));
						++VisualsCount;
					}
				}
			}
			// The turnip has growth meshes and the watering can a held mesh, so a missing bundle can't pass unnoticed.
			ASSERT_THAT(IsTrue(VisualsCount > 0));
		}

		TEST_METHOD(FindsOnlyTheAskedClass)
		{
			const FPrimaryAssetId HoeTool(UVestigeToolData::AssetType, TEXT("Base.Hoe"));
			const FPrimaryAssetId HoeItem(UVestigeItemData::AssetType, TEXT("Base.Hoe"));

			ASSERT_THAT(IsNotNull(Catalog->Find<UVestigeToolData>(HoeTool)));
			ASSERT_THAT(IsNull(Catalog->Find<UVestigeToolData>(HoeItem)));
			ASSERT_THAT(IsNotNull(Catalog->Find<UVestigeItemData>(HoeItem)));
			ASSERT_THAT(IsNull(Catalog->Find<UVestigeItemData>(HoeTool)));
			ASSERT_THAT(IsNull(Catalog->Find(FPrimaryAssetId(UVestigeItemData::AssetType, TEXT("SomeMod.Pumpkin")))));
		}

		TEST_METHOD(TestContentStaysInItsGame)
		{
			UVestigeItemData& Pumpkin = Spawner.SpawnObject<UVestigeItemData>();
			Pumpkin.SetContentIdForTests(TEXT("Test.Pumpkin"));
			UVestigeItemData& Turnip = Spawner.SpawnObject<UVestigeItemData>();
			Turnip.SetContentIdForTests(TEXT("Base.Turnip"));
			const UVestigeItemData& NoId = Spawner.SpawnObject<UVestigeItemData>();

			ASSERT_THAT(IsTrue(Catalog->AddForTests(Pumpkin)));
			ASSERT_THAT(IsTrue(Catalog->AddForTests(Turnip)));
			ASSERT_THAT(IsFalse(Catalog->AddForTests(NoId)));

			ASSERT_THAT(IsTrue(Catalog->Find(Pumpkin.GetPrimaryAssetId()) == &Pumpkin));
			ASSERT_THAT(IsTrue(Catalog->Find(Turnip.GetPrimaryAssetId()) == &Turnip));

			FActorTestSpawner OtherGame;
			OtherGame.InitializeGameSubsystems();
			const UVestigeContentCatalog* OtherCatalog = UVestigeContentCatalog::Get(&OtherGame.GetWorld());
			ASSERT_THAT(IsNotNull(OtherCatalog));
			ASSERT_THAT(IsNull(OtherCatalog->Find(Pumpkin.GetPrimaryAssetId())));
			ASSERT_THAT(IsTrue(OtherCatalog->Find(Turnip.GetPrimaryAssetId()) != &Turnip));
			ASSERT_THAT(IsNotNull(OtherCatalog->Find(Turnip.GetPrimaryAssetId())));
		}

		TEST_METHOD(ShuttingDownReleasesTheLoad)
		{
			FActorTestSpawner OtherGame;
			OtherGame.InitializeGameSubsystems();
			UVestigeContentCatalog* OtherCatalog = UVestigeContentCatalog::Get(&OtherGame.GetWorld());
			ASSERT_THAT(IsNotNull(OtherCatalog));
			const TSharedPtr<FStreamableHandle> Handle = OtherCatalog->GetLoadHandleForTests();
			ASSERT_THAT(IsTrue(Handle.IsValid() && Handle->IsActive()));
			const FPrimaryAssetId TurnipId(UVestigeItemData::AssetType, TEXT("Base.Turnip"));
			ASSERT_THAT(IsNotNull(OtherCatalog->Find(TurnipId)));

			OtherGame.GetGameInstance()->Shutdown();

			ASSERT_THAT(IsFalse(Handle->IsActive()));
			ASSERT_THAT(IsNull(OtherCatalog->Find(TurnipId)));
			// This test's own game still holds its load.
			ASSERT_THAT(IsTrue(Catalog->GetLoadHandleForTests()->IsActive()));
		}
	};
}

#endif
