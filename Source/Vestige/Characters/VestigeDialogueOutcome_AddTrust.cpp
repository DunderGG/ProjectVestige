// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Characters/VestigeDialogueOutcome_AddTrust.h"

#include "Characters/VestigeRelationshipComponent.h"
#include "Content/VestigeCharacterData.h"
#include "GameFramework/PlayerState.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "VestigeDialogueOutcomeAddTrust"

void UVestigeDialogueOutcome_AddTrust::Apply_Implementation(const FVestigeDialogueContext& Context) const
{
	UVestigeRelationshipComponent* Relationships = UVestigeRelationshipComponent::Find(Context.Player);
	if (Relationships && Context.Speaker)
	{
		Relationships->AddTrust(*Context.Speaker, Amount);
	}
}

#if WITH_EDITOR
EDataValidationResult UVestigeDialogueOutcome_AddTrust::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	if (Amount < 1)
	{
		Context.AddError(LOCTEXT("BadAmount", "The Add Trust outcome's Amount is below 1."));
		Result = EDataValidationResult::Invalid;
	}
	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
