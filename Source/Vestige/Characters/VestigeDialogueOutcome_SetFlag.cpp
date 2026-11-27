// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Characters/VestigeDialogueOutcome_SetFlag.h"

#include "Characters/VestigeRelationshipComponent.h"
#include "Content/VestigeCharacterData.h"
#include "GameFramework/PlayerState.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "VestigeDialogueOutcomeSetFlag"

void UVestigeDialogueOutcome_SetFlag::Apply_Implementation(const FVestigeDialogueContext& Context) const
{
	UVestigeRelationshipComponent* Relationships = UVestigeRelationshipComponent::Find(Context.Player);
	if (Relationships && Context.Speaker)
	{
		Relationships->SetFlag(*Context.Speaker, Flag, Context.Day);
	}
}

#if WITH_EDITOR
EDataValidationResult UVestigeDialogueOutcome_SetFlag::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	if (!Flag.IsValid())
	{
		Context.AddError(LOCTEXT("NoFlag", "The Set Flag outcome has no valid Flag tag."));
		Result = EDataValidationResult::Invalid;
	}
	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
