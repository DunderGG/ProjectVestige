// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interaction/VestigeInteractable.h"
#include "UObject/PrimaryAssetId.h"
#include "VestigeNpcCharacter.generated.h"

class UBoxComponent;
class UVestigeCharacterData;
class UVestigeConversationComponent;

namespace VestigeNpc
{
	/** How fast a character turns to face the player or back, in degrees per second: a half turn takes 0.3 seconds. */
	constexpr float TurnDegreesPerSecond = 600.f;
}

/**
 * The body of a named character, robot or human, that the player can talk to (Systems.md, section 10): which character it is,
 * offering "Talk to Hob" and starting the talk, turning to the player through its engine AI controller's focus and back to its
 * placed facing when the talk ends. BP_Hob (Content/Vestige/Characters) is Hob, with a placeholder body of engine shapes, and
 * the one placed in L_Farm is where Hob stands. It knows nothing of what is said (the conversation component's) or how the
 * player stands with it (the relationship component's).
 */
UCLASS(Blueprintable)
class AVestigeNpcCharacter : public ACharacter, public IVestigeInteractable
{
	GENERATED_BODY()

public:
	AVestigeNpcCharacter();

	/** The action name of the character's one option. */
	static const FName TalkAction;

	/** The character this body is, or nothing. */
	const FPrimaryAssetId& GetCharacterId() const { return CharacterId; }

	/** The character's definition, found at BeginPlay, or nullptr when the ID names none the catalogue has. */
	const UVestigeCharacterData* GetCharacterData() const { return CharacterData; }

	/** Offers Talk to anyone with a conversation component while no talk runs. */
	virtual void GetInteractionOptions_Implementation(AActor* Interactor, TArray<FVestigeInteractionOption>& OutOptions) const override;

	/** Starts the talk with Interactor's conversation component and turns to face Interactor. */
	virtual void Interact_Implementation(AActor* Interactor, const FVestigeInteractionOption& Option) override;

	/** Faces Target through the AI controller's focus. Does nothing without a controller. Public for the passing line component. */
	void FaceActor(AActor& Target);

	/** Faces the placed facing again, through the controller's focal point. Does nothing without a controller. */
	void FaceHome();

protected:
	/** Finds the character's definition, builds the prompt text and holds the placed facing. */
	virtual void BeginPlay() override;

	/** Stops listening to the conversation component. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Turns back to the placed facing and stops listening. Bound to the conversation component's OnConversationEnded. */
	void FinishTalk();

	/** Which character this is (a VestigeCharacter, such as VestigeCharacter:Base.Hob). */
	UPROPERTY(EditDefaultsOnly, Category = "Character", meta = (AllowPrivateAccess = "true", AllowedTypes = "VestigeCharacter"))
	FPrimaryAssetId CharacterId;

	/** Where the character can be talked to from: a box a little larger than the body, on the interactables' collision channel. */
	UPROPERTY(VisibleAnywhere, Category = "Interaction")
	TObjectPtr<UBoxComponent> InteractionVolume;

	UPROPERTY()
	TObjectPtr<const UVestigeCharacterData> CharacterData;

	/** "Talk to Hob", built once at BeginPlay so the five-times-a-second ask copies a text and nothing more. */
	FText TalkText;

	/** The facing the level placed the character with, which it turns back to. */
	FRotator HomeRotation = FRotator::ZeroRotator;

	/** The component of the talk this body listens to, until it ends. */
	TWeakObjectPtr<UVestigeConversationComponent> Conversation;
};
