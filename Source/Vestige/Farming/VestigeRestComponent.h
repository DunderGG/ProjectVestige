// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/VestigeSaveable.h"
#include "VestigeRestComponent.generated.h"

class UAbilitySystemComponent;
class UVestigeClockSubsystem;
struct FVestigeRestSaveData;

/**
 * Owns the player's side of the end of the day and the late-night rules (GameDesign.md, section 5): asking the clock to
 * end the day when going to sleep, the warnings at LateWarningHour and LateNightHour, the stamina drain over
 * each game minute from LateNightHour until the day ends, and waking up. A day that ends at LateNightHour or later (staying up until the
 * day's end included) starts the next day exhausted, with stamina refilled to ExhaustedRefillFraction of the maximum; an
 * earlier one refills it fully and clears exhaustion. Starting the next day is the game mode's job (AVestigeGameMode).
 * Lives on the player state, so the night happens with or without a body, and changes stamina through gameplay
 * effects on the owner's ability system.
 * Saves whether the player is exhausted under Base.Rest through UVestigeSaveRegistry while it is in play.
 */
UCLASS(ClassGroup = (Rest), meta = (BlueprintSpawnableComponent))
class UVestigeRestComponent : public UActorComponent, public IVestigeSaveable
{
	GENERATED_BODY()

public:
	UVestigeRestComponent();

	/** The rest component of PlayerActor: its own, or its player state's if it is a pawn. Nullptr if it has none. */
	static UVestigeRestComponent* Find(const AActor* PlayerActor);

	/**
	 * Goes to bed: asks the clock to end the day now. The game mode then starts the next day, and the player wakes
	 * exhausted if it was LateNightHour or later. Does nothing without a clock or once the day has ended.
	 */
	void Sleep();

	/** Whether the owner is exhausted from a late night. False without an ability system. */
	bool IsExhausted() const;

	/** Whether the day started exhausted, for saving. */
	FVestigeRestSaveData GetSaveData() const;

	/** Makes the owner exhausted again if the save says so, without refilling or draining anything. */
	void RestoreFromSaveData(const FVestigeRestSaveData& Data);

	virtual FName GetSaveKey() const override;
	virtual int32 GetSaveVersion() const override;
	virtual FVestigeSaveRecord WriteSaveRecord() const override;
	virtual bool ReadSaveRecord(const FVestigeSaveRecord& Record, int32& OutLeftOutCount) override;

	/** Clears exhaustion, without a message, as a new player starts rested. */
	virtual void StartNewGame() override;

protected:
	/** Follows the clock, and its minutes if the game data has a usable late-night drain, then takes its saved part. */
	virtual void BeginPlay() override;

	/** Keeps its saved part for the next map, then stops following the clock. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Warns at LateWarningHour and at LateNightHour. Bound to UVestigeClockSubsystem::OnHourStarted. */
	void ReactToHour(int32 Hour);

	/**
	 * Drains one game minute's share of LateNightDrainPerGameHour for the minute that ends at Minute, if it is after
	 * LateNightHour. Bound to UVestigeClockSubsystem::OnMinuteStarted, so the drain follows game time: a skip drains as if
	 * the time had passed, and a stopped clock drains nothing.
	 */
	void DrainLateMinute(int32 Minute);

	/**
	 * Refills stamina and sets or clears exhaustion by how late the last day ended (the clock's GetLastDayEnd). Bound to
	 * UVestigeClockSubsystem::OnNewDay, so the save made at the new day's end of work holds the new stamina.
	 */
	void WakeUp(int32 NewDay);

	/** Makes the owner exhausted, once, and refills stamina to ExhaustedRefillFraction, telling the player. */
	void WakeExhausted(UAbilitySystemComponent& AbilitySystem);

	/** Refills stamina fully and clears exhaustion, telling the player if there was any. */
	void WakeRested(UAbilitySystemComponent& AbilitySystem);

	/** Applies the game data's exhausted effect, if it has one, without checking whether the owner already is. */
	void ApplyExhaustion(UAbilitySystemComponent& AbilitySystem) const;

	/** Sets the owner's stamina to Fraction of its maximum through the game data's stamina refill effect. */
	void RefillStamina(UAbilitySystemComponent& AbilitySystem, float Fraction) const;

	/** Whether the game data's late-night drain can drain once a minute: set and instant. Logs an error if it is set and isn't. */
	bool HasUsableLateNightDrain() const;

	/** Applies the game data's late-night drain once, for one game minute. */
	void ApplyLateNightDrain(UAbilitySystemComponent& AbilitySystem) const;

	/** The ability system of the player state this component is on, or nullptr. */
	UAbilitySystemComponent* GetAbilitySystem() const;

	/** Hour of the "getting late" warning, from midnight at the start of the day. */
	UPROPERTY(EditDefaultsOnly, Category = "Rest", meta = (AllowPrivateAccess = "true", ClampMin = 0, ClampMax = 30))
	int32 LateWarningHour = 22;

	/**
	 * Hour the late night starts, from midnight at the start of the day (24 is midnight): stamina starts draining, and
	 * going to bed from then on makes the player exhausted.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Rest", meta = (AllowPrivateAccess = "true", ClampMin = 0, ClampMax = 30))
	int32 LateNightHour = 24;

	/** Stamina the late night drains per game hour, a game minute's share at a time, whatever the clock's speed. */
	UPROPERTY(EditDefaultsOnly, Category = "Rest", meta = (AllowPrivateAccess = "true", ClampMin = 0))
	float LateNightDrainPerGameHour = 10.f;

	/** Share of the maximum stamina an exhausted player starts the day with. */
	UPROPERTY(EditDefaultsOnly, Category = "Rest", meta = (AllowPrivateAccess = "true", ClampMin = 0, ClampMax = 1))
	float ExhaustedRefillFraction = 0.75f;

	/** The game's clock, found at BeginPlay. It outlives the pawn, so EndPlay stops listening to it. */
	UPROPERTY()
	TObjectPtr<UVestigeClockSubsystem> Clock;
};
