// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/VestigeInteractable.h"
#include "FarmShippingBin.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

/**
 * The farm's shipping bin: the place the player puts crops to be sold overnight. It offers "Ship" to anyone with a bag and
 * ships the stack in hand through the farm's shipment (UFarmShippingSubsystem), telling the player how it went. It is not
 * the shipment, which belongs to the subsystem, so a second bin or a robot adds to the same one, and it shows no state.
 * BP_ShippingBin (Content/Vestige/Blueprints) is the one placed in levels and sets the mesh, which this class doesn't have.
 * Systems.md, section 25.
 */
UCLASS(Blueprintable)
class AFarmShippingBin : public AActor, public IVestigeInteractable
{
	GENERATED_BODY()

public:
	AFarmShippingBin();

	/** The action name of the bin's one option. */
	static const FName ShipAction;

	virtual void GetInteractionOptions_Implementation(AActor* Interactor, TArray<FVestigeInteractionOption>& OutOptions) const override;
	virtual void Interact_Implementation(AActor* Interactor, const FVestigeInteractionOption& Option) override;

private:
	/** The bin itself, which the player walks around. No mesh in C++: BP_ShippingBin sets SM_ShippingBin. */
	UPROPERTY(VisibleAnywhere, Category = "Interaction")
	TObjectPtr<UStaticMeshComponent> BinMesh;

	/**
	 * Where the bin can be used from: a box a little bigger than its 90 cm hopper, on the interactables' collision channel. Sized
	 * so the bin can be used facing any side, anywhere along it, but not at a corner, since the aim has four directions
	 * (Systems.md, section 25).
	 */
	UPROPERTY(VisibleAnywhere, Category = "Interaction")
	TObjectPtr<UBoxComponent> InteractionVolume;

	/** What the prompt says. Built once: the bin shows no state, and the scan asks five times a second. */
	FText ShipText;
};
