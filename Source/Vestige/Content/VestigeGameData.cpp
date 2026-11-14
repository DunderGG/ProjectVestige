// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Content/VestigeGameData.h"

#include "Content/VestigeAssetManager.h"
#include "GameplayEffect.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "VestigeGameData"

const UVestigeGameData& UVestigeGameData::Get()
{
	if (const UVestigeGameData* Loaded = UVestigeAssetManager::GetGameData())
	{
		return *Loaded;
	}
	return *GetDefault<UVestigeGameData>();
}

#if WITH_EDITOR
EDataValidationResult UVestigeGameData::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	const TPair<FText, TSubclassOf<UGameplayEffect>> Effects[] = {
		{LOCTEXT("StaminaCost", "Stamina Cost Effect"), StaminaCostEffect},
		{LOCTEXT("StaminaRefill", "Stamina Refill Effect"), StaminaRefillEffect},
		{LOCTEXT("LateNightDrain", "Late Night Drain Effect"), LateNightDrainEffect},
		{LOCTEXT("Exhausted", "Exhausted Effect"), ExhaustedEffect},
	};
	for (const TPair<FText, TSubclassOf<UGameplayEffect>>& Effect : Effects)
	{
		if (!Effect.Value)
		{
			Context.AddError(FText::Format(LOCTEXT("NoEffect", "{0} is empty, so nothing applies it."), Effect.Key));
			Result = EDataValidationResult::Invalid;
		}
	}

	if (LateNightDrainEffect)
	{
		if (LateNightDrainEffect.GetDefaultObject()->DurationPolicy != EGameplayEffectDurationType::Instant)
		{
			Context.AddError(FText::Format(LOCTEXT("DrainNotInstant",
											   "Late Night Drain Effect {0} must be instant: the rest component applies it once every "
											   "game minute of the late night."),
				FText::FromString(LateNightDrainEffect->GetName())));
			Result = EDataValidationResult::Invalid;
		}
	}
	return CombineDataValidationResults(Result, Calendar.Validate(Context));
}
#endif

#if WITH_DEV_AUTOMATION_TESTS
void UVestigeGameData::SetEffectsForTests(TSubclassOf<UGameplayEffect> InStaminaCost, TSubclassOf<UGameplayEffect> InStaminaRefill,
	TSubclassOf<UGameplayEffect> InLateNightDrain, TSubclassOf<UGameplayEffect> InExhausted)
{
	StaminaCostEffect = InStaminaCost;
	StaminaRefillEffect = InStaminaRefill;
	LateNightDrainEffect = InLateNightDrain;
	ExhaustedEffect = InExhausted;
}

void UVestigeGameData::SetCalendarForTests(const FVestigeCalendar& InCalendar)
{
	Calendar = InCalendar;
}
#endif

#undef LOCTEXT_NAMESPACE
