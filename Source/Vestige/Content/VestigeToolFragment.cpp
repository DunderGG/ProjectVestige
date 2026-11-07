// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Content/VestigeToolFragment.h"

#include "Content/VestigeToolData.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "VestigeToolFragment"

#if WITH_EDITOR
EDataValidationResult UVestigeToolFragment::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	if (const TOptional<FText> Error = UVestigeContentData::FindContentReferenceError(ToolId, UVestigeToolData::AssetType))
	{
		Context.AddError(FText::Format(LOCTEXT("BadToolId", "The tool fragment's Tool Id {0}"), *Error));
		Result = EDataValidationResult::Invalid;
	}
	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
