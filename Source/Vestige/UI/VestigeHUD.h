// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Player/VestigeKeyHints.h"
#include "UI/VestigeHudMessages.h"
#include "VestigeHUD.generated.h"

class AVestigePlayerCharacter;
class APawn;
class UAbilitySystemComponent;
class UCommonInputSubsystem;
enum class ECommonInputType : uint8;
class UVestigeClockSubsystem;
class UVestigeHudWidget;
class UVestigeInteractionComponent;
class UVestigeInventoryComponent;
struct FGameplayTag;

/**
 * The farming HUD's data side for its player: the day and time from the game's UVestigeClockSubsystem, the item in hand
 * and the items from the inventory and the stamina from the ability system on the player state, the farmer's key hints,
 * the prompt for the interactable the pawn's UVestigeInteractionComponent focuses, and the feedback messages
 * UVestigeFeedbackSubsystem carries. It only displays.
 * It creates its HudWidgetClass widget (WBP_Hud, set on BP_HUD) for a local player at BeginPlay, builds each text when an
 * event says what it shows has changed, and pushes it to the widget; the messages are the one thing it keeps, and only
 * until they fade. New HUD parts find their own data rather than growing this class (Systems.md, section 6).
 */
UCLASS()
class AVestigeHUD : public AHUD
{
	GENERATED_BODY()

public:
	AVestigeHUD();

	virtual void Tick(float DeltaSeconds) override;

	/** Most messages on screen at once, each in its own box, from UVestigeHudSettings at BeginPlay. */
	int32 GetMaxMessages() const { return MaxMessages; }

	/** The widget class this HUD shows, WBP_Hud on BP_HUD. */
	TSubclassOf<UVestigeHudWidget> GetHudWidgetClass() const { return HudWidgetClass; }

	/**
	 * The device whose keys the hints name after the player used InputType: the gamepad's after the gamepad, else the
	 * keyboard's. The game has no touch controls, so a touch leaves the keyboard hints.
	 */
	static EVestigeHintDevice GetHintDevice(ECommonInputType InputType);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** The widget the HUD shows. Without one the HUD logs an error and shows nothing. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UVestigeHudWidget> HudWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UVestigeHudWidget> HudWidget;

	/** Follows the new pawn's interaction component and key hints. Bound to the controller's OnPossessedPawnChanged. */
	UFUNCTION()
	void FollowPawn(APawn* OldPawn, APawn* NewPawn);

	void StopFollowingPawn();

	/** Rebuilds the day, clock and item in hand line and remembers the day and minute it shows. */
	void RefreshStatusText();

	/** Rebuilds the item in hand's name and the status line. Bound to the inventory's OnItemInHandChanged. */
	void RefreshItemInHandText();

	/** Bound to the interaction component's OnFocusChanged. */
	void RefreshPromptText();

	/** Follows the inventory on the player state, which outlives the pawn, so this happens once, at BeginPlay. */
	void FollowInventory();

	/** Rebuilds the bag line and the key hints, which name the hotbar's items. Bound to the inventory's OnInventoryChanged. */
	void RefreshInventoryTexts();

	void RefreshBagText();

	/** Follows the stamina on the player state's ability system, which outlives the pawn, so this happens once, at BeginPlay. */
	void FollowAbilitySystem();

	/**
	 * Bound to the change delegates of Stamina and MaxStamina, and to the tag events of Vestige.State.OutOfStamina and
	 * Vestige.State.Exhausted.
	 */
	void RefreshStamina();

	void RefreshKeyHintsText();

	/**
	 * Follows the device the player uses, from Common Input's subsystem on the local player, so the hints name its keys.
	 * Only called with a local player, which the widget needs anyway.
	 */
	void FollowInputMethod();

	/**
	 * Rebuilds the key hints and the prompt when the device whose keys they name changes. Bound to Common Input's
	 * OnInputMethodChangedNative, which also fires with an unchanged type; that rebuilds nothing.
	 */
	void HandleInputMethodChanged(ECommonInputType InputType);

	/** Shows Message in a box of its own, under the ones already showing, for MessageSeconds. Bound to UVestigeFeedbackSubsystem. */
	void ShowMessage(const FText& Message);

	/** Drops the messages whose time is up, and waits for the next one to expire. */
	void RemoveExpiredMessages();

	/** Pushes the list to the widget, and sets the timer for the next message to expire. */
	void MessagesChanged();

	// Read from UVestigeHudSettings at BeginPlay and kept for the session.
	float MessageSeconds = 0.f;
	int32 MaxMessages = 0;

	TWeakObjectPtr<const UVestigeClockSubsystem> Clock;
	TWeakObjectPtr<UVestigeInteractionComponent> Interaction;
	TWeakObjectPtr<UVestigeInventoryComponent> Inventory;
	TWeakObjectPtr<UAbilitySystemComponent> AbilitySystem;
	TWeakObjectPtr<const AVestigePlayerCharacter> Farmer;
	TWeakObjectPtr<UCommonInputSubsystem> InputMethod;

	/** The device whose keys the hints and the prompt name. */
	EVestigeHintDevice HintDevice = EVestigeHintDevice::KeyboardAndMouse;

	FText ItemInHandText;

	// The day and minute the status line shows, so Tick rebuilds it only when one changes.
	int32 StatusDay = INDEX_NONE;
	int32 StatusClockMinutes = INDEX_NONE;

	FVestigeHudMessages Messages;
	FTimerHandle ExpireTimer;
};
