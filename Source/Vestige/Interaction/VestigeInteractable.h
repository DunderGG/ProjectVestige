// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "UObject/Interface.h"
#include "VestigeInteractable.generated.h"

namespace VestigeInteraction
{
	/**
	 * The object channel only interactables use, named Interactable in Config/DefaultEngine.ini. Nothing else responds to
	 * it, so a query for it finds interactables and nothing else.
	 */
	constexpr ECollisionChannel ObjectChannel = ECC_GameTraceChannel1;

	/** The collision profile of the shape that marks where an interactable can be used from: object type Interactable, queries only. */
	const FName CollisionProfile(TEXT("VestigeInteractable"));
}

/** One thing the player can do with an interactable now, as its prompt shows it. */
USTRUCT(BlueprintType)
struct FVestigeInteractionOption
{
	GENERATED_BODY()

	/** What doing it is called, as the prompt's verb: "Sleep". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FText Text;

	/** A name the interactable chooses to tell its own options apart when one is chosen. The player never sees it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FName Action;

	/**
	 * Whether Location is set: the option is somewhere else than its actor's location (a tile of a large actor). The player
	 * must face that location to be offered it. Without one, the player must face the actor's interaction shape.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	bool bHasLocation = false;

	/** Where in the world the option is, when bHasLocation is set. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (EditCondition = "bHasLocation"))
	FVector Location = FVector::ZeroVector;

	/** Whether Other is the same option with the same text, so a prompt showing this one needn't change. */
	bool IsSameAs(const FVestigeInteractionOption& Other) const;
};

UINTERFACE(Blueprintable)
class UVestigeInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Anything the player can use: a bed, a shipping bin, a water source, a robot (Systems.md, section 7). An actor that
 * implements it also needs a shape with the VestigeInteractable collision profile, which marks where it can be used from.
 * The interactable owns what using it does; the player only asks.
 * Call these through the Execute_ functions (IVestigeInteractable::Execute_Interact), which also reach Blueprint
 * implementations, and test for the interface with Implements<UVestigeInteractable>().
 */
class IVestigeInteractable
{
	GENERATED_BODY()

public:
	/**
	 * Extension point: what Interactor can do with this now, the likeliest first. Until the player can choose, interacting
	 * uses the first option. No options means there is nothing to do, and this isn't offered. Asked five times a second
	 * while the player is near, so it must be cheap and change nothing.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	void GetInteractionOptions(AActor* Interactor, TArray<FVestigeInteractionOption>& OutOptions) const;

	/** Extension point: Interactor does Option, one of the options this offered. */
	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	void Interact(AActor* Interactor, const FVestigeInteractionOption& Option);
};
