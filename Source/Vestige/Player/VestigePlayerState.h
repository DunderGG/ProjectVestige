// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "VestigePlayerState.generated.h"

class UAbilitySystemComponent;
class UVestigeAbilitySet;
class UVestigeInventoryComponent;

/**
 * The player's data that outlives the pawn: the inventory, and the ability system with its abilities and stamina.
 * BP_PlayerState sets its values (the starting items, the ability sets), and the game mode names it. Code below the
 * player layer doesn't know this class: it finds the inventory with FindComponentByClass on the player's APlayerState,
 * and the ability system through the pawn's IAbilitySystemInterface.
 * Registers the ability system's saved attribute sets (the stamina) with UVestigeSaveRegistry while it is in play, since
 * the sets have no BeginPlay or EndPlay of their own; the inventory registers itself.
 */
UCLASS()
class AVestigePlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AVestigePlayerState();

	UVestigeInventoryComponent* GetInventory() const { return Inventory; }

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystem; }

protected:
	/** Grants the ability sets, so the stamina set exists before any actor begins play and reads it. */
	virtual void PostInitializeComponents() override;

	/** Hands each saved attribute set its part of the save. */
	virtual void BeginPlay() override;

	/** Keeps each saved attribute set's part for the next map. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Makes the new pawn the ability system's avatar, the body its abilities act through. Bound to OnPawnSet. */
	UFUNCTION()
	void FollowPawn(APlayerState* Player, APawn* NewPawn, APawn* OldPawn);

	UPROPERTY(VisibleAnywhere, Category = "Inventory", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UVestigeInventoryComponent> Inventory;

	UPROPERTY(VisibleAnywhere, Category = "Abilities", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAbilitySystemComponent> AbilitySystem;

	/** What the player can do and the attributes they have from the start (DA_AbilitySet_Player). */
	UPROPERTY(EditDefaultsOnly, Category = "Abilities", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<const UVestigeAbilitySet>> AbilitySets;
};
