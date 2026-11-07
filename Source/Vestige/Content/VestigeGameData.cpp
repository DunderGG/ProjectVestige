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
		const UGameplayEffect* Drain = GetDefault<UGameplayEffect>(LateNightDrainEffect);
		const bool bInfiniteAndPeriodic = Drain->DurationPolicy == EGameplayEffectDurationType::Infinite
			&& Drain->Period.GetValueAtLevel(1.f) > UGameplayEffect::NO_PERIOD;
		if (!bInfiniteAndPeriodic)
		{
			Context.AddError(FText::Format(LOCTEXT("DrainNotPeriodic",
											   "Late Night Drain Effect {0} must be infinite with a period above 0: the rest component "
											   "drains with it until the day ends."),
				FText::FromString(LateNightDrainEffect->GetName())));
			Result = EDataValidationResult::Invalid;
		}
	}
	return Result;
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
#endif

#undef LOCTEXT_NAMESPACE
