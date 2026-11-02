// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "UI/VestigeHudMessages.h"
#include "VestigeHUD.generated.h"

class AVestigeDayCycle;
class AVestigePlayerCharacter;
class APawn;
class SVestigeHudWidget;
class UAbilitySystemComponent;
class UFarmingComponent;
class UVestigeToolData;
class UVestigeInteractionComponent;
class UVestigeInventoryComponent;
struct FGameplayTag;

/**
 * Shows the farming HUD for its player: the day and time from the world's AVestigeDayCycle, the tool from the pawn's
 * UFarmingComponent, the items from the inventory and the stamina from the ability system on the player state, the
 * farmer's key hints, the prompt for the interactable the pawn's UVestigeInteractionComponent focuses, and the feedback
 * messages UVestigeFeedbackSubsystem carries.
 * It only displays. It builds each text when an event says what it shows has changed, so the widget's per-frame
 * bindings only read finished text; the messages are the one thing it keeps, and only until they fade.
 */
UCLASS()
class AVestigeHUD : public AHUD
{
	GENERATED_BODY()

public:
	/**
	 * "Day 3   18:40   |   Hoe". Read every frame for the clock, but only rebuilt when the day, the shown time or
	 * the tool changes.
	 */
	FText GetStatusText() const;

	/** "Stamina 85 / 100", with "Exhausted" after it while exhausted. Rebuilt when the stamina, its maximum or exhaustion changes. */
	const FText& GetStaminaText() const { return StaminaText; }

	/** Whether the player is out of stamina, so the stamina line warns. Follows Vestige.State.OutOfStamina. */
	bool IsOutOfStamina() const { return bOutOfStamina; }

	/** "Bag: Turnip Seeds 9   Turnip 2", the non-empty slots in order. Rebuilt when the inventory changes. */
	const FText& GetInventoryText() const { return InventoryText; }

	/** "F Sleep": the interact key and what it would do while an interactable is focused, else empty. Rebuilt when the focus changes. */
	const FText& GetInteractionText() const { return InteractionText; }

	/** The key hints, built from the farmer's input mappings. Rebuilt when the toolbar or the pawn changes. */
	const FText& GetControlsText() const { return ControlsText; }

	/** The feedback messages showing now, oldest first, each in its own box. Changes only when a message arrives or expires. */
	const FVestigeHudMessages& GetMessages() const { return Messages; }

	/** How visible the message at Index is now, from 0 to 1, as it fades in and out. Cheap enough to read every frame. */
	float GetMessageOpacity(int32 Index) const;

	/** Most messages on screen at once, each in its own box, from UVestigeHudSettings at BeginPlay. */
	int32 GetMaxMessages() const { return MaxMessages; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Follows the new pawn's farming and interaction components and key hints. Bound to the controller's OnPossessedPawnChanged. */
	UFUNCTION()
	void FollowPawn(APawn* OldPawn, APawn* NewPawn);

	void StopFollowingPawn();

	/** Rebuilds the tool's name and the key hints, which name the toolbar's tools. Bound to OnToolChanged. */
	void RefreshToolTexts(const UVestigeToolData* Tool);

	/** Bound to the interaction component's OnFocusChanged. */
	void RefreshInteractionText();

	/** Follows the inventory on the player state, which outlives the pawn, so this happens once, at BeginPlay. */
	void FollowInventory();

	/** Bound to the inventory's OnInventoryChanged. */
	void RefreshInventoryText();

	/** Follows the stamina on the player state's ability system, which outlives the pawn, so this happens once, at BeginPlay. */
	void FollowAbilitySystem();

	/** Bound to the change delegates of Stamina and MaxStamina, and to the tag event of Vestige.State.Exhausted. */
	void RefreshStaminaText();

	/** Bound to the ability system's event for Vestige.State.OutOfStamina being added or removed. */
	void RefreshOutOfStamina(const FGameplayTag Tag, int32 NewCount);

	void RefreshControlsText();

	/** Shows Message in a box of its own, under the ones already showing, for MessageSeconds. Bound to UVestigeFeedbackSubsystem. */
	void ShowMessage(const FText& Message);

	/** Drops the messages whose time is up, and waits for the next one to expire. */
	void RemoveExpiredMessages();

	/** Tells the widget the list changed, and sets the timer for the next message to expire. */
	void MessagesChanged();

	// Read from UVestigeHudSettings at BeginPlay and kept for the session, so the widget's boxes, made once, always match.
	float MessageSeconds = 0.f;
	int32 MaxMessages = 0;

	TWeakObjectPtr<const AVestigeDayCycle> DayCycle;
	TWeakObjectPtr<UFarmingComponent> Farming;
	TWeakObjectPtr<UVestigeInteractionComponent> Interaction;
	TWeakObjectPtr<UVestigeInventoryComponent> Inventory;
	TWeakObjectPtr<UAbilitySystemComponent> AbilitySystem;
	TWeakObjectPtr<const AVestigePlayerCharacter> Farmer;

	FText ToolText;
	FText StaminaText;
	FText InventoryText;
	FText ControlsText;
	FText InteractionText;
	bool bOutOfStamina = false;

	// The status line as last built, and the day and minute it shows. GetStatusText is a const per-frame getter that
	// rebuilds them only when they change.
	mutable FText StatusText;
	mutable int32 StatusDay = INDEX_NONE;
	mutable int32 StatusClockMinutes = INDEX_NONE;

	FVestigeHudMessages Messages;
	FTimerHandle ExpireTimer;

	TSharedPtr<SVestigeHudWidget> HudWidget;
};
