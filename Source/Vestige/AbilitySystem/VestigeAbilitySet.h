// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "VestigeAbilitySet.generated.h"

class UAbilitySystemComponent;
class UAttributeSet;
class UGameplayAbility;

/**
 * Attribute sets and abilities granted together, so who has which abilities is data rather than code: the player's set is
 * DA_AbilitySet_Player, named on BP_PlayerState. Later a tool, item or robot type grants its abilities the same way.
 */
UCLASS(Const)
class UVestigeAbilitySet : public UDataAsset
{
	GENERATED_BODY()

public:
	/**
	 * Gives AbilitySystem this set's attribute sets and abilities. Needs authority over AbilitySystem's owner, which a
	 * single-player game always has. An attribute set AbilitySystem already has is kept, with its values.
	 */
	void GiveTo(UAbilitySystemComponent& AbilitySystem) const;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Abilities", meta = (AllowPrivateAccess = "true"))
	TArray<TSubclassOf<UAttributeSet>> AttributeSets;

	UPROPERTY(EditDefaultsOnly, Category = "Abilities", meta = (AllowPrivateAccess = "true"))
	TArray<TSubclassOf<UGameplayAbility>> Abilities;
};
