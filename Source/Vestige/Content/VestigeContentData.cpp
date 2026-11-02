// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Content/VestigeContentData.h"

#include "Content/VestigeContentLog.h"
#include "Engine/AssetManager.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetData.h"
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "VestigeContentData"

const FName UVestigeContentData::VisualsBundle(TEXT("Visuals"));

const UVestigeContentData* UVestigeContentData::LoadWithVisuals(const FPrimaryAssetId& ContentAssetId)
{
	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	if (!AssetManager || !ContentAssetId.IsValid())
	{
		return nullptr;
	}

	if (const TSharedPtr<FStreamableHandle> Handle = AssetManager->LoadPrimaryAsset(ContentAssetId, {VisualsBundle}))
	{
		Handle->WaitUntilComplete();
	}
	const UVestigeContentData* Content = AssetManager->GetPrimaryAssetObject<UVestigeContentData>(ContentAssetId);
	if (!Content)
	{
		VESTIGE_LOG(AssetManager, LogVestigeContent, Warning,
			"No content asset has the ID {ContentId}. Check the asset's Content Id and folder.", ContentAssetId.ToString());
	}
	return Content;
}

FPrimaryAssetId UVestigeContentData::GetPrimaryAssetId() const
{
	if (ContentId.IsNone())
	{
		return FPrimaryAssetId();
	}
	return FPrimaryAssetId(GetContentType(), ContentId);
}

bool UVestigeContentData::IsWellFormedContentId(FName Id)
{
	FString Namespace;
	FString Name;
	if (Id.IsNone() || !Id.ToString().Split(TEXT("."), &Namespace, &Name))
	{
		return false;
	}

	auto IsPascalCaseWord = [](const FString& Word)
	{
		if (Word.IsEmpty() || !FChar::IsUpper(Word[0]))
		{
			return false;
		}
		for (const TCHAR Character : Word)
		{
			const bool bAsciiLetterOrDigit = FChar::IsAlnum(Character) && Character < 128;
			if (!bAsciiLetterOrDigit)
			{
				return false;
			}
		}
		return true;
	};
	return IsPascalCaseWord(Namespace) && IsPascalCaseWord(Name);
}

#if WITH_EDITOR
EDataValidationResult UVestigeContentData::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	if (!IsWellFormedContentId(ContentId))
	{
		Context.AddError(FText::Format(
			LOCTEXT("BadContentId",
				"Content Id \"{0}\" isn't a content ID. Use a namespace and a name joined by one dot, each starting with a capital letter, "
				"letters and digits only: Base.Turnip."),
			FText::FromName(ContentId)));
		Result = EDataValidationResult::Invalid;
	}
	return Result;
}

TOptional<FText> UVestigeContentData::FindContentReferenceError(const FPrimaryAssetId& Id, const FPrimaryAssetType& ExpectedType)
{
	if (!Id.IsValid())
	{
		return FText::Format(LOCTEXT("ContentReferenceUnset", "is empty. Pick the {0} it names."), FText::FromName(ExpectedType.GetName()));
	}
	if (Id.PrimaryAssetType != ExpectedType)
	{
		return FText::Format(LOCTEXT("ContentReferenceWrongType", "names {0}, which isn't a {1}."), FText::FromString(Id.ToString()),
			FText::FromName(ExpectedType.GetName()));
	}

	FAssetData Asset;
	const UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	if (AssetManager && !AssetManager->GetPrimaryAssetData(Id, Asset))
	{
		return FText::Format(LOCTEXT("ContentReferenceUnknown", "names {0}, which no asset has. Check the ID and that asset's Content Id."),
			FText::FromString(Id.ToString()));
	}
	return {};
}
#endif

#undef LOCTEXT_NAMESPACE
