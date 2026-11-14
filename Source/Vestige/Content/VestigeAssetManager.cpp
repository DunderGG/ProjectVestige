// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Content/VestigeAssetManager.h"

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Content/VestigeContentData.h"
#include "Content/VestigeContentLog.h"
#include "Content/VestigeGameData.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

TArray<FVestigeDuplicateContentId> UVestigeAssetManager::FindDuplicateContentIds(TConstArrayView<TPair<FPrimaryAssetId, FName>> Assets)
{
	TMap<FPrimaryAssetId, TArray<FName>> PackagesById;
	for (const TPair<FPrimaryAssetId, FName>& Asset : Assets)
	{
		PackagesById.FindOrAdd(Asset.Key).Add(Asset.Value);
	}

	TArray<FVestigeDuplicateContentId> Duplicates;
	for (TPair<FPrimaryAssetId, TArray<FName>>& Entry : PackagesById)
	{
		if (Entry.Value.Num() > 1)
		{
			Duplicates.Add({Entry.Key, MoveTemp(Entry.Value)});
		}
	}
	return Duplicates;
}

const UVestigeGameData* UVestigeAssetManager::GetGameData()
{
	const UVestigeAssetManager* Manager = Cast<UVestigeAssetManager>(UAssetManager::GetIfInitialized());
	return Manager ? Manager->GameData.Get() : nullptr;
}

void UVestigeAssetManager::StartInitialLoading()
{
	Super::StartInitialLoading();

	LoadGameData();
}

void UVestigeAssetManager::LoadGameData()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UVestigeAssetManager::LoadGameData);

	GameData = GameDataPath.LoadSynchronous();
	if (!GameData)
	{
		VESTIGE_LOG(this, LogVestigeContent, Error,
			"The game data {Path} didn't load, so nothing applies the stamina effects and the calendar has no seasons. Set GameDataPath "
			"in Config/DefaultGame.ini.",
			GameDataPath.ToString());
	}
}

void UVestigeAssetManager::PostInitialAssetScan()
{
	Super::PostInitialAssetScan();

	ReportDuplicateContentIds();
}

void UVestigeAssetManager::ReportDuplicateContentIds() const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UVestigeAssetManager::ReportDuplicateContentIds);

	TArray<FAssetData> ContentAssets;
	GetAssetRegistry().GetAssetsByClass(UVestigeContentData::StaticClass()->GetClassPathName(), ContentAssets, true);

	TArray<TPair<FPrimaryAssetId, FName>> Assets;
	Assets.Reserve(ContentAssets.Num());
	for (const FAssetData& Asset : ContentAssets)
	{
		// The ID is stored in the asset's registry tags when it is saved, so nothing has to load. An asset without a
		// valid ID is reported by its own data validation instead.
		const FPrimaryAssetId Id = Asset.GetPrimaryAssetId();
		if (Id.IsValid())
		{
			Assets.Emplace(Id, Asset.PackageName);
		}
	}

	for (const FVestigeDuplicateContentId& Duplicate : FindDuplicateContentIds(Assets))
	{
		TArray<FString> PackageNames;
		for (const FName Package : Duplicate.Packages)
		{
			PackageNames.Add(Package.ToString());
		}
		VESTIGE_LOG(this, LogVestigeContent, Error,
			"Content ID {ContentId} is used by more than one asset, so only one of them is used: {Packages}. Give each its own ID.",
			Duplicate.Id.ToString(), FString::Join(PackageNames, TEXT(", ")));
	}
}
