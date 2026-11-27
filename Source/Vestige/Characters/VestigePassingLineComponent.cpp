// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Characters/VestigePassingLineComponent.h"

#include "Characters/VestigeConversationComponent.h"
#include "Characters/VestigeDialogueLog.h"
#include "Characters/VestigeNpcCharacter.h"
#include "Core/VestigeMilestoneSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "TimerManager.h"
#include "Time/VestigeClockSubsystem.h"

UVestigePassingLineComponent::UVestigePassingLineComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// An earshot only: found by pawns, solid to nothing, no cost to navigation or rendering.
	InitSphereRadius(VestigePassingLine::DefaultEarshotRadius);
	SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetCollisionObjectType(ECC_WorldDynamic);
	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SetGenerateOverlapEvents(true);
	SetCanEverAffectNavigation(false);
	bHiddenInGame = true;
}

void UVestigePassingLineComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!Cast<AVestigeNpcCharacter>(GetOwner()))
	{
		VESTIGE_LOG(this, LogVestigeDialogue, Error, "The passing line component is on {Owner}, which isn't a character body, so it does nothing.",
			GetNameSafe(GetOwner()));
		SetGenerateOverlapEvents(false);
		return;
	}

	OnComponentBeginOverlap.AddDynamic(this, &UVestigePassingLineComponent::HandleBeginOverlap);
	if (UVestigeMilestoneSubsystem* MilestoneSubsystem = UVestigeMilestoneSubsystem::Get(GetWorld()))
	{
		MilestoneSubsystem->OnMilestoneReached.AddUObject(this, &UVestigePassingLineComponent::ScheduleTry);
		MilestoneSource = MilestoneSubsystem;
	}
	if (UVestigeClockSubsystem* ClockSubsystem = UVestigeClockSubsystem::Get(GetWorld()))
	{
		ClockSubsystem->OnHourStarted.AddUObject(this, &UVestigePassingLineComponent::HandleHourStarted);
		ClockSource = ClockSubsystem;
	}
}

void UVestigePassingLineComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	OnComponentBeginOverlap.RemoveAll(this);
	if (UVestigeMilestoneSubsystem* MilestoneSubsystem = MilestoneSource.Get())
	{
		MilestoneSubsystem->OnMilestoneReached.RemoveAll(this);
	}
	if (UVestigeClockSubsystem* ClockSubsystem = ClockSource.Get())
	{
		ClockSubsystem->OnHourStarted.RemoveAll(this);
	}
	if (UVestigeConversationComponent* Conversation = ListenedConversation.Get())
	{
		Conversation->OnPassingLineEnded.RemoveAll(this);
	}
	MilestoneSource.Reset();
	ClockSource.Reset();
	ListenedConversation.Reset();
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(QuietTimer);
	}
	bTryScheduled = false;
	Super::EndPlay(EndPlayReason);
}

void UVestigePassingLineComponent::TrySay()
{
	AVestigeNpcCharacter* Body = Cast<AVestigeNpcCharacter>(GetOwner());
	const UWorld* World = GetWorld();
	const APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	UVestigeConversationComponent* Conversation = PlayerController ? UVestigeConversationComponent::Find(PlayerController->PlayerState) : nullptr;
	const UVestigeCharacterData* Character = Body ? Body->GetCharacterData() : nullptr;
	if (!Pawn || !Conversation || !Character)
	{
		VESTIGE_LOG(this, LogVestigeDialogue, Verbose, "{Owner} has no one to say a line in passing to.", GetNameSafe(GetOwner()));
		return;
	}

	// Measured now, so a pawn that spawned inside the sphere needs no begin event.
	const float Earshot = GetScaledSphereRadius();
	if (FVector::DistSquared(Pawn->GetActorLocation(), Body->GetActorLocation()) > FMath::Square(Earshot))
	{
		return;
	}

	if (Conversation->SayPassingLine(*Body, *Character))
	{
		Body->FaceActor(*Pawn);
		Conversation->OnPassingLineEnded.RemoveAll(this);
		Conversation->OnPassingLineEnded.AddUObject(this, &UVestigePassingLineComponent::HandleLineEnded);
		ListenedConversation = Conversation;
		return;
	}

	// Refused for the quiet after a talk: tries once more when it is over, if the player is still in earshot then.
	const double QuietLeft = Conversation->GetPassingQuietSecondsLeft();
	if (QuietLeft > 0.0)
	{
		World->GetTimerManager().SetTimer(QuietTimer, this, &UVestigePassingLineComponent::TrySay, static_cast<float>(QuietLeft));
	}
}

void UVestigePassingLineComponent::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	const APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (OtherActor && PlayerController && OtherActor == PlayerController->GetPawn())
	{
		TrySay();
	}
}

void UVestigePassingLineComponent::ScheduleTry(const FGameplayTag& Milestone)
{
	if (bTryScheduled || !GetWorld())
	{
		return;
	}
	bTryScheduled = true;
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UVestigePassingLineComponent::RunScheduledTry);
}

void UVestigePassingLineComponent::HandleHourStarted(int32 Hour)
{
	TrySay();
}

void UVestigePassingLineComponent::RunScheduledTry()
{
	bTryScheduled = false;
	TrySay();
}

void UVestigePassingLineComponent::HandleLineEnded()
{
	AVestigeNpcCharacter* Body = Cast<AVestigeNpcCharacter>(GetOwner());
	const UVestigeConversationComponent* Conversation = ListenedConversation.Get();
	if (!Body || !Conversation || Conversation->GetPassingSpeakerActor() != Body || Conversation->IsInConversation())
	{
		return;
	}
	Body->FaceHome();
}
