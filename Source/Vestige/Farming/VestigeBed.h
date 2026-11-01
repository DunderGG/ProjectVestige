// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/VestigeInteractable.h"
#include "VestigeBed.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

/**
 * A bed the player sleeps in. It offers Sleep to anyone with a UVestigeRestComponent and asks that component to sleep, so
 * the rules of sleeping stay with the rest component. BP_Bed (Content/Vestige/Blueprints) is the one placed in levels.
 */
UCLASS(Blueprintable)
class AVestigeBed : public AActor, public IVestigeInteractable
{
	GENERATED_BODY()

public:
	AVestigeBed();

	/** The action name of the bed's one option. */
	static const FName SleepAction;

	virtual void GetInteractionOptions_Implementation(AActor* Interactor, TArray<FVestigeInteractionOption>& OutOptions) const override;
	virtual void Interact_Implementation(AActor* Interactor, const FVestigeInteractionOption& Option) override;

private:
	/** The bed itself, which the player walks around. An engine cube until the bed has its own mesh, which BP_Bed will set. */
	UPROPERTY(VisibleAnywhere, Category = "Interaction")
	TObjectPtr<UStaticMeshComponent> BedMesh;

	/** Where the bed can be used from: a box a little bigger than the bed, on the interactables' collision channel. */
	UPROPERTY(VisibleAnywhere, Category = "Interaction")
	TObjectPtr<UBoxComponent> InteractionVolume;
};
