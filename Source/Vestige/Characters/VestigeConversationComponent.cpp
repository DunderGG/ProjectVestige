// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Characters/VestigeConversationComponent.h"

#include "AbilitySystem/VestigeGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Algo/AllOf.h"
#include "Characters/VestigeDialogueLog.h"
#include "Characters/VestigeRelationshipComponent.h"
#include "Content/VestigeCharacterData.h"
#include "Content/VestigeContentCatalog.h"
#include "Content/VestigeDialogueCondition.h"
#include "Content/VestigeDialogueData.h"
#include "Content/VestigeDialogueOutcome.h"
#include "Core/VestigeFeedbackSubsystem.h"
#include "Core/VestigePlayerComponents.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "Time/VestigeClockSubsystem.h"

UVestigeConversationComponent::UVestigeConversationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

UVestigeConversationComponent* UVestigeConversationComponent::Find(const AActor* PlayerActor)
{
	return VestigePlayerComponents::Find<UVestigeConversationComponent>(PlayerActor);
}

void UVestigeConversationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	EndConversation();
	Super::EndPlay(EndPlayReason);
}

bool UVestigeConversationComponent::StartConversation(AActor& InSpeakerActor, const UVestigeCharacterData& Speaker)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UVestigeConversationComponent::StartConversation);

	if (IsInConversation() || IsUsingTool())
	{
		return false;
	}

	const FVestigeDialogueContext Context = MakeContext(Speaker);
	const UVestigeDialogueData* Chosen = ChooseDialogue(Context);
	if (!Chosen || Chosen->GetLines().IsEmpty())
	{
		VESTIGE_LOG(this, LogVestigeDialogue, Warning, "{Speaker} has no conversation to play, so the talk doesn't start.",
			Speaker.GetContentId().ToString());
		UVestigeFeedbackSubsystem::Report(GetWorld(), VestigeGameplayTags::Feedback_Dialogue_NothingToSay);
		return false;
	}
	VESTIGE_LOG(this, LogVestigeDialogue, Verbose, "Talking to {Speaker}: {Conversation}.", Speaker.GetContentId().ToString(),
		Chosen->GetContentId().ToString());

	Dialogue = Chosen;
	SpeakerData = &Speaker;
	SpeakerActor = &InSpeakerActor;
	TalkDay = Context.Day;
	TalksTodayBefore = Context.TalksToday;

	// "Met" counts from the start, so a first meeting cut short with Escape still happened.
	if (UVestigeRelationshipComponent* Relationships = UVestigeRelationshipComponent::Find(GetOwner()))
	{
		Relationships->RecordTalk(Speaker.GetPrimaryAssetId(), Context.Day);
	}
	if (UVestigeClockSubsystem* Clock = UVestigeClockSubsystem::Get(GetWorld()))
	{
		Clock->Hold(VestigeDialogue::ClockHold);
		HeldClock = Clock;
	}

	EnterLine(0);
	OnConversationStarted.Broadcast();

	// A listener that can't show the talk (no box) ends it, so the clock is never held for a talk nobody sees.
	if (!IsInConversation())
	{
		return false;
	}
	OnLineChanged.Broadcast();
	return true;
}

void UVestigeConversationComponent::Advance()
{
	if (!IsInConversation() || !ListedChoices.IsEmpty())
	{
		return;
	}
	const FVestigeDialogueLine& Line = Dialogue->GetLines()[CurrentLine];
	if (!Line.Next.IsNone())
	{
		GoToLine(Line.Next);
		return;
	}
	const int32 NextIndex = FindNextLineIndex(Line);
	if (NextIndex == INDEX_NONE)
	{
		EndConversation();
		return;
	}
	ShowLine(NextIndex);
}

void UVestigeConversationComponent::Choose(int32 Index)
{
	if (!IsInConversation() || !ListedChoices.IsValidIndex(Index))
	{
		return;
	}
	const FVestigeDialogueChoice& Choice = Dialogue->GetLines()[CurrentLine].Choices[ListedChoices[Index]];
	const FVestigeDialogueContext Context = MakeContext(*SpeakerData);
	for (const UVestigeDialogueOutcome* Outcome : Choice.Outcomes)
	{
		if (Outcome)
		{
			Outcome->Apply(Context);
		}
	}
	GoToLine(Choice.Next);
}

void UVestigeConversationComponent::EndConversation()
{
	if (!IsInConversation())
	{
		return;
	}
	if (UVestigeClockSubsystem* Clock = HeldClock.Get())
	{
		Clock->Release(VestigeDialogue::ClockHold);
	}
	HeldClock.Reset();

	Dialogue = nullptr;
	SpeakerData = nullptr;
	SpeakerActor.Reset();
	LineText = FText::GetEmpty();
	ChoiceTexts.Reset();
	ListedChoices.Reset();
	CurrentLine = INDEX_NONE;
	OnConversationEnded.Broadcast();
}

bool UVestigeConversationComponent::IsLastLine() const
{
	return IsInConversation() && ListedChoices.IsEmpty() && FindNextLineIndex(Dialogue->GetLines()[CurrentLine]) == INDEX_NONE;
}

bool UVestigeConversationComponent::IsUsingTool() const
{
	const UAbilitySystemComponent* AbilitySystem = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
	return AbilitySystem && AbilitySystem->HasMatchingGameplayTag(VestigeGameplayTags::State_UsingTool);
}

FVestigeDialogueContext UVestigeConversationComponent::MakeContext(const UVestigeCharacterData& Speaker) const
{
	FVestigeDialogueContext Context;
	Context.Speaker = &Speaker;
	Context.Player = Cast<APlayerState>(GetOwner());
	if (const UVestigeClockSubsystem* Clock = UVestigeClockSubsystem::Get(GetWorld()))
	{
		Context.Day = Clock->GetCurrentDay();
		Context.TimeOfDay = Clock->GetTimeOfDay();
	}
	if (const UVestigeRelationshipComponent* Relationships = UVestigeRelationshipComponent::Find(GetOwner()))
	{
		Context.bHasMet = Relationships->HasMet(Speaker.GetPrimaryAssetId());
		Context.TalksToday = Relationships->GetTalksToday(Speaker.GetPrimaryAssetId(), Context.Day);
	}
	return Context;
}

const UVestigeDialogueData* UVestigeConversationComponent::ChooseDialogue(const FVestigeDialogueContext& Context) const
{
	const UVestigeContentCatalog* Catalog = UVestigeContentCatalog::Get(GetWorld());
	if (!Catalog)
	{
		return nullptr;
	}

	// Listed in ID order, so with a tie the lower ID is found first and kept.
	TArray<const UVestigeContentData*> All;
	Catalog->FindAll(UVestigeDialogueData::AssetType, All);

	const FPrimaryAssetId SpeakerId = Context.Speaker->GetPrimaryAssetId();
	const UVestigeDialogueData* Best = nullptr;
	for (const UVestigeContentData* Content : All)
	{
		const UVestigeDialogueData* Candidate = Cast<UVestigeDialogueData>(Content);
		if (!Candidate || Candidate->GetSpeaker() != SpeakerId || !Candidate->AreConditionsMet(Context))
		{
			continue;
		}
		if (Best && Candidate->GetPriority() == Best->GetPriority())
		{
			VESTIGE_LOG(this, LogVestigeDialogue, Warning,
				"{First} and {Second} both apply to {Speaker} at priority {Priority}, so the first by ID is played. Give them different "
				"priorities.",
				Best->GetContentId().ToString(), Candidate->GetContentId().ToString(), SpeakerId.ToString(), Best->GetPriority());
		}
		else if (!Best || Candidate->GetPriority() > Best->GetPriority())
		{
			Best = Candidate;
		}
	}
	return Best;
}

void UVestigeConversationComponent::EnterLine(int32 Index)
{
	CurrentLine = Index;
	const FVestigeDialogueLine& Line = Dialogue->GetLines()[Index];
	LineText = Line.PickText(TalkDay, TalksTodayBefore);

	// The talk is recorded by now, so the values name the state after this talk began.
	const FVestigeDialogueContext Context = MakeContext(*SpeakerData);
	ChoiceTexts.Reset();
	ListedChoices.Reset();
	for (int32 ChoiceIndex = 0; ChoiceIndex < Line.Choices.Num(); ++ChoiceIndex)
	{
		const FVestigeDialogueChoice& Choice = Line.Choices[ChoiceIndex];
		const bool bListed = Algo::AllOf(Choice.Conditions,
			[&Context](const UVestigeDialogueCondition* Condition)
			{
				return Condition && Condition->IsMet(Context);
			});
		if (bListed)
		{
			ListedChoices.Add(ChoiceIndex);
			ChoiceTexts.Add(Choice.Text);
		}
	}
	for (const UVestigeDialogueOutcome* Outcome : Line.Outcomes)
	{
		if (Outcome)
		{
			Outcome->Apply(Context);
		}
	}
}

void UVestigeConversationComponent::ShowLine(int32 Index)
{
	EnterLine(Index);
	OnLineChanged.Broadcast();
}

void UVestigeConversationComponent::GoToLine(FName LineId)
{
	const TArray<FVestigeDialogueLine>& Lines = Dialogue->GetLines();
	const int32 Index = Lines.IndexOfByPredicate(
		[LineId](const FVestigeDialogueLine& Line)
		{
			return Line.LineId == LineId;
		});
	if (Index == INDEX_NONE)
	{
		VESTIGE_LOG(this, LogVestigeDialogue, Error, "{Conversation} goes next to {Line}, which it has no line for, so the talk ends.",
			Dialogue->GetContentId().ToString(), LineId.ToString());
		EndConversation();
		return;
	}
	ShowLine(Index);
}

int32 UVestigeConversationComponent::FindNextLineIndex(const FVestigeDialogueLine& Line) const
{
	const TArray<FVestigeDialogueLine>& Lines = Dialogue->GetLines();
	if (Line.Next.IsNone())
	{
		const int32 Following = CurrentLine + 1;
		return Lines.IsValidIndex(Following) ? Following : INDEX_NONE;
	}
	return Lines.IndexOfByPredicate(
		[&Line](const FVestigeDialogueLine& Candidate)
		{
			return Candidate.LineId == Line.Next;
		});
}
