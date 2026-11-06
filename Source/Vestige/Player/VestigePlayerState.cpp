// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Player/VestigePlayerState.h"

#include "AbilitySystem/VestigeAbilitySet.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "Core/VestigeSaveRegistry.h"
#include "Core/VestigeSaveable.h"
#include "Farming/VestigeRestComponent.h"
#include "GameFramework/Pawn.h"
#include "Inventory/VestigeInventoryComponent.h"

AVestigePlayerState::AVestigePlayerState()
{
	Inventory = CreateDefaultSubobject<UVestigeInventoryComponent>(TEXT("Inventory"));

	// Single-player: nothing is predicted or sent to other machines, so the leanest replication mode is enough.
	AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
	AbilitySystem->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	Rest = CreateDefaultSubobject<UVestigeRestComponent>(TEXT("Rest"));
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

void AVestigePlayerState::BeginPlay()
{
	Super::BeginPlay();

	for (UAttributeSet* AttributeSet : AbilitySystem->GetSpawnedAttributes())
	{
		if (IVestigeSaveable* Saveable = Cast<IVestigeSaveable>(AttributeSet))
		{
			UVestigeSaveRegistry::RegisterWorldOwner(GetWorld(), *Saveable);
		}
	}
}

void AVestigePlayerState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (UAttributeSet* AttributeSet : AbilitySystem->GetSpawnedAttributes())
	{
		if (IVestigeSaveable* Saveable = Cast<IVestigeSaveable>(AttributeSet))
		{
			UVestigeSaveRegistry::UnregisterWorldOwner(GetWorld(), *Saveable);
		}
	}
	Super::EndPlay(EndPlayReason);
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
