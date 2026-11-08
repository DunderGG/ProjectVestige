// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Content/VestigeContentCatalog.h"

#include "Content/VestigeContentData.h"
#include "Content/VestigeContentLog.h"
#include "Engine/AssetManager.h"
#include "Engine/GameInstance.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

UVestigeContentCatalog* UVestigeContentCatalog::Get(const UWorld* World)
{
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UVestigeContentCatalog>() : nullptr;
}

TArray<FPrimaryAssetType> UVestigeContentCatalog::FindContentTypes()
{
	TArray<FPrimaryAssetTypeInfo> TypeInfos;
	UAssetManager::Get().GetPrimaryAssetTypeInfoList(TypeInfos);

	TArray<FPrimaryAssetType> Types;
	for (const FPrimaryAssetTypeInfo& Info : TypeInfos)
	{
		const UClass* BaseClass = Info.AssetBaseClassLoaded;
		if (BaseClass && BaseClass->IsChildOf(UVestigeContentData::StaticClass()) && !Info.bHasBlueprintClasses && !Info.bIsEditorOnly)
		{
			Types.Add(Info.PrimaryAssetType);
		}
	}
	return Types;
}

void UVestigeContentCatalog::Initialize(FSubsystemCollectionBase& Collection)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UVestigeContentCatalog::Initialize);

	Super::Initialize(Collection);

	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	if (!AssetManager)
	{
		VESTIGE_LOG(this, LogVestigeContent, Error, "There is no Asset Manager, so the game has no content.");
		return;
	}
	if (!AssetManager->HasInitialScanCompleted())
	{
		VESTIGE_LOG(this, LogVestigeContent, Error, "The Asset Manager hasn't finished scanning, so content may be missing.");
	}

	TArray<FPrimaryAssetId> Ids;
	for (const FPrimaryAssetType& Type : FindContentTypes())
	{
		TArray<FPrimaryAssetId> TypeIds;
		AssetManager->GetPrimaryAssetIdList(Type, TypeIds);
		Ids.Append(TypeIds);
	}
	if (Ids.IsEmpty())
	{
		// A preload of nothing returns no handle and fails an ensure.
		VESTIGE_LOG(this, LogVestigeContent, Warning, "The Asset Manager knows no content.");
		return;
	}

	// No world plays yet, so waiting is allowed here, and once loaded nothing waits on content during play.
	LoadHandle = AssetManager->PreloadPrimaryAssets(Ids, {UVestigeContentData::VisualsBundle}, false);
	if (LoadHandle)
	{
		LoadHandle->WaitUntilComplete();
	}

	Content.Reserve(Ids.Num());
	for (const FPrimaryAssetId& Id : Ids)
	{
		if (const UVestigeContentData* Loaded = AssetManager->GetPrimaryAssetObject<UVestigeContentData>(Id))
		{
			Content.Add(Id, Loaded);
		}
		else
		{
			VESTIGE_LOG(this, LogVestigeContent, Warning, "The content {ContentId} didn't load.", Id.ToString());
		}
	}
	VESTIGE_LOG(this, LogVestigeContent, Log, "Loaded {Count} content definitions.", Content.Num());
}

void UVestigeContentCatalog::Deinitialize()
{
	// Released, not only reset, so the load ends even while a copy of the handle lives.
	if (LoadHandle)
	{
		LoadHandle->ReleaseHandle();
		LoadHandle.Reset();
	}
	Content.Reset();

	Super::Deinitialize();
}

const UVestigeContentData* UVestigeContentCatalog::Find(const FPrimaryAssetId& Id) const
{
	const TObjectPtr<const UVestigeContentData>* Found = Content.Find(Id);
	return Found ? Found->Get() : nullptr;
}

#if WITH_DEV_AUTOMATION_TESTS
bool UVestigeContentCatalog::AddForTests(const UVestigeContentData& InContent)
{
	const FPrimaryAssetId Id = InContent.GetPrimaryAssetId();
	if (!Id.IsValid())
	{
		return false;
	}
	Content.Add(Id, &InContent);
	return true;
}
#endif
