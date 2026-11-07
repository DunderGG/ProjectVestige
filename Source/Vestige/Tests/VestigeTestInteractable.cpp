// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Tests/VestigeTestInteractable.h"

#if WITH_TESTS

#include "Components/BoxComponent.h"

namespace VestigeTestInteractable
{
	/** Half the size of the box, in cm: a small object, so the distance to it is about the distance to its location. */
	const FVector BoxExtent(10.f);
}

AVestigeTestInteractable::AVestigeTestInteractable()
{
	PrimaryActorTick.bCanEverTick = false;

	InteractionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionVolume"));
	InteractionVolume->SetBoxExtent(VestigeTestInteractable::BoxExtent);
	InteractionVolume->SetCollisionProfileName(VestigeInteraction::CollisionProfile);
	RootComponent = InteractionVolume;
}

void AVestigeTestInteractable::Offer(const FText& Text, FName Action)
{
	FVestigeInteractionOption& Option = OfferedOptions.AddDefaulted_GetRef();
	Option.Text = Text;
	Option.Action = Action;
}

void AVestigeTestInteractable::Offer(const FText& Text, FName Action, const FVector& Location)
{
	Offer(Text, Action);
	OfferedOptions.Last().bHasLocation = true;
	OfferedOptions.Last().Location = Location;
}

void AVestigeTestInteractable::MoveOffersTo(const FVector& Location)
{
	for (FVestigeInteractionOption& Option : OfferedOptions)
	{
		Option.bHasLocation = true;
		Option.Location = Location;
	}
}

void AVestigeTestInteractable::GetInteractionOptions_Implementation(AActor* Interactor, TArray<FVestigeInteractionOption>& OutOptions) const
{
	OutOptions.Append(OfferedOptions);
}

void AVestigeTestInteractable::Interact_Implementation(AActor* Interactor, const FVestigeInteractionOption& Option)
{
	Interactions.Add(Option.Action);
}

#endif
