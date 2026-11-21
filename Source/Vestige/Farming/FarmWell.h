// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/VestigeInteractable.h"
#include "FarmWell.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

/**
 * The farm's well: the place the player draws clean water from. It offers "Fill can (N left)" to anyone carrying a water
 * container and fills the first can that needs water from the farm's store (UFarmWaterSubsystem), telling the player how it
 * went. It is not the store: the store belongs to the subsystem, so the sprinkler and other sources draw on the same water.
 * BP_Well (Content/Vestige/Blueprints) is the one placed in levels and sets the mesh, which this class doesn't have.
 * Systems.md, section 24.
 */
UCLASS(Blueprintable)
class AFarmWell : public AActor, public IVestigeInteractable
{
	GENERATED_BODY()

public:
	AFarmWell();

	/** The action name of the well's one option. */
	static const FName FillAction;

	virtual void GetInteractionOptions_Implementation(AActor* Interactor, TArray<FVestigeInteractionOption>& OutOptions) const override;
	virtual void Interact_Implementation(AActor* Interactor, const FVestigeInteractionOption& Option) override;

private:
	/** The well itself, which the player walks around. No mesh in C++: BP_Well sets SM_Well. */
	UPROPERTY(VisibleAnywhere, Category = "Interaction")
	TObjectPtr<UStaticMeshComponent> WellMesh;

	/**
	 * Where the well can be used from: a box a little bigger than its ring, on the interactables' collision channel. Sized so
	 * the farmer standing against the ring, who can't climb it, always has the box in front of him (Systems.md, section 24).
	 */
	UPROPERTY(VisibleAnywhere, Category = "Interaction")
	TObjectPtr<UBoxComponent> InteractionVolume;

	/** What the prompt says for Left waterings left, rebuilt only when Left changes since the scan asks five times a second. */
	const FText& GetFillText(int32 Left) const;

	mutable int32 CachedLeft = INDEX_NONE;
	mutable FText CachedFillText;
};
