// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Content/FarmCropData.h"

#include "Content/VestigeItemData.h"
#include "Engine/StaticMesh.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "FarmCropData"

const FPrimaryAssetType UFarmCropData::AssetType(TEXT("FarmCrop"));

UStaticMesh* UFarmCropData::GetGrowthStageMesh(int32 GrowthDays) const
{
	if (GrowthStageMeshes.IsEmpty())
	{
		return nullptr;
	}
	return GrowthStageMeshes[FMath::Clamp(GrowthDays, 0, GrowthStageMeshes.Num() - 1)].Get();
}

#if WITH_EDITOR
EDataValidationResult UFarmCropData::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	if (const TOptional<FText> Error = FindContentReferenceError(HarvestItemId, UVestigeItemData::AssetType))
	{
		Context.AddError(FText::Format(LOCTEXT("BadHarvestItemId", "Harvest Item Id {0}"), *Error));
		Result = EDataValidationResult::Invalid;
	}
	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
