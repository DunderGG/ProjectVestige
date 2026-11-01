// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "AbilitySystem/VestigeAbilitySet.h"

#include "Abilities/GameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "Core/Vestige.h"
#include "GameFramework/Actor.h"

void UVestigeAbilitySet::GiveTo(UAbilitySystemComponent& AbilitySystem) const
{
	AActor* Owner = AbilitySystem.GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		UE_LOG(LogVestige, Warning, TEXT("%s can't be given to %s: only the authority over its owner can grant abilities."), *GetName(),
			*AbilitySystem.GetPathName());
		return;
	}

	for (const TSubclassOf<UAttributeSet>& SetClass : AttributeSets)
	{
		if (SetClass && !AbilitySystem.GetAttributeSet(SetClass))
		{
			// Attribute sets are subobjects of the ability system's owner, as the ones it finds at creation are.
			AbilitySystem.AddAttributeSetSubobject(NewObject<UAttributeSet>(Owner, SetClass));
		}
	}
	for (const TSubclassOf<UGameplayAbility>& AbilityClass : Abilities)
	{
		if (AbilityClass)
		{
			AbilitySystem.GiveAbility(FGameplayAbilitySpec(AbilityClass));
		}
	}
}
