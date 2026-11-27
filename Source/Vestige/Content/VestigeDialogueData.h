// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Content/VestigeContentData.h"
#include "VestigeDialogueData.generated.h"

class UVestigeDialogueCondition;
class UVestigeDialogueOutcome;
struct FVestigeDialogueContext;

namespace VestigeDialogue
{
	/** Most replies a line lists. */
	constexpr int32 MaxChoices = 3;

	/** Longest line text, in characters (English): three lines in the box, with room for a longer translation. */
	constexpr int32 MaxLineLength = 150;

	/** Longest reply text, in characters (English). */
	constexpr int32 MaxReplyLength = 40;

	/** Longest line said in passing, in characters (English): a caption of two short lines. */
	constexpr int32 MaxPassingLineLength = 80;

	/**
	 * The reserved Next that ends the talk after the line or reply, in place of naming a line: a line with no Next goes to the
	 * following line of the asset, so a line in the middle of the list (a reply's answer) needs this to be the last one played.
	 */
	inline const FName EndTalk(TEXT("End"));
}

/** How a conversation reaches the player. A conversation is chosen only among those of its own delivery. */
UENUM()
enum class EVestigeDialogueDelivery : uint8
{
	/** A talk in the dialogue box, started by the player. */
	Talk,

	/** One line said unasked as a caption while the player keeps playing: no box, no reply, no trust, not a talk. */
	Passing
};

/** One reply the player can pick under a line. */
USTRUCT()
struct FVestigeDialogueChoice
{
	GENERATED_BODY()

	/** The reply's stable ID (Hob.Meet.Reply.Thanks), unique in the asset. */
	UPROPERTY(EditAnywhere, Category = "Dialogue")
	FName ReplyId;

	/** What the player says, linked to its entry in the dialogue string table. */
	UPROPERTY(EditAnywhere, Category = "Dialogue")
	FText Text;

	/** The line that follows the reply, or VestigeDialogue::EndTalk to end the talk. Required. */
	UPROPERTY(EditAnywhere, Category = "Dialogue")
	FName Next;

	/** The reply is listed only while all of these hold. */
	UPROPERTY(EditAnywhere, Instanced, Category = "Dialogue")
	TArray<TObjectPtr<UVestigeDialogueCondition>> Conditions;

	/** Applied when the reply is chosen. */
	UPROPERTY(EditAnywhere, Instanced, Category = "Dialogue")
	TArray<TObjectPtr<UVestigeDialogueOutcome>> Outcomes;
};

/** One line of a conversation: what the speaker says, and where the talk goes after it. */
USTRUCT()
struct FVestigeDialogueLine
{
	GENERATED_BODY()

	/** The line's stable ID (Hob.Meet.010), unique in the asset. For a pool of texts, the pool's ID (Hob.Greet.Morning). */
	UPROPERTY(EditAnywhere, Category = "Dialogue")
	FName LineId;

	/** What the speaker says, each linked to an entry of the dialogue string table. Several make a pool: one is shown (PickText). */
	UPROPERTY(EditAnywhere, Category = "Dialogue")
	TArray<FText> Texts;

	/** The line that follows. None is the next line in the asset's list, and the last line ends the talk; EndTalk ends it here. */
	UPROPERTY(EditAnywhere, Category = "Dialogue")
	FName Next;

	/**
	 * The replies under the line. At most VestigeDialogue::MaxChoices are listed at once, so a line may hold more entries only when
	 * the ones beyond that carry conditions that keep them apart. Nothing advances until one is chosen.
	 */
	UPROPERTY(EditAnywhere, Category = "Dialogue")
	TArray<FVestigeDialogueChoice> Choices;

	/** Applied when the line is shown. */
	UPROPERTY(EditAnywhere, Instanced, Category = "Dialogue")
	TArray<TObjectPtr<UVestigeDialogueOutcome>> Outcomes;

	/**
	 * The text this talk shows: Texts[(Day + TalksTodayBefore) % Texts.Num()], where TalksTodayBefore is how many talks with
	 * the speaker came before this one today. Stateless: the day's first talk never repeats yesterday's (consecutive days differ
	 * by one) and a later talk never repeats the one before it. Empty without texts.
	 */
	const FText& PickText(int32 Day, int32 TalksTodayBefore) const;
};

/**
 * One conversation with a character: whom it is with, how it is chosen (priority and conditions) and its lines. One asset per
 * conversation (DA_Dialogue_HobFirstMeeting), found through the Asset Manager. The conversation names its speaker, not the
 * other way round, so a mod adds one to a character without editing the character's asset (Systems.md, section 11). The
 * conversation component chooses and runs it; this asset holds only data.
 */
UCLASS()
class UVestigeDialogueData : public UVestigeContentData
{
	GENERATED_BODY()

public:
	/** The primary asset type conversations are registered under in Config/DefaultGame.ini. */
	static const FPrimaryAssetType AssetType;

	const FPrimaryAssetId& GetSpeaker() const { return Speaker; }

	int32 GetPriority() const { return Priority; }

	EVestigeDialogueDelivery GetDelivery() const { return Delivery; }

	const TArray<TObjectPtr<UVestigeDialogueCondition>>& GetConditions() const { return Conditions; }

	/** The lines in play order: the talk starts at the first. */
	const TArray<FVestigeDialogueLine>& GetLines() const { return Lines; }

	/** The line with this ID, or nullptr. */
	const FVestigeDialogueLine* FindLine(FName LineId) const;

	/** Whether every condition holds for Context. A null condition counts as not holding. */
	bool AreConditionsMet(const FVestigeDialogueContext& Context) const;

#if WITH_EDITOR
	/**
	 * Reports what would break the talk: an unknown speaker, no lines, a missing or repeated ID, a Next that names no line, a
	 * line without text, more than the most replies without conditions, a null condition or outcome, and what each condition or
	 * outcome reports of itself. A Passing conversation must also be exactly one line with exactly one text, no replies and no Next. A text that isn't from a string table is a warning.
	 */
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

#if WITH_DEV_AUTOMATION_TESTS
	/** For tests only. Conversations get their data in the editor. */
	void SetSpeakerForTests(const FPrimaryAssetId& InSpeaker) { Speaker = InSpeaker; }
	void SetPriorityForTests(int32 InPriority) { Priority = InPriority; }
	void SetDeliveryForTests(EVestigeDialogueDelivery InDelivery) { Delivery = InDelivery; }
	void SetConditionsForTests(TArray<TObjectPtr<UVestigeDialogueCondition>> InConditions) { Conditions = MoveTemp(InConditions); }
	void SetLinesForTests(TArray<FVestigeDialogueLine> InLines) { Lines = MoveTemp(InLines); }
#endif

protected:
	virtual FPrimaryAssetType GetContentType() const override { return AssetType; }

private:
	/** The character who speaks (a VestigeCharacter, such as VestigeCharacter:Base.Hob). */
	UPROPERTY(EditDefaultsOnly, Category = "Dialogue", meta = (AllowPrivateAccess = "true", AllowedTypes = "VestigeCharacter"))
	FPrimaryAssetId Speaker;

	/** Among the speaker's conversations whose conditions all hold, the highest priority is the one played. */
	UPROPERTY(EditDefaultsOnly, Category = "Dialogue", meta = (AllowPrivateAccess = "true"))
	int32 Priority = 0;

	/** Talk opens the dialogue box; Passing is one caption line said in passing (Systems.md, section 11). */
	UPROPERTY(EditDefaultsOnly, Category = "Dialogue", meta = (AllowPrivateAccess = "true"))
	EVestigeDialogueDelivery Delivery = EVestigeDialogueDelivery::Talk;

	/** All of these must hold for the conversation to be a candidate. */
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Dialogue", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UVestigeDialogueCondition>> Conditions;

	/** The lines, played from the first. */
	UPROPERTY(EditDefaultsOnly, Category = "Dialogue", meta = (AllowPrivateAccess = "true", TitleProperty = "LineId"))
	TArray<FVestigeDialogueLine> Lines;
};
