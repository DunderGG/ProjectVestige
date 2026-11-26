// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Characters/VestigeNpcCharacter.h"

#include "AIController.h"
#include "Characters/VestigeConversationComponent.h"
#include "Characters/VestigeDialogueLog.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Content/VestigeCharacterData.h"
#include "Content/VestigeContentCatalog.h"
#include "GameFramework/CharacterMovementComponent.h"

#define LOCTEXT_NAMESPACE "VestigeNpcCharacter"

namespace VestigeNpcBody
{
	/** The capsule of a 1.5 m body: radius and half height, in cm. */
	constexpr float CapsuleRadius = 35.f;
	constexpr float CapsuleHalfHeight = 75.f;

	/** Half the size of the box the character is talked to from: a little larger than the capsule, up to the body's height. */
	const FVector InteractionExtent(65.f, 65.f, 80.f);

	/** How far ahead of the character the focal point is, so it turns toward its placed facing. Any distance does; the direction counts. */
	constexpr float HomeFocusDistance = 100.f;
}

const FName AVestigeNpcCharacter::TalkAction(TEXT("Talk"));

AVestigeNpcCharacter::AVestigeNpcCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->InitCapsuleSize(VestigeNpcBody::CapsuleRadius, VestigeNpcBody::CapsuleHalfHeight);

	// The controller's focus turns the body at the movement component's rate, never by snapping the actor.
	bUseControllerRotationYaw = false;
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = false;
	Movement->bUseControllerDesiredRotation = true;
	Movement->RotationRate = FRotator(0.f, VestigeNpc::TurnDegreesPerSecond, 0.f);

	// The engine's AI controller is enough while the character only stands and turns; a placed one gets it at once.
	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// Only found by the interaction component's query, which needs no overlap events.
	InteractionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionVolume"));
	InteractionVolume->SetupAttachment(GetRootComponent());
	InteractionVolume->SetBoxExtent(VestigeNpcBody::InteractionExtent);
	InteractionVolume->SetCollisionProfileName(VestigeInteraction::CollisionProfile);
	InteractionVolume->SetGenerateOverlapEvents(false);
	InteractionVolume->SetCanEverAffectNavigation(false);
}

void AVestigeNpcCharacter::BeginPlay()
{
	Super::BeginPlay();

	const UVestigeContentCatalog* Catalog = UVestigeContentCatalog::Get(GetWorld());
	CharacterData = Catalog ? Catalog->Find<UVestigeCharacterData>(CharacterId) : nullptr;
	if (CharacterData)
	{
		TalkText = FText::FormatNamed(LOCTEXT("TalkTo", "Talk to {Name}"), TEXT("Name"), CharacterData->GetDisplayName());
	}
	else
	{
		VESTIGE_LOG(this, LogVestigeDialogue, Warning, "No character has the ID {CharacterId}, so {Actor} offers nothing.",
			CharacterId.ToString(), GetName());
	}

	HomeRotation = GetActorRotation();
	FaceHome();
}

void AVestigeNpcCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UVestigeConversationComponent* Talk = Conversation.Get())
	{
		Talk->OnConversationEnded.RemoveAll(this);
	}
	Conversation.Reset();
	Super::EndPlay(EndPlayReason);
}

void AVestigeNpcCharacter::GetInteractionOptions_Implementation(AActor* Interactor, TArray<FVestigeInteractionOption>& OutOptions) const
{
	const UVestigeConversationComponent* Talk = UVestigeConversationComponent::Find(Interactor);
	if (!CharacterData || !Talk || Talk->IsInConversation())
	{
		return;
	}
	FVestigeInteractionOption& TalkOption = OutOptions.AddDefaulted_GetRef();
	TalkOption.Text = TalkText;
	TalkOption.Action = TalkAction;
}

void AVestigeNpcCharacter::Interact_Implementation(AActor* Interactor, const FVestigeInteractionOption& Option)
{
	UVestigeConversationComponent* Talk = Option.Action == TalkAction ? UVestigeConversationComponent::Find(Interactor) : nullptr;
	if (!CharacterData || !Talk || !Talk->StartConversation(*this, *CharacterData))
	{
		return;
	}

	// Listens for the end of this one talk only.
	Talk->OnConversationEnded.RemoveAll(this);
	Talk->OnConversationEnded.AddUObject(this, &AVestigeNpcCharacter::FinishTalk);
	Conversation = Talk;
	FaceActor(*Interactor);
}

void AVestigeNpcCharacter::FaceActor(AActor& Target)
{
	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		AIController->SetFocus(&Target);
	}
}

void AVestigeNpcCharacter::FaceHome()
{
	// The default priority, Gameplay, as SetFocus uses, so the focal point replaces the focus actor.
	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		AIController->SetFocalPoint(GetActorLocation() + HomeRotation.Vector() * VestigeNpcBody::HomeFocusDistance);
	}
}

void AVestigeNpcCharacter::FinishTalk()
{
	if (UVestigeConversationComponent* Talk = Conversation.Get())
	{
		Talk->OnConversationEnded.RemoveAll(this);
	}
	Conversation.Reset();
	FaceHome();
}

#undef LOCTEXT_NAMESPACE
