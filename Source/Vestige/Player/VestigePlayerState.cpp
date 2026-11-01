// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Player/VestigePlayerState.h"

#include "AbilitySystem/VestigeAbilitySet.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "Inventory/VestigeInventoryComponent.h"

AVestigePlayerState::AVestigePlayerState()
{
	Inventory = CreateDefaultSubobject<UVestigeInventoryComponent>(TEXT("Inventory"));

	// Single-player: nothing is predicted or sent to other machines, so the leanest replication mode is enough.
	AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
	AbilitySystem->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
}

void AVestigePlayerState::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	for (const UVestigeAbilitySet* AbilitySet : AbilitySets)
	{
		if (AbilitySet)
		{
			AbilitySet->GiveTo(*AbilitySystem);
		}
	}
	OnPawnSet.AddDynamic(this, &AVestigePlayerState::FollowPawn);
}

void AVestigePlayerState::FollowPawn(APlayerState* Player, APawn* NewPawn, APawn* OldPawn)
{
	// An ability still running acts through the old body, so it ends before the body changes.
	if (OldPawn)
	{
		AbilitySystem->CancelAllAbilities();
	}
	AbilitySystem->InitAbilityActorInfo(this, NewPawn);
}
