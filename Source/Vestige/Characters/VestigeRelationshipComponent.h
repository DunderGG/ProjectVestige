// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/VestigeSaveable.h"
#include "GameplayTagContainer.h"
#include "UObject/PrimaryAssetId.h"
#include "VestigeRelationshipComponent.generated.h"

class UVestigeCharacterData;

/**
 * How the player stands with each character (Systems.md, section 11), by the character's primary asset ID: whether they have
 * met, the trust earned by talking and the flags set (each with the day it was last set), which are saved, and how many talks
 * with them started today, which isn't (after a reload the day's greeting plays again). Lives on the player state, so it
 * outlives the pawn; characters themselves keep no feelings about the player.
 * Saves under Base.Relationships through UVestigeSaveRegistry while it is in play.
 */
UCLASS(ClassGroup = (Characters), meta = (BlueprintSpawnableComponent))
class UVestigeRelationshipComponent : public UActorComponent, public IVestigeSaveable
{
	GENERATED_BODY()

public:
	UVestigeRelationshipComponent();

	/** The relationship component of PlayerActor: its own, or its player state's if it is a pawn. Nullptr if it has none. */
	static UVestigeRelationshipComponent* Find(const AActor* PlayerActor);

	/** Whether the player has met Character. */
	bool HasMet(const FPrimaryAssetId& Character) const;

	/** How many talks with Character started on Day: 0 when the last talk was on another day. */
	int32 GetTalksToday(const FPrimaryAssetId& Character, int32 Day) const;

	/** A talk with Character starts on Day: counts it, and marks Character met. */
	void RecordTalk(const FPrimaryAssetId& Character, int32 Day);

	/** Adds Amount to the trust Character has earned by talking. Does nothing for an Amount below 1. Logs the new trust. */
	void AddTrust(const UVestigeCharacterData& Character, int32 Amount);

	/** The trust points Character has earned by talking, not counting the farm's firsts. */
	int32 GetEarnedTrust(const FPrimaryAssetId& Character) const;

	/**
	 * How much the player trusts Character: the points earned plus, for each of the character's TrustFromMilestones that
	 * UVestigeMilestoneSubsystem has reached, its points. Worked out now, never stored, so a first reached before meeting the
	 * character still counts and nothing counts twice.
	 */
	int32 GetTrust(const UVestigeCharacterData& Character) const;

	/** Sets Flag for Character on Day, or moves its day to Day if it is set. Does nothing for an invalid tag. */
	void SetFlag(const UVestigeCharacterData& Character, const FGameplayTag& Flag, int32 Day);

	/** Whether Character's Flag is set, whenever it was set. */
	bool HasFlag(const FPrimaryAssetId& Character, const FGameplayTag& Flag) const;

	/** Character's flags, each with the day it was last set. Empty for a character with none. */
	const TMap<FGameplayTag, int32>& GetFlagDays(const FPrimaryAssetId& Character) const;

	virtual FName GetSaveKey() const override;
	virtual int32 GetSaveVersion() const override;
	virtual FVestigeSaveRecord WriteSaveRecord() const override;
	virtual bool ReadSaveRecord(const FVestigeSaveRecord& Record, int32& OutLeftOutCount) override;

	/** Forgets everyone, as the player starts a new game having met no one. */
	virtual void StartNewGame() override;

protected:
	/** Registers with the save registry, which hands over the saved part or starts a new game. */
	virtual void BeginPlay() override;

	/** Keeps its saved part for the next map, then leaves the registry. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** The talks with one character on one day. */
	struct FTalksOnDay
	{
		int32 Day = 0;
		int32 Count = 0;
	};

	/** Forgets all of it. */
	void Clear();

	TSet<FPrimaryAssetId> Met;
	TMap<FPrimaryAssetId, FTalksOnDay> Talks;
	TMap<FPrimaryAssetId, int32> EarnedTrust;
	TMap<FPrimaryAssetId, TMap<FGameplayTag, int32>> Flags;
};
