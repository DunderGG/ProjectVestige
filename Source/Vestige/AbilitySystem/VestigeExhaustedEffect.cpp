// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "AbilitySystem/VestigeExhaustedEffect.h"

#include "AbilitySystem/VestigeGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UVestigeExhaustedEffect::UVestigeExhaustedEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	// A native effect's components are default subobjects listed in GEComponents (UGameplayEffect::PostInitProperties
	// checks this). Natives get no PostLoad, so the granted tag is applied to the effect here.
	UTargetTagsGameplayEffectComponent* GrantedTags = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("GrantedTags"));
	GEComponents.Add(GrantedTags);
	FInheritedTagContainer Tags;
	Tags.AddTag(VestigeGameplayTags::State_Exhausted);
	GrantedTags->SetAndApplyTargetTagChanges(Tags);
}
