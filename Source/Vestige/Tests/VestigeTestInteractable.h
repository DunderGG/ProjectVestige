// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

// Only in builds with tests, like UVestigeTestEventRecorder.
#if WITH_TESTS

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/VestigeInteractable.h"
#include "VestigeTestInteractable.generated.h"

class UBoxComponent;

/**
 * An interactable for tests: offers the options a test gives it and writes down each interaction. Its box is on the
 * interactables' collision channel, as a real interactable's shape is.
 */
UCLASS(NotBlueprintable, HideDropdown)
class AVestigeTestInteractable : public AActor, public IVestigeInteractable
{
	GENERATED_BODY()

public:
	AVestigeTestInteractable();

	/** Offers an option with Text and Action to every interactor, after the ones it already offers. */
	void Offer(const FText& Text, FName Action);

	/** Offers an option with Text and Action at Location, after the ones it already offers. */
	void Offer(const FText& Text, FName Action, const FVector& Location);

	/** Moves every option it offers to Location. */
	void MoveOffersTo(const FVector& Location);

	/** The actions of the options it was asked to do, oldest first. */
	const TArray<FName>& GetInteractions() const { return Interactions; }

	virtual void GetInteractionOptions_Implementation(AActor* Interactor, TArray<FVestigeInteractionOption>& OutOptions) const override;
	virtual void Interact_Implementation(AActor* Interactor, const FVestigeInteractionOption& Option) override;

private:
	UPROPERTY()
	TObjectPtr<UBoxComponent> InteractionVolume;

	TArray<FVestigeInteractionOption> OfferedOptions;
	TArray<FName> Interactions;
};

#endif
