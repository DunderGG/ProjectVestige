// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "Components/ActorComponent.h"
#include "VestigeRestComponent.generated.h"

class AVestigeDayCycle;
class UAbilitySystemComponent;
struct FVestigeRestSaveData;

/**
 * Owns the end of the player's day and the late-night rules (GameDesign.md, section 5): going to sleep, staying up
 * until the day ends, the warnings at LateWarningHour and LateNightHour, the stamina drain from LateNightHour until the
 * next day, and exhaustion. A night that ends at LateNightHour or later starts the next day exhausted, with stamina
 * refilled to ExhaustedRefillFraction of the maximum; an earlier night refills it fully and clears exhaustion.
 * Lives on the player's pawn and changes stamina through gameplay effects on the owner's ability system.
 */
UCLASS(ClassGroup = (Farming), meta = (BlueprintSpawnableComponent))
class UVestigeRestComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UVestigeRestComponent();

	/** Ends the day and starts the next one, exhausted if it is LateNightHour or later. Does nothing without a day cycle. */
	void Sleep();

	/** Whether the owner is exhausted from a late night. False without an ability system. */
	bool IsExhausted() const;

	/** Whether stamina is draining for the late night. */
	bool IsDrainingStamina() const { return LateNightDrain.IsValid(); }

	/** Whether the day started exhausted, for saving. */
	FVestigeRestSaveData GetSaveData() const;

	/** Makes the owner exhausted again if the save says so, without refilling or draining anything. */
	void RestoreFromSaveData(const FVestigeRestSaveData& Data);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Warns at LateWarningHour, and warns and starts the drain at LateNightHour. Bound to AVestigeDayCycle::OnHourStarted. */
	void ReactToHour(int32 Hour);

	/** Starts the next day when the clock reaches the end of the day. Bound to AVestigeDayCycle::OnDayEndReached. */
	void StayUpAllNight(int32 EndedDay);

	/**
	 * Stops the drain, refills stamina and sets or clears exhaustion by how late it is, then starts the next day, and
	 * tells the player, with MessageFormat's {Day} set to the new day.
	 */
	void StartNextDay(const FText& MessageFormat);

	/** Makes the owner exhausted, once, and refills stamina to ExhaustedRefillFraction, telling the player. */
	void WakeExhausted(UAbilitySystemComponent& AbilitySystem);

	/** Refills stamina fully and clears exhaustion, telling the player if there was any. */
	void WakeRested(UAbilitySystemComponent& AbilitySystem);

	/** Sets the owner's stamina to Fraction of its maximum through UVestigeStaminaRefillEffect. */
	void RefillStamina(UAbilitySystemComponent& AbilitySystem, float Fraction) const;

	void StartLateNightDrain();
	void StopLateNightDrain();

	/** The ability system of the player who owns this pawn, or nullptr. */
	UAbilitySystemComponent* GetAbilitySystem() const;

	/** Hour of the "getting late" warning, from midnight at the start of the day. */
	UPROPERTY(EditDefaultsOnly, Category = "Farming|Rest", meta = (AllowPrivateAccess = "true", ClampMin = 0, ClampMax = 30))
	int32 LateWarningHour = 22;

	/**
	 * Hour the late night starts, from midnight at the start of the day (24 is midnight): stamina starts draining, and
	 * going to bed from then on makes the player exhausted.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Farming|Rest", meta = (AllowPrivateAccess = "true", ClampMin = 0, ClampMax = 30))
	int32 LateNightHour = 24;

	/** Stamina the late night drains per game hour, whatever the clock's speed. */
	UPROPERTY(EditDefaultsOnly, Category = "Farming|Rest", meta = (AllowPrivateAccess = "true", ClampMin = 0))
	float LateNightDrainPerGameHour = 10.f;

	/** Share of the maximum stamina an exhausted player starts the day with. */
	UPROPERTY(EditDefaultsOnly, Category = "Farming|Rest", meta = (AllowPrivateAccess = "true", ClampMin = 0, ClampMax = 1))
	float ExhaustedRefillFraction = 0.75f;

	UPROPERTY()
	TObjectPtr<AVestigeDayCycle> DayCycle;

	/** The late-night drain while it runs, on the owner's ability system. */
	FActiveGameplayEffectHandle LateNightDrain;

	/** The ability system the drain runs on, to remove it from even after the pawn has lost its player state. */
	TWeakObjectPtr<UAbilitySystemComponent> DrainedAbilitySystem;
};
