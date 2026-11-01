// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/OverlapResult.h"
#include "Interaction/VestigeInteractable.h"
#include "VestigeInteractionComponent.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnVestigeInteractionFocusChanged);

/**
 * Owns finding and using interactables for its owner, the player's pawn (Systems.md, section 7). A few times a second it
 * looks for interactables in range on their own collision channel and focuses the nearest one that offers something.
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

	/** Looks for interactables in range and focuses the nearest one with options. Runs every ScanSeconds. */
	void Scan();

	/** The focused interactable, or nullptr when nothing in range offers anything. */
	AActor* GetFocusedActor() const { return FocusedActor.Get(); }

	/** The option interacting would use: the focused interactable's first. Empty while nothing is focused. */
	const FVestigeInteractionOption& GetFocusedOption() const { return FocusedOption; }

	/** Broadcast when the focused interactable, or the option it offers first, changes. */
	FOnVestigeInteractionFocusChanged OnFocusChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Focuses Actor with Option, and tells listeners if either changed. A null Actor clears the focus. */
	void SetFocus(AActor* Actor, const FVestigeInteractionOption& Option);

	/** Seconds between looks for interactables in range. */
	UPROPERTY(EditDefaultsOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true", ClampMin = 0.05))
	float ScanSeconds = 0.2f;

	/** How far from the owner's location an interactable's shape can be and still be used, in cm. One tile. */
	UPROPERTY(EditDefaultsOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true", ClampMin = 0))
	float InteractionRadius = 100.f;

	TWeakObjectPtr<AActor> FocusedActor;
	FVestigeInteractionOption FocusedOption;

	// Kept between scans so a scan reuses their memory instead of allocating.
	TArray<FOverlapResult> Overlaps;
	TArray<FVestigeInteractionOption> Options;

	FTimerHandle ScanTimer;
};
