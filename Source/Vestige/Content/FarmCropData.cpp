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

int32 UFarmCropData::GetGrowthStageIndex(int32 GrowthDays) const
{
	const int32 Days = FMath::Max(GrowthDays, 0);
	if (IsRipeAt(Days) || GrowthStageMeshes.IsEmpty())
	{
		return INDEX_NONE;
	}
	return Days * GrowthStageMeshes.Num() / DaysToMature;
}

FFarmCropGrowthLook UFarmCropData::GetGrowthLook(int32 GrowthDays) const
{
	const int32 Days = FMath::Max(GrowthDays, 0);
	if (IsRipeAt(Days))
	{
		return {RipeMesh.Get(), 1.f};
	}

	const int32 StageIndex = GetGrowthStageIndex(Days);
	if (StageIndex == INDEX_NONE)
	{
		return {};
	}

	// Stage k shows on the days d with d * N / D == k, which run from ceil(k * D / N) to ceil((k + 1) * D / N) - 1.
	const int32 NumStages = GrowthStageMeshes.Num();
	const int32 FirstDay = FMath::DivideAndRoundUp(StageIndex * DaysToMature, NumStages);
	const int32 LastDay = FMath::DivideAndRoundUp((StageIndex + 1) * DaysToMature, NumStages) - 1;
	const float Scale = LastDay == FirstDay
		? 1.f
		: FMath::Lerp(StageStartScale, 1.f, static_cast<float>(Days - FirstDay) / static_cast<float>(LastDay - FirstDay));
	return {GrowthStageMeshes[StageIndex].Get(), Scale};
}

#if WITH_EDITOR
EDataValidationResult UFarmCropData::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	auto ReportError = [&Context, &Result](const FText& Error)
	{
		Context.AddError(Error);
		Result = EDataValidationResult::Invalid;
	};

	if (const TOptional<FText> Error = FindContentReferenceError(HarvestItemId, UVestigeItemData::AssetType))
	{
		ReportError(FText::Format(LOCTEXT("BadHarvestItemId", "Harvest Item Id {0}"), *Error));
	}
	if (DaysToMature < 1)
	{
		ReportError(FText::Format(LOCTEXT("BadDaysToMature", "Days To Mature is {0}. It must be at least 1."), DaysToMature));
	}
	if (StageStartScale <= 0.f || StageStartScale > 1.f)
	{
		ReportError(
			FText::Format(LOCTEXT("BadStageStartScale", "Stage Start Scale is {0}. It must be above 0 and at most 1."), StageStartScale));
	}
	if (RipeMesh.IsNull())
	{
		ReportError(LOCTEXT("NoRipeMesh", "Ripe Mesh is empty, so the ripe crop draws the placeholder. Pick the crop's ripe mesh."));
	}
	if (GrowthStageMeshes.IsEmpty())
	{
		ReportError(LOCTEXT("NoGrowthStageMeshes",
			"Growth Stage Meshes is empty, so the growing crop draws the placeholder. Add the crop's stage meshes, seedling first."));
	}
	for (int32 Index = 0; Index < GrowthStageMeshes.Num(); ++Index)
	{
		if (GrowthStageMeshes[Index].IsNull())
		{
			ReportError(FText::Format(
				LOCTEXT("EmptyGrowthStageMesh", "Growth Stage Meshes has an empty entry at index {0}. Pick its mesh or remove it."),
				Index));
		}
	}
	if (!RipeMesh.IsNull() && GrowthStageMeshes.Contains(RipeMesh))
	{
		ReportError(LOCTEXT("RipeMeshAmongStages",
			"Growth Stage Meshes has the Ripe Mesh in it, so it shows before the crop is ripe. Remove it from the stages."));
	}
	if (GrowthStageMeshes.Num() > DaysToMature)
	{
		Context.AddWarning(FText::Format(
			LOCTEXT("MoreStagesThanDays", "Growth Stage Meshes has {0} meshes for {1} Days To Mature, so some stages then never show."),
			GrowthStageMeshes.Num(), DaysToMature));
	}
	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
