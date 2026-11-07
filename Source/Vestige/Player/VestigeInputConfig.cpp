// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Player/VestigeInputConfig.h"

#include "AbilitySystem/VestigeGameplayTags.h"
#include "InputAction.h"
#include "Player/VestigeInputLog.h"
#include "PlayerMappableKeySettings.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "VestigeInputConfig"

const UInputAction* UVestigeInputConfig::FindNativeInputActionForTag(const FGameplayTag& Tag, bool bLogNotFound) const
{
	// A linear search: the config has about fifteen entries, and it is asked once per possession and when hints are rebuilt.
	for (const FVestigeInputAction& Entry : NativeInputActions)
	{
		if (Entry.InputAction && Entry.InputTag == Tag)
		{
			return Entry.InputAction;
		}
	}
	if (bLogNotFound)
	{
		VESTIGE_LOG(this, LogVestigeInput, Error, "No input action for the input tag {Tag}, so nothing is bound to it.", Tag.ToString());
	}
	return nullptr;
}

#if WITH_EDITOR
EDataValidationResult UVestigeInputConfig::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	const auto AddError = [&Context, &Result](const FText& Error)
	{
		Context.AddError(Error);
		Result = EDataValidationResult::Invalid;
	};

	TSet<FGameplayTag> Tags;
	TSet<const UInputAction*> Actions;
	for (int32 Index = 0; Index < NativeInputActions.Num(); ++Index)
	{
		const FVestigeInputAction& Entry = NativeInputActions[Index];
		if (!Entry.InputTag.IsValid() || !Entry.InputTag.MatchesTag(VestigeGameplayTags::Input))
		{
			AddError(FText::Format(LOCTEXT("BadTag", "Entry {0} needs an input tag under Vestige.Input, not '{1}'."), Index,
				FText::FromString(Entry.InputTag.ToString())));
		}
		else if (Tags.Contains(Entry.InputTag))
		{
			AddError(FText::Format(LOCTEXT("TagTwice", "The input tag {0} is listed twice; only the first is bound."),
				FText::FromString(Entry.InputTag.ToString())));
		}
		Tags.Add(Entry.InputTag);

		const UInputAction* Action = Entry.InputAction;
		if (!Action)
		{
			AddError(FText::Format(LOCTEXT("NoAction", "Entry {0} has no input action."), Index));
			continue;
		}
		if (Actions.Contains(Action))
		{
			AddError(FText::Format(LOCTEXT("ActionTwice", "The input action {0} is listed twice."), FText::FromString(Action->GetName())));
		}
		Actions.Add(Action);

		// Every bound call is a press or a release, which only a digital action means.
		if (Action->ValueType != EInputActionValueType::Boolean)
		{
			AddError(FText::Format(
				LOCTEXT("NotBoolean", "The input action {0} must have a Boolean value type."), FText::FromString(Action->GetName())));
		}

		// The name is the key a saved remap is stored under, so an unnamed one could never be rebound.
		const UPlayerMappableKeySettings* MappableSettings = Action->GetPlayerMappableKeySettings();
		if (MappableSettings && MappableSettings->Name.IsNone())
		{
			AddError(FText::Format(LOCTEXT("NoMappableName", "The input action {0} has player-mappable key settings without a name."),
				FText::FromString(Action->GetName())));
		}
	}
	return Result;
}
#endif

#if WITH_DEV_AUTOMATION_TESTS
void UVestigeInputConfig::SetNativeInputActionsForTests(TArray<FVestigeInputAction> InNativeInputActions)
{
	NativeInputActions = MoveTemp(InNativeInputActions);
}
#endif

#undef LOCTEXT_NAMESPACE
