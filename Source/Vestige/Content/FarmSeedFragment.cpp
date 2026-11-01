// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Content/FarmSeedFragment.h"

#include "Content/FarmCropData.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "FarmSeedFragment"

#if WITH_EDITOR
EDataValidationResult UFarmSeedFragment::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	if (const TOptional<FText> Error = UVestigeContentData::FindContentReferenceError(CropId, UFarmCropData::AssetType))
	{
		Context.AddError(FText::Format(LOCTEXT("BadCropId", "The seed fragment's Crop Id {0}"), *Error));
		Result = EDataValidationResult::Invalid;
	}
	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
