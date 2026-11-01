// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VestigeRestComponent.generated.h"

class AVestigeDayCycle;

/**
 * Owns the end of the player's day: going to sleep, and passing out when the clock reaches the end of the day, each of
 * which refills the player's stamina. Lives on the player's pawn. The late-night rules and exhaustion join it next
 * (Systems.md, section 9).
 */
UCLASS(ClassGroup = (Farming), meta = (BlueprintSpawnableComponent))
class UVestigeRestComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UVestigeRestComponent();

	/** Ends the day and starts the next one. Does nothing without a day cycle. */
	void Sleep();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Starts the next day when the clock reaches the end of the day. Bound to AVestigeDayCycle::OnDayEndReached. */
	void PassOut(int32 EndedDay);

	/** Refills stamina and starts the next day, and tells the player, with MessageFormat's {Day} set to the new day. */
	void StartNextDay(const FText& MessageFormat);

	/** Fills the owner's stamina back up through UVestigeStaminaRefillEffect. Does nothing without an ability system. */
	void RefillStamina() const;

	UPROPERTY()
	TObjectPtr<AVestigeDayCycle> DayCycle;
};
