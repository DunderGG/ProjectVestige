// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Content/VestigeDialogueData.h"

#include "Content/VestigeCharacterData.h"
#include "Content/VestigeDialogueCondition.h"
#include "Content/VestigeDialogueContext.h"
#include "Content/VestigeDialogueOutcome.h"

#if WITH_EDITOR
#include "Algo/Count.h"
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "VestigeDialogueData"

const FPrimaryAssetType UVestigeDialogueData::AssetType(TEXT("VestigeDialogue"));

const FText& FVestigeDialogueLine::PickText(int32 Day, int32 TalksTodayBefore) const
{
	if (Texts.IsEmpty())
	{
		return FText::GetEmpty();
	}
	return Texts[(Day + TalksTodayBefore) % Texts.Num()];
}

const FVestigeDialogueLine* UVestigeDialogueData::FindLine(FName LineId) const
{
	return Lines.FindByPredicate(
		[LineId](const FVestigeDialogueLine& Line)
		{
			return Line.LineId == LineId;
		});
}

bool UVestigeDialogueData::AreConditionsMet(const FVestigeDialogueContext& Context) const
{
	for (const UVestigeDialogueCondition* Condition : Conditions)
	{
		if (!Condition || !Condition->IsMet(Context))
		{
			return false;
		}
	}
	return true;
}

#if WITH_EDITOR
namespace VestigeDialogueValidation
{
	/** Adds an error for each null entry of Objects, naming Owner (the line or reply they belong to). */
	template <typename T>
	bool CheckNoNulls(const TArray<TObjectPtr<T>>& Objects, const FText& Kind, FName Owner, FDataValidationContext& Context)
	{
		const bool bHasNull = Objects.ContainsByPredicate(
			[](const TObjectPtr<T>& Object)
			{
				return !Object;
			});
		if (!bHasNull)
		{
			return true;
		}
		Context.AddError(
			FText::Format(LOCTEXT("NullEntry", "{0} of {1} has an empty entry. Pick a class or remove it."), Kind, FText::FromName(Owner)));
		return false;
	}

	/** Whether every object in Objects passes its own IsDataValid (a null one is CheckNoNulls' to report). */
	template <typename T> bool CheckEntries(const TArray<TObjectPtr<T>>& Objects, FDataValidationContext& Context)
	{
		bool bAllValid = true;
		for (const T* Object : Objects)
		{
			if (Object && Object->IsDataValid(Context) == EDataValidationResult::Invalid)
			{
				bAllValid = false;
			}
		}
		return bAllValid;
	}

	/** Whether Text is a usable text from a string table: warns when it isn't from one, errors when it is empty. */
	bool CheckText(const FText& Text, FName Owner, FDataValidationContext& Context)
	{
		if (Text.IsEmpty())
		{
			Context.AddError(FText::Format(LOCTEXT("EmptyText", "{0} has an empty text."), FText::FromName(Owner)));
			return false;
		}
		if (!Text.IsFromStringTable())
		{
			Context.AddWarning(FText::Format(LOCTEXT("TextNotFromStringTable",
												 "{0} has a text that isn't from a string table, so it can't be translated by its ID. Link "
												 "it to its entry in ST_Dialogue."),
				FText::FromName(Owner)));
		}
		return true;
	}
}

EDataValidationResult UVestigeDialogueData::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	bool bValid = true;

	if (const TOptional<FText> SpeakerError = FindContentReferenceError(Speaker, UVestigeCharacterData::AssetType))
	{
		Context.AddError(FText::Format(LOCTEXT("BadSpeaker", "Speaker {0}"), SpeakerError.GetValue()));
		bValid = false;
	}
	if (Lines.IsEmpty())
	{
		Context.AddError(LOCTEXT("NoLines", "Lines is empty, so the talk has nothing to say."));
		bValid = false;
	}
	if (Delivery == EVestigeDialogueDelivery::Passing)
	{
		const bool bOneLine = Lines.Num() == 1;
		const bool bPlain = bOneLine && Lines[0].Texts.Num() == 1 && Lines[0].Choices.IsEmpty() && Lines[0].Next.IsNone();
		if (!bPlain)
		{
			Context.AddError(LOCTEXT("BadPassing",
				"A passing conversation is exactly one line with exactly one text, no replies and no Next: it is shown as a caption."));
			bValid = false;
		}
	}
	bValid &= VestigeDialogueValidation::CheckNoNulls(Conditions, LOCTEXT("Conditions", "Conditions"), GetFName(), Context);
	bValid &= VestigeDialogueValidation::CheckEntries(Conditions, Context);

	TSet<FName> LineIds;
	TSet<FName> ReplyIds;
	for (const FVestigeDialogueLine& Line : Lines)
	{
		if (Line.LineId.IsNone() || LineIds.Contains(Line.LineId))
		{
			Context.AddError(FText::Format(LOCTEXT("BadLineId", "Line ID \"{0}\" is empty or used twice."), FText::FromName(Line.LineId)));
			bValid = false;
		}
		if (Line.LineId == VestigeDialogue::EndTalk)
		{
			Context.AddError(LOCTEXT("ReservedLineId", "Line ID \"End\" is reserved for ending the talk."));
			bValid = false;
		}
		LineIds.Add(Line.LineId);
	}

	for (const FVestigeDialogueLine& Line : Lines)
	{
		if (Line.Texts.IsEmpty())
		{
			Context.AddError(FText::Format(LOCTEXT("NoTexts", "Line {0} has no text."), FText::FromName(Line.LineId)));
			bValid = false;
		}
		for (const FText& Text : Line.Texts)
		{
			bValid &= VestigeDialogueValidation::CheckText(Text, Line.LineId, Context);
		}
		if (!Line.Next.IsNone() && Line.Next != VestigeDialogue::EndTalk && !LineIds.Contains(Line.Next))
		{
			Context.AddError(FText::Format(LOCTEXT("BadLineNext", "Line {0} goes next to {1}, which isn't a line of this asset."),
				FText::FromName(Line.LineId), FText::FromName(Line.Next)));
			bValid = false;
		}

		// Replies with conditions may be more than the box lists, since the data keeps them apart; the ones without can't be.
		const int32 UnconditionalChoices = Algo::CountIf(Line.Choices,
			[](const FVestigeDialogueChoice& Choice)
			{
				return Choice.Conditions.IsEmpty();
			});
		if (UnconditionalChoices > VestigeDialogue::MaxChoices)
		{
			Context.AddError(
				FText::Format(LOCTEXT("TooManyChoices",
								  "Line {0} has more than {1} replies without conditions, so more than {1} could be listed at once."),
					FText::FromName(Line.LineId), VestigeDialogue::MaxChoices));
			bValid = false;
		}
		bValid &= VestigeDialogueValidation::CheckNoNulls(Line.Outcomes, LOCTEXT("Outcomes", "Outcomes"), Line.LineId, Context);
		bValid &= VestigeDialogueValidation::CheckEntries(Line.Outcomes, Context);

		for (const FVestigeDialogueChoice& Choice : Line.Choices)
		{
			if (Choice.ReplyId.IsNone() || ReplyIds.Contains(Choice.ReplyId))
			{
				Context.AddError(
					FText::Format(LOCTEXT("BadReplyId", "Reply ID \"{0}\" is empty or used twice."), FText::FromName(Choice.ReplyId)));
				bValid = false;
			}
			ReplyIds.Add(Choice.ReplyId);
			bValid &= VestigeDialogueValidation::CheckText(Choice.Text, Choice.ReplyId, Context);
			if (Choice.Next != VestigeDialogue::EndTalk && !LineIds.Contains(Choice.Next))
			{
				Context.AddError(FText::Format(LOCTEXT("BadReplyNext", "Reply {0} goes next to {1}, which isn't a line of this asset."),
					FText::FromName(Choice.ReplyId), FText::FromName(Choice.Next)));
				bValid = false;
			}
			bValid &= VestigeDialogueValidation::CheckNoNulls(
				Choice.Conditions, LOCTEXT("ReplyConditions", "Conditions"), Choice.ReplyId, Context);
			bValid &=
				VestigeDialogueValidation::CheckNoNulls(Choice.Outcomes, LOCTEXT("ReplyOutcomes", "Outcomes"), Choice.ReplyId, Context);
			bValid &= VestigeDialogueValidation::CheckEntries(Choice.Conditions, Context);
			bValid &= VestigeDialogueValidation::CheckEntries(Choice.Outcomes, Context);
		}
	}

	return bValid ? Result : EDataValidationResult::Invalid;
}
#endif

#undef LOCTEXT_NAMESPACE
