// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Content/VestigeToolData.h"

#include "Abilities/GameplayAbility.h"
#include "Engine/StaticMesh.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "VestigeToolData"

const FPrimaryAssetType UVestigeToolData::AssetType(TEXT("VestigeTool"));

UStaticMesh* UVestigeToolData::GetHeldMesh() const
{
	return HeldMesh.Get();
}

#if WITH_EDITOR
EDataValidationResult UVestigeToolData::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	if (!Ability)
	{
		Context.AddError(LOCTEXT("NoAbility", "Ability is empty, so the tool does nothing."));
		Result = EDataValidationResult::Invalid;
	}
	else if (Ability->HasAnyClassFlags(CLASS_Abstract))
	{
		Context.AddError(FText::Format(
			LOCTEXT("AbstractAbility", "Ability {0} is abstract and can't be granted."), FText::FromString(Ability->GetName())));
		Result = EDataValidationResult::Invalid;
	}
	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
