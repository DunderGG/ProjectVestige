// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/OverlapResult.h"
#include "Interaction/VestigeInteractable.h"
#include "VestigeInteractionComponent.generated.h"

class UPrimitiveComponent;

DECLARE_MULTICAST_DELEGATE(FOnVestigeInteractionFocusChanged);
DECLARE_DELEGATE_RetVal(FVector, FVestigeFacingDirection);

/**
 * Owns finding and using interactables for its owner, the player's pawn (Systems.md, section 7). A few times a second it
 * looks for interactables in range on their own collision channel and focuses the nearest one the owner faces that offers
 * something: an option with a location within FrontHalfAngleDegrees of the facing direction, or an option without one
 * whose interaction shape the facing ray hits.
 * Interacting goes through the interact ability, so the ability system's rules apply to it as they do to using a tool.
 */
UCLASS(ClassGroup = (Interaction), meta = (BlueprintSpawnableComponent))
class UVestigeInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UVestigeInteractionComponent();

	/** Activates the abilities tagged Vestige.Ability.Interact on the owner's ability system, which use the focus. */
	void Interact();

	/**
	 * Uses the focused interactable's first option, after looking again so the focus is current. Called by the interact
	 * ability. Returns false when nothing in range offers anything.
	 */
	bool InteractWithFocus();

	/** Looks for interactables in range and focuses the nearest faced one with options. Runs every ScanSeconds. */
	void Scan();

	/** The focused interactable, or nullptr when nothing in range offers anything. */
	AActor* GetFocusedActor() const { return FocusedActor.Get(); }

	/** The option interacting would use: the focused interactable's first. Empty while nothing is focused. */
	const FVestigeInteractionOption& GetFocusedOption() const { return FocusedOption; }

	/** Broadcast when the focused interactable, or the action or text of the option it offers first, changes. */
	FOnVestigeInteractionFocusChanged OnFocusChanged;

	/**
	 * The direction the owner faces on the ground, for choosing what it means to use. Bound by the owner (the farmer binds its
	 * aim); unbound or zero, the owner's forward vector is used.
	 */
	FVestigeFacingDirection FacingDirection;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/**
	 * Focuses Actor with Option, keeping the latest option, and tells listeners if the actor or the option's action or text
	 * changed. A null Actor clears the focus.
	 */
	void SetFocus(AActor* Actor, const FVestigeInteractionOption& Option);

	/** Seconds between looks for interactables in range. */
	UPROPERTY(EditDefaultsOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true", ClampMin = 0.05))
	float ScanSeconds = 0.2f;

	/** How far from the owner's location an interactable's shape can be and still be used, in cm. One tile. */
	UPROPERTY(EditDefaultsOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true", ClampMin = 0))
	float InteractionRadius = 100.f;

	/** The facing direction on the ground plane, of length 1, or zero if there is none. */
	FVector GetFacing() const;

	/**
	 * Whether the owner at Origin, facing Facing, faces Option of the interactable whose shape is Shape, and how far away it
	 * is: to the option's location, or to where the facing ray hits the shape.
	 */
	bool IsFaced(const FVestigeInteractionOption& Option, UPrimitiveComponent* Shape, const FVector& Origin, const FVector& Facing,
		double& OutDistanceSquared) const;

	/** How far off the facing direction an option's location can be and still be faced, in degrees. */
	UPROPERTY(EditDefaultsOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true", ClampMin = 0, ClampMax = 90))
	float FrontHalfAngleDegrees = 50.f;

	TWeakObjectPtr<AActor> FocusedActor;
	FVestigeInteractionOption FocusedOption;

	// Kept between scans so a scan reuses their memory instead of allocating.
	TArray<FOverlapResult> Overlaps;
	TArray<FVestigeInteractionOption> Options;

	FTimerHandle ScanTimer;
};
