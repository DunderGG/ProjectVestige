// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "Engine/TimerHandle.h"
#include "GameplayTagContainer.h"
#include "VestigePassingLineComponent.generated.h"

class UVestigeClockSubsystem;
class UVestigeConversationComponent;
class UVestigeMilestoneSubsystem;

namespace VestigePassingLine
{
	/** The default earshot radius, in cm: about 18 m, so a line is heard in the field and not at the well. */
	constexpr float DefaultEarshotRadius = 1800.f;
}

/**
 * Decides when a character says a line in passing (Systems.md, section 11): its sphere is the earshot, and on the events that can
 * make a line due it asks the player's conversation component to say one (SayPassingLine), which chooses, applies and shows it.
 * The events are a milestone reached (tried on the next tick, once however many arrive together, so a priority picks between
 * them), the local player's pawn coming into earshot, the start of each game hour (a player who stays in earshot past midnight)
 * and the end of the quiet after a talk. It never ticks and does nothing on a new day. Added to the character's Blueprint
 * (BP_Hob), where the owner must be an AVestigeNpcCharacter. A line not said is invisible by design: nothing is reported.
 * When it says one, the body turns to the farmer, and back when the line ends.
 */
UCLASS(ClassGroup = (Characters), meta = (BlueprintSpawnableComponent))
class UVestigePassingLineComponent : public USphereComponent
{
	GENERATED_BODY()

public:
	UVestigePassingLineComponent();

	/** Asks for a line now, from the player in earshot. Public so a test needs no event. */
	void TrySay();

protected:
	/** Listens for the events that make a line due. Logs an error and listens to nothing when the owner isn't a character body. */
	virtual void BeginPlay() override;

	/** Stops listening and clears the timers. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** The local player's pawn came into earshot. Bound to OnComponentBeginOverlap. */
	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** Tries on the next tick, once however many are asked in the frame. Bound to the milestone subsystem's OnMilestoneReached. */
	void ScheduleTry(const FGameplayTag& Milestone);

	/** Bound to the clock's OnHourStarted. */
	void HandleHourStarted(int32 Hour);

	/** Runs the scheduled try. */
	void RunScheduledTry();

	/** The line the body said has ended: turns it back to its place when no talk has it. Bound to OnPassingLineEnded. */
	void HandleLineEnded();

	TWeakObjectPtr<UVestigeMilestoneSubsystem> MilestoneSource;
	TWeakObjectPtr<UVestigeClockSubsystem> ClockSource;

	/** The component whose line end is listened to, so EndPlay unbinds from the same one. */
	TWeakObjectPtr<UVestigeConversationComponent> ListenedConversation;

	/** The one-shot try for the end of the quiet after a talk. */
	FTimerHandle QuietTimer;

	/** Whether a try is waiting for the next tick. */
	bool bTryScheduled = false;
};
