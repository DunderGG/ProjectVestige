// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Content/VestigeContentData.h"
#include "GameplayTagContainer.h"
#include "VestigeCharacterData.generated.h"

/**
 * The definition of one character, robot or human: its ID (Base.Hob), its display name and its tags. One asset per character
 * (DA_Character_Hob), found through the Asset Manager. It holds no state and lists no conversations: a conversation names its
 * speaker, so a mod adds one to a character without editing the character's asset (Systems.md, sections 10 and 11).
 */
UCLASS()
class UVestigeCharacterData : public UVestigeContentData
{
	GENERATED_BODY()

public:
	/** The primary asset type characters are registered under in Config/DefaultGame.ini. */
	static const FPrimaryAssetType AssetType;

	/** What the character is: Vestige.Character.Robot or Vestige.Character.Human. */
	const FGameplayTagContainer& GetCharacterTags() const { return CharacterTags; }

	/** The trust points each milestone gives toward this character: how the farm's firsts count (Systems.md, section 11). */
	const TMap<FGameplayTag, int32>& GetTrustFromMilestones() const { return TrustFromMilestones; }

#if WITH_EDITOR
	/** Reports a TrustFromMilestones entry with an invalid tag or fewer than 1 point. */
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

#if WITH_DEV_AUTOMATION_TESTS
	/** For tests only. Characters get their tags in the editor. */
	void SetCharacterTagsForTests(const FGameplayTagContainer& InCharacterTags) { CharacterTags = InCharacterTags; }

	/** For tests only. Characters get their trust points in the editor. */
	void SetTrustFromMilestonesForTests(const TMap<FGameplayTag, int32>& InTrustFromMilestones)
	{
		TrustFromMilestones = InTrustFromMilestones;
	}
#endif

protected:
	virtual FPrimaryAssetType GetContentType() const override { return AssetType; }

private:
	/** Whether the character is a robot or a human, under Vestige.Character. Nothing in C++ reads them yet. */
	UPROPERTY(EditDefaultsOnly, Category = "Character", meta = (AllowPrivateAccess = "true", Categories = "Vestige.Character"))
	FGameplayTagContainer CharacterTags;

	/**
	 * The trust points each milestone (Vestige.Milestone.Farm.Harvested) gives the player's trust with this character, at least 1.
	 * Counted when asked, never stored, so a milestone reached before meeting the character still counts.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Character", meta = (AllowPrivateAccess = "true", Categories = "Vestige.Milestone"))
	TMap<FGameplayTag, int32> TrustFromMilestones;
};
