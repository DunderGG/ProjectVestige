// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/VestigeGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Characters/VestigeConversationComponent.h"
#include "Characters/VestigeDialogueOutcome_AddTrust.h"
#include "Characters/VestigeDialogueOutcome_SetFlag.h"
#include "Characters/VestigeRelationshipComponent.h"
#include "Characters/VestigeRelationshipSaveData.h"
#include "Components/ActorTestSpawner.h"
#include "Content/VestigeCharacterData.h"
#include "Content/VestigeContentCatalog.h"
#include "Content/VestigeDialogueCondition_Date.h"
#include "Content/VestigeDialogueCondition_Flag.h"
#include "Content/VestigeDialogueCondition_HasMet.h"
#include "Content/VestigeDialogueCondition_Milestone.h"
#include "Content/VestigeDialogueCondition_TalkedToday.h"
#include "Content/VestigeDialogueCondition_TimeOfDay.h"
#include "Content/VestigeDialogueCondition_Trust.h"
#include "Content/VestigeDialogueContext.h"
#include "Content/VestigeDialogueData.h"
#include "Core/VestigeFeedbackSubsystem.h"
#include "Core/VestigeMilestoneSubsystem.h"
#include "Core/VestigeSaveRecord.h"
#include "CQTest.h"
#include "Engine/AssetManager.h"
#include "EnhancedActionKeyMapping.h"
#include "Farming/FarmGrid.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Internationalization/Text.h"
#include "Misc/DataValidation.h"
#include "Player/VestigeCharacterMovementComponent.h"
#include "Player/VestigePlayerCharacter.h"
#include "Player/VestigePlayerState.h"
#include "Tests/VestigeTestDialogueOutcome.h"
#include "Time/VestigeClockSaveData.h"
#include "Time/VestigeClockSubsystem.h"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace VestigeDialogueTests
{
	/** The made-up speaker of the tests that build their own conversations. */
	const FName TestSpeakerId(TEXT("Test.Speaker"));

	/** Hob, whose real data the last tests play. */
	// A function, not a static: UVestigeCharacterData::AssetType is another file's static, which a unity build may initialize after this one.
	FPrimaryAssetId HobId()
	{
		return FPrimaryAssetId(UVestigeCharacterData::AssetType, TEXT("Base.Hob"));
	}

	/** The first of the day's hours the clock shows, and the hours the dialogue bands and the day end use. */
	constexpr float MorningHour = 8.f;

	FVestigeDialogueLine MakeLine(FName Id, const TCHAR* Text, FName Next = NAME_None)
	{
		FVestigeDialogueLine Line;
		Line.LineId = Id;
		Line.Texts.Add(FText::AsCultureInvariant(Text));
		Line.Next = Next;
		return Line;
	}

	FVestigeDialogueChoice MakeChoice(FName Id, const TCHAR* Text, FName Next)
	{
		FVestigeDialogueChoice Choice;
		Choice.ReplyId = Id;
		Choice.Text = FText::AsCultureInvariant(Text);
		Choice.Next = Next;
		return Choice;
	}

	/** The string table key of Text, or an empty name when it isn't from one. */
	FName GetTableKey(const FText& Text)
	{
		FName TableId;
		FString Key;
		return FTextInspector::GetTableIdAndKey(Text, TableId, Key) ? FName(*Key) : NAME_None;
	}

	FGameplayTag MakeTag(const TCHAR* Name) { return FGameplayTag::RequestGameplayTag(FName(Name)); }

	/** A relationship flag, a tag under Vestige.Relationship. */
	FGameplayTag MakeFlag(const TCHAR* Name) { return MakeTag(*(FString(TEXT("Vestige.Relationship.")) + Name)); }

	TEST_CLASS(Dialogue, "Vestige")
	{
		FActorTestSpawner Spawner;
		UVestigeContentCatalog* Catalog = nullptr;
		UVestigeClockSubsystem* Clock = nullptr;
		UVestigeMilestoneSubsystem* Milestones = nullptr;
		AVestigePlayerState* PlayerState = nullptr;
		UVestigeConversationComponent* Talk = nullptr;
		UVestigeRelationshipComponent* Relationships = nullptr;
		UVestigeCharacterData* Speaker = nullptr;
		AActor* SpeakerActor = nullptr;

		/** What the outcomes of a test wrote, in order. */
		TArray<FString> Applied;

		BEFORE_EACH()
		{
			Spawner.InitializeGameSubsystems();
			Catalog = UVestigeContentCatalog::Get(&Spawner.GetWorld());
			Clock = UVestigeClockSubsystem::Get(&Spawner.GetWorld());
			ASSERT_THAT(IsNotNull(Catalog));
			ASSERT_THAT(IsNotNull(Clock));
			Milestones = UVestigeMilestoneSubsystem::Get(&Spawner.GetWorld());
			ASSERT_THAT(IsNotNull(Milestones));

			PlayerState = &Spawner.SpawnActor<AVestigePlayerState>();
			PlayerState->DispatchBeginPlay();
			Talk = PlayerState->FindComponentByClass<UVestigeConversationComponent>();
			Relationships = PlayerState->FindComponentByClass<UVestigeRelationshipComponent>();
			ASSERT_THAT(IsNotNull(Talk));
			ASSERT_THAT(IsNotNull(Relationships));

			Speaker = &Spawner.SpawnObject<UVestigeCharacterData>();
			Speaker->SetContentIdForTests(TestSpeakerId);
			Speaker->SetDisplayNameForTests(FText::AsCultureInvariant(TEXT("Tester")));
			ASSERT_THAT(IsTrue(Catalog->AddForTests(*Speaker)));
			// A root, so a test can place the speaker.
			SpeakerActor = &Spawner.SpawnActor<AActor>();
			USceneComponent* SpeakerRoot = NewObject<USceneComponent>(SpeakerActor);
			SpeakerActor->SetRootComponent(SpeakerRoot);
			SpeakerRoot->RegisterComponent();
			SetClock(1, MorningHour);
		}

		void SetClock(int32 Day, float Hour)
		{
			FVestigeClockSaveData Data;
			Data.Day = Day;
			Data.TimeOfDay = Hour;
			Clock->RestoreFromSaveData(Data);
		}

		/** Adds a conversation of the test speaker to the catalogue. */
		UVestigeDialogueData& AddDialogue(const TCHAR* Id, int32 Priority, TArray<FVestigeDialogueLine> Lines,
			TArray<TObjectPtr<UVestigeDialogueCondition>> Conditions = {}, const FPrimaryAssetId& ForSpeaker = FPrimaryAssetId())
		{
			UVestigeDialogueData& Dialogue = Spawner.SpawnObject<UVestigeDialogueData>();
			Dialogue.SetContentIdForTests(Id);
			Dialogue.SetSpeakerForTests(ForSpeaker.IsValid() ? ForSpeaker : Speaker->GetPrimaryAssetId());
			Dialogue.SetPriorityForTests(Priority);
			Dialogue.SetConditionsForTests(MoveTemp(Conditions));
			Dialogue.SetLinesForTests(MoveTemp(Lines));
			Catalog->AddForTests(Dialogue);
			return Dialogue;
		}

		UVestigeDialogueCondition_HasMet* MakeHasMet(bool bMet)
		{
			UVestigeDialogueCondition_HasMet& Condition = Spawner.SpawnObject<UVestigeDialogueCondition_HasMet>();
			Condition.SetMetForTests(bMet);
			return &Condition;
		}

		UVestigeDialogueCondition_TalkedToday* MakeTalkedToday(bool bTalked)
		{
			UVestigeDialogueCondition_TalkedToday& Condition = Spawner.SpawnObject<UVestigeDialogueCondition_TalkedToday>();
			Condition.SetTalkedForTests(bTalked);
			return &Condition;
		}

		UVestigeDialogueCondition_TimeOfDay* MakeBand(float FromHour, float ToHour)
		{
			UVestigeDialogueCondition_TimeOfDay& Condition = Spawner.SpawnObject<UVestigeDialogueCondition_TimeOfDay>();
			Condition.SetBandForTests(FromHour, ToHour);
			return &Condition;
		}

		UVestigeTestDialogueOutcome* MakeOutcome(const TCHAR* Name)
		{
			UVestigeTestDialogueOutcome& Outcome = Spawner.SpawnObject<UVestigeTestDialogueOutcome>();
			Outcome.Setup(Name, Applied);
			return &Outcome;
		}

		bool Start() { return Talk->StartConversation(*SpeakerActor, *Speaker); }

		bool LineIs(const TCHAR* Text) const { return Talk->GetLineText().ToString() == Text; }

		FVestigeDialogueContext MakeContext(float TimeOfDay, bool bMet, int32 TalksToday) const
		{
			FVestigeDialogueContext Context;
			Context.Speaker = Speaker;
			Context.Day = 1;
			Context.TimeOfDay = TimeOfDay;
			Context.bHasMet = bMet;
			Context.TalksToday = TalksToday;
			return Context;
		}

		/** The key of the line the talk shows now, when it comes from a string table. */
		FName LineKey() const { return GetTableKey(Talk->GetLineText()); }

		// ---- Choosing ----

		TEST_METHOD(TheHighestPriorityTalkWhoseConditionsHoldWins)
		{
			AddDialogue(TEXT("Test.Low"), 10, {MakeLine(TEXT("Low"), TEXT("low"))});
			AddDialogue(TEXT("Test.High"), 50, {MakeLine(TEXT("High"), TEXT("high"))});
			AddDialogue(TEXT("Test.Middle"), 30, {MakeLine(TEXT("Middle"), TEXT("middle"))});

			ASSERT_THAT(IsTrue(Start()));

			ASSERT_THAT(IsTrue(LineIs(TEXT("high"))));
		}

		TEST_METHOD(ATalkWhoseConditionFailsIsSkipped)
		{
			AddDialogue(TEXT("Test.Low"), 10, {MakeLine(TEXT("Low"), TEXT("low"))});
			AddDialogue(TEXT("Test.NeedsMet"), 50, {MakeLine(TEXT("Met"), TEXT("met"))}, {MakeHasMet(true)});

			ASSERT_THAT(IsTrue(Start()));

			ASSERT_THAT(IsTrue(LineIs(TEXT("low"))));
		}

		TEST_METHOD(ATalkWithAFailingConditionAmongSeveralIsSkipped)
		{
			// Every condition must hold, not one of them.
			AddDialogue(TEXT("Test.Low"), 10, {MakeLine(TEXT("Low"), TEXT("low"))});
			AddDialogue(TEXT("Test.Both"), 50, {MakeLine(TEXT("Both"), TEXT("both"))}, {MakeHasMet(false), MakeTalkedToday(true)});

			ASSERT_THAT(IsTrue(Start()));

			ASSERT_THAT(IsTrue(LineIs(TEXT("low"))));
		}

		TEST_METHOD(ATieAtOnePriorityPicksTheLowerIdAndWarns)
		{
			// Added in the other order, so the pick can't be the one added first.
			AddDialogue(TEXT("Test.TieB"), 20, {MakeLine(TEXT("B"), TEXT("b"))});
			AddDialogue(TEXT("Test.TieA"), 20, {MakeLine(TEXT("A"), TEXT("a"))});
			TestRunner->AddExpectedMessagePlain(TEXT("both apply"), ELogVerbosity::Warning);

			ASSERT_THAT(IsTrue(Start()));

			ASSERT_THAT(IsTrue(LineIs(TEXT("a"))));
		}

		TEST_METHOD(ATalkForAnotherSpeakerIsNeverChosen)
		{
			const FPrimaryAssetId Other(UVestigeCharacterData::AssetType, TEXT("Test.Other"));
			AddDialogue(TEXT("Test.Mine"), 10, {MakeLine(TEXT("Mine"), TEXT("mine"))});
			AddDialogue(TEXT("Test.Theirs"), 99, {MakeLine(TEXT("Theirs"), TEXT("theirs"))}, {}, Other);

			ASSERT_THAT(IsTrue(Start()));

			ASSERT_THAT(IsTrue(LineIs(TEXT("mine"))));
		}

		TEST_METHOD(WithNoTalkTheStartIsRefusedAndReported)
		{
			TArray<FGameplayTag> Reported;
			UVestigeFeedbackSubsystem* Feedback = Spawner.GetWorld().GetSubsystem<UVestigeFeedbackSubsystem>();
			ASSERT_THAT(IsNotNull(Feedback));
			Feedback->OnFeedback.AddLambda(
				[&Reported](const FVestigeFeedbackReport& Report)
				{
					Reported.Add(Report.Tag);
				});
			TestRunner->AddExpectedMessagePlain(TEXT("has no conversation to play"), ELogVerbosity::Warning);

			ASSERT_THAT(IsFalse(Start()));

			ASSERT_THAT(IsFalse(Talk->IsInConversation()));
			ASSERT_THAT(IsFalse(Clock->IsHeld()));
			ASSERT_THAT(IsFalse(Relationships->HasMet(Speaker->GetPrimaryAssetId())));
			ASSERT_THAT(IsTrue(Reported.Num() == 1 && Reported[0] == VestigeGameplayTags::Feedback_Dialogue_NothingToSay));
		}

		// ---- Conditions ----

		TEST_METHOD(TheTimeOfDayBandIncludesItsStartAndNotItsEnd)
		{
			const UVestigeDialogueCondition_TimeOfDay* MorningBand = MakeBand(6.f, 12.f);

			ASSERT_THAT(IsTrue(MorningBand->IsMet(MakeContext(6.f, false, 0))));
			ASSERT_THAT(IsTrue(MorningBand->IsMet(MakeContext(11.99f, false, 0))));
			ASSERT_THAT(IsFalse(MorningBand->IsMet(MakeContext(12.f, false, 0))));
			ASSERT_THAT(IsFalse(MorningBand->IsMet(MakeContext(5.99f, false, 0))));
		}

		TEST_METHOD(AfterMidnightIsInTheLateBand)
		{
			const UVestigeDialogueCondition_TimeOfDay* Late = MakeBand(24.f, 30.f);

			ASSERT_THAT(IsTrue(Late->IsMet(MakeContext(25.5f, false, 0))));
			ASSERT_THAT(IsTrue(Late->IsMet(MakeContext(24.f, false, 0))));
			ASSERT_THAT(IsFalse(Late->IsMet(MakeContext(23.5f, false, 0))));
			ASSERT_THAT(IsFalse(Late->IsMet(MakeContext(30.f, false, 0))));
		}

		TEST_METHOD(HasMetAndTalkedTodayReadTheContext)
		{
			const UVestigeDialogueCondition_HasMet* MustHaveMet = MakeHasMet(true);
			const UVestigeDialogueCondition_HasMet* MustBeStrangers = MakeHasMet(false);
			ASSERT_THAT(IsTrue(MustHaveMet->IsMet(MakeContext(8.f, true, 0))));
			ASSERT_THAT(IsFalse(MustHaveMet->IsMet(MakeContext(8.f, false, 0))));
			ASSERT_THAT(IsTrue(MustBeStrangers->IsMet(MakeContext(8.f, false, 0))));
			ASSERT_THAT(IsFalse(MustBeStrangers->IsMet(MakeContext(8.f, true, 0))));

			const UVestigeDialogueCondition_TalkedToday* MustHaveTalked = MakeTalkedToday(true);
			const UVestigeDialogueCondition_TalkedToday* MustNotHaveTalked = MakeTalkedToday(false);
			ASSERT_THAT(IsTrue(MustHaveTalked->IsMet(MakeContext(8.f, true, 1))));
			ASSERT_THAT(IsTrue(MustHaveTalked->IsMet(MakeContext(8.f, true, 3))));
			ASSERT_THAT(IsFalse(MustHaveTalked->IsMet(MakeContext(8.f, true, 0))));
			ASSERT_THAT(IsTrue(MustNotHaveTalked->IsMet(MakeContext(8.f, true, 0))));
			ASSERT_THAT(IsFalse(MustNotHaveTalked->IsMet(MakeContext(8.f, true, 1))));
		}

		TEST_METHOD(TheConversationFillsTheContextFromTheClockAndTheRelationship)
		{
			// A talk that needs the 8 o'clock band on day 3, after one earlier talk that day.
			SetClock(3, 20.5f);
			Relationships->RecordTalk(Speaker->GetPrimaryAssetId(), 3);
			AddDialogue(TEXT("Test.Evening"), 50, {MakeLine(TEXT("Evening"), TEXT("evening"))},
				{MakeBand(18.f, 24.f), MakeHasMet(true), MakeTalkedToday(true)});
			AddDialogue(TEXT("Test.Fallback"), 10, {MakeLine(TEXT("Fallback"), TEXT("fallback"))});

			ASSERT_THAT(IsTrue(Start()));

			ASSERT_THAT(IsTrue(LineIs(TEXT("evening"))));
		}

		// ---- Running ----

		TEST_METHOD(ALineWithoutNextGoesToTheFollowingLine)
		{
			AddDialogue(TEXT("Test.Talk"), 10, {MakeLine(TEXT("One"), TEXT("one")), MakeLine(TEXT("Two"), TEXT("two"))});
			ASSERT_THAT(IsTrue(Start()));
			ASSERT_THAT(IsTrue(LineIs(TEXT("one"))));
			ASSERT_THAT(IsFalse(Talk->IsLastLine()));

			Talk->Advance();

			ASSERT_THAT(IsTrue(LineIs(TEXT("two"))));
			ASSERT_THAT(IsTrue(Talk->IsLastLine()));
		}

		TEST_METHOD(ALineWithNextJumpsToThatLine)
		{
			AddDialogue(TEXT("Test.Talk"), 10,
				{MakeLine(TEXT("One"), TEXT("one"), TEXT("Three")), MakeLine(TEXT("Two"), TEXT("two")), MakeLine(TEXT("Three"), TEXT("three"))});
			ASSERT_THAT(IsTrue(Start()));

			Talk->Advance();

			ASSERT_THAT(IsTrue(LineIs(TEXT("three"))));
		}

		TEST_METHOD(TheLastLineEndsTheTalk)
		{
			AddDialogue(TEXT("Test.Talk"), 10, {MakeLine(TEXT("One"), TEXT("one"))});
			ASSERT_THAT(IsTrue(Start()));
			int32 Ended = 0;
			Talk->OnConversationEnded.AddLambda(
				[&Ended]()
				{
					++Ended;
				});

			Talk->Advance();

			ASSERT_THAT(IsFalse(Talk->IsInConversation()));
			ASSERT_THAT(IsTrue(Ended == 1));
			ASSERT_THAT(IsTrue(Talk->GetLineText().IsEmpty()));
			ASSERT_THAT(IsNull(Talk->GetSpeakerActor()));
		}

		TEST_METHOD(AChoiceGoesToItsNextLine)
		{
			FVestigeDialogueLine Question = MakeLine(TEXT("Ask"), TEXT("ask"));
			Question.Choices = {MakeChoice(TEXT("Yes"), TEXT("yes"), TEXT("YesLine")), MakeChoice(TEXT("No"), TEXT("no"), TEXT("NoLine"))};
			AddDialogue(TEXT("Test.Talk"), 10, {Question, MakeLine(TEXT("YesLine"), TEXT("said yes")), MakeLine(TEXT("NoLine"), TEXT("said no"))});
			ASSERT_THAT(IsTrue(Start()));
			ASSERT_THAT(IsTrue(Talk->GetChoiceTexts().Num() == 2));
			ASSERT_THAT(IsTrue(Talk->GetChoiceTexts()[1].ToString() == TEXT("no")));

			Talk->Choose(1);

			ASSERT_THAT(IsTrue(LineIs(TEXT("said no"))));
			ASSERT_THAT(IsTrue(Talk->GetChoiceTexts().IsEmpty()));
		}

		TEST_METHOD(AdvanceDoesNothingWhileChoicesShow)
		{
			FVestigeDialogueLine Question = MakeLine(TEXT("Ask"), TEXT("ask"));
			Question.Choices = {MakeChoice(TEXT("Yes"), TEXT("yes"), TEXT("YesLine"))};
			AddDialogue(TEXT("Test.Talk"), 10, {Question, MakeLine(TEXT("YesLine"), TEXT("said yes"))});
			ASSERT_THAT(IsTrue(Start()));

			Talk->Advance();

			ASSERT_THAT(IsTrue(LineIs(TEXT("ask"))));
			ASSERT_THAT(IsTrue(Talk->IsInConversation()));
			ASSERT_THAT(IsFalse(Talk->IsLastLine()));
		}

		TEST_METHOD(ChoosingWithABadIndexIsIgnored)
		{
			FVestigeDialogueLine Question = MakeLine(TEXT("Ask"), TEXT("ask"));
			Question.Choices = {MakeChoice(TEXT("Yes"), TEXT("yes"), TEXT("YesLine"))};
			AddDialogue(TEXT("Test.Talk"), 10, {Question, MakeLine(TEXT("YesLine"), TEXT("said yes"))});
			ASSERT_THAT(IsTrue(Start()));

			Talk->Choose(-1);
			Talk->Choose(1);

			ASSERT_THAT(IsTrue(LineIs(TEXT("ask"))));
			ASSERT_THAT(IsTrue(Talk->IsInConversation()));
		}

		TEST_METHOD(AChoiceWhoseConditionFailsIsNotListed)
		{
			// The talk marks the speaker met as it starts, so a reply for strangers is already hidden when the replies are listed.
			FVestigeDialogueLine Question = MakeLine(TEXT("Ask"), TEXT("ask"));
			FVestigeDialogueChoice Hidden = MakeChoice(TEXT("Hidden"), TEXT("hidden"), TEXT("HiddenLine"));
			Hidden.Conditions = {MakeHasMet(false)};
			Question.Choices = {Hidden, MakeChoice(TEXT("Shown"), TEXT("shown"), TEXT("ShownLine"))};
			AddDialogue(TEXT("Test.Talk"), 10,
				{Question, MakeLine(TEXT("HiddenLine"), TEXT("went hidden")), MakeLine(TEXT("ShownLine"), TEXT("went shown"))});
			ASSERT_THAT(IsTrue(Start()));

			// Listed replies are numbered without the hidden one, so 0 is the second choice of the data.
			ASSERT_THAT(IsTrue(Talk->GetChoiceTexts().Num() == 1));
			ASSERT_THAT(IsTrue(Talk->GetChoiceTexts()[0].ToString() == TEXT("shown")));
			Talk->Choose(0);

			ASSERT_THAT(IsTrue(LineIs(TEXT("went shown"))));
		}

		TEST_METHOD(ALinesOutcomesApplyWhenShownAndAChoicesWhenChosen)
		{
			FVestigeDialogueLine Question = MakeLine(TEXT("Ask"), TEXT("ask"));
			Question.Outcomes = {MakeOutcome(TEXT("line"))};
			FVestigeDialogueChoice Choice = MakeChoice(TEXT("Yes"), TEXT("yes"), TEXT("After"));
			Choice.Outcomes = {MakeOutcome(TEXT("choice"))};
			Question.Choices = {Choice};
			FVestigeDialogueLine After = MakeLine(TEXT("After"), TEXT("after"));
			After.Outcomes = {MakeOutcome(TEXT("after"))};
			AddDialogue(TEXT("Test.Talk"), 10, {Question, After});

			ASSERT_THAT(IsTrue(Start()));
			ASSERT_THAT(IsTrue(Applied == TArray<FString>{TEXT("line")}));

			Talk->Choose(0);

			// The choice's outcome comes before the next line's.
			ASSERT_THAT(IsTrue(Applied == TArray<FString>{TEXT("line"), TEXT("choice"), TEXT("after")}));
		}

		TEST_METHOD(AChoiceToALineThatDoesntExistEndsTheTalk)
		{
			FVestigeDialogueLine Question = MakeLine(TEXT("Ask"), TEXT("ask"));
			Question.Choices = {MakeChoice(TEXT("Lost"), TEXT("lost"), TEXT("Nowhere"))};
			AddDialogue(TEXT("Test.Talk"), 10, {Question});
			ASSERT_THAT(IsTrue(Start()));
			TestRunner->AddExpectedMessagePlain(TEXT("which it has no line for"), ELogVerbosity::Error);

			Talk->Choose(0);

			ASSERT_THAT(IsFalse(Talk->IsInConversation()));
			ASSERT_THAT(IsFalse(Clock->IsHeld()));
		}

		TEST_METHOD(EndingEarlyEndsTheTalk)
		{
			AddDialogue(TEXT("Test.Talk"), 10, {MakeLine(TEXT("One"), TEXT("one")), MakeLine(TEXT("Two"), TEXT("two"))});
			ASSERT_THAT(IsTrue(Start()));
			int32 Ended = 0;
			Talk->OnConversationEnded.AddLambda(
				[&Ended]()
				{
					++Ended;
				});

			Talk->EndConversation();
			Talk->EndConversation();

			ASSERT_THAT(IsFalse(Talk->IsInConversation()));
			ASSERT_THAT(IsTrue(Ended == 1));
		}

		TEST_METHOD(ATalkCannotStartDuringAnother)
		{
			AddDialogue(TEXT("Test.Talk"), 10, {MakeLine(TEXT("One"), TEXT("one")), MakeLine(TEXT("Two"), TEXT("two"))});
			ASSERT_THAT(IsTrue(Start()));
			Talk->Advance();
			AActor& Other = Spawner.SpawnActor<AActor>();

			ASSERT_THAT(IsFalse(Talk->StartConversation(Other, *Speaker)));

			ASSERT_THAT(IsTrue(Talk->GetSpeakerActor() == SpeakerActor));
			ASSERT_THAT(IsTrue(LineIs(TEXT("two"))));
		}

		TEST_METHOD(ATalkCannotStartWhileUsingATool)
		{
			// The game's player state has the ability system the tool state lives on.
			UClass* PlayerStateClass =
				LoadClass<AVestigePlayerState>(nullptr, TEXT("/Game/Vestige/Blueprints/BP_PlayerState.BP_PlayerState_C"));
			ASSERT_THAT(IsNotNull(PlayerStateClass));
			AVestigePlayerState& GamePlayer = Spawner.SpawnActor<AVestigePlayerState>(FActorSpawnParameters(), PlayerStateClass);
			GamePlayer.DispatchBeginPlay();
			UVestigeConversationComponent* GameTalk = GamePlayer.FindComponentByClass<UVestigeConversationComponent>();
			UAbilitySystemComponent* AbilitySystem = GamePlayer.GetAbilitySystemComponent();
			ASSERT_THAT(IsNotNull(GameTalk));
			ASSERT_THAT(IsNotNull(AbilitySystem));
			AddDialogue(TEXT("Test.Talk"), 10, {MakeLine(TEXT("One"), TEXT("one"))});
			AbilitySystem->AddLooseGameplayTag(VestigeGameplayTags::State_UsingTool);

			ASSERT_THAT(IsFalse(GameTalk->StartConversation(*SpeakerActor, *Speaker)));
			ASSERT_THAT(IsFalse(GameTalk->IsInConversation()));
			ASSERT_THAT(IsFalse(Clock->IsHeld()));

			AbilitySystem->RemoveLooseGameplayTag(VestigeGameplayTags::State_UsingTool);

			ASSERT_THAT(IsTrue(GameTalk->StartConversation(*SpeakerActor, *Speaker)));
		}

		// ---- Pools ----

		TEST_METHOD(ThePoolNeverRepeatsYesterdaysGreeting)
		{
			FVestigeDialogueLine Greeting;
			Greeting.LineId = TEXT("Greet");
			Greeting.Texts = {FText::AsCultureInvariant(TEXT("a")), FText::AsCultureInvariant(TEXT("b")), FText::AsCultureInvariant(TEXT("c"))};

			// The day's first talk, on consecutive days, over more than a cycle of the pool.
			for (int32 Day = 1; Day < 12; ++Day)
			{
				const FString Today = Greeting.PickText(Day, 0).ToString();
				const FString Tomorrow = Greeting.PickText(Day + 1, 0).ToString();
				ASSERT_THAT(IsTrue(Today != Tomorrow));
			}
		}

		TEST_METHOD(ThePoolNeverRepeatsTheLastTalkToday)
		{
			FVestigeDialogueLine Again;
			Again.LineId = TEXT("Again");
			Again.Texts = {FText::AsCultureInvariant(TEXT("a")), FText::AsCultureInvariant(TEXT("b")), FText::AsCultureInvariant(TEXT("c"))};
			AddDialogue(TEXT("Test.Pool"), 10, {Again});

			// Four talks in one day, each after the one before it.
			FString Last;
			for (int32 Talks = 0; Talks < 4; ++Talks)
			{
				ASSERT_THAT(IsTrue(Start()));
				const FString Shown = Talk->GetLineText().ToString();
				ASSERT_THAT(IsTrue(Shown != Last));
				Last = Shown;
				Talk->EndConversation();
			}
		}

		// ---- The clock ----

		TEST_METHOD(TheClockIsHeldDuringATalkAndReleasedAfter)
		{
			AddDialogue(TEXT("Test.Talk"), 10, {MakeLine(TEXT("One"), TEXT("one")), MakeLine(TEXT("Two"), TEXT("two"))});
			ASSERT_THAT(IsFalse(Clock->IsHeld()));

			ASSERT_THAT(IsTrue(Start()));
			ASSERT_THAT(IsTrue(Clock->IsHeld()));
			ASSERT_THAT(IsFalse(Clock->IsTickable()));
			Talk->Advance();
			ASSERT_THAT(IsTrue(Clock->IsHeld()));

			Talk->Advance();

			ASSERT_THAT(IsFalse(Clock->IsHeld()));
		}

		TEST_METHOD(EscapeReleasesTheClock)
		{
			AddDialogue(TEXT("Test.Talk"), 10, {MakeLine(TEXT("One"), TEXT("one")), MakeLine(TEXT("Two"), TEXT("two"))});
			ASSERT_THAT(IsTrue(Start()));

			Talk->EndConversation();

			ASSERT_THAT(IsFalse(Clock->IsHeld()));
		}

		TEST_METHOD(EndPlayReleasesTheClock)
		{
			AddDialogue(TEXT("Test.Talk"), 10, {MakeLine(TEXT("One"), TEXT("one"))});
			ASSERT_THAT(IsTrue(Start()));
			ASSERT_THAT(IsTrue(Clock->IsHeld()));

			PlayerState->Destroy();

			ASSERT_THAT(IsFalse(Clock->IsHeld()));
		}

		// ---- Relationships ----

		TEST_METHOD(StartingATalkMarksTheCharacterMet)
		{
			AddDialogue(TEXT("Test.Talk"), 10, {MakeLine(TEXT("One"), TEXT("one")), MakeLine(TEXT("Two"), TEXT("two"))});
			const FPrimaryAssetId Id = Speaker->GetPrimaryAssetId();
			ASSERT_THAT(IsFalse(Relationships->HasMet(Id)));

			ASSERT_THAT(IsTrue(Start()));

			// Met from the start, so a first meeting cut short still counts.
			ASSERT_THAT(IsTrue(Relationships->HasMet(Id)));
			ASSERT_THAT(IsTrue(Relationships->GetTalksToday(Id, 1) == 1));
			Talk->EndConversation();
			ASSERT_THAT(IsTrue(Relationships->HasMet(Id)));
		}

		TEST_METHOD(TalksTodayCountPerCharacterAndResetOnANewDay)
		{
			const FPrimaryAssetId Id = Speaker->GetPrimaryAssetId();
			const FPrimaryAssetId Other(UVestigeCharacterData::AssetType, TEXT("Test.Other"));

			Relationships->RecordTalk(Id, 4);
			Relationships->RecordTalk(Id, 4);
			Relationships->RecordTalk(Other, 4);

			ASSERT_THAT(IsTrue(Relationships->GetTalksToday(Id, 4) == 2));
			ASSERT_THAT(IsTrue(Relationships->GetTalksToday(Other, 4) == 1));
			ASSERT_THAT(IsTrue(Relationships->GetTalksToday(Id, 5) == 0));
			ASSERT_THAT(IsTrue(Relationships->GetTalksToday(Id, 3) == 0));

			Relationships->RecordTalk(Id, 5);

			ASSERT_THAT(IsTrue(Relationships->GetTalksToday(Id, 5) == 1));
			ASSERT_THAT(IsTrue(Relationships->HasMet(Id)));
		}

		TEST_METHOD(MetIsSavedAndTalksTodayAreNot)
		{
			const FPrimaryAssetId Id = Speaker->GetPrimaryAssetId();
			Relationships->RecordTalk(Id, 4);
			Relationships->RecordTalk(Id, 4);
			ASSERT_THAT(IsTrue(Relationships->GetSaveKey() == TEXT("Base.Relationships")));
			ASSERT_THAT(IsTrue(Relationships->GetSaveVersion() == FVestigeRelationshipSaveData::Version));
			const FVestigeSaveRecord Record = Relationships->WriteSaveRecord();

			AVestigePlayerState& Fresh = Spawner.SpawnActor<AVestigePlayerState>();
			Fresh.DispatchBeginPlay();
			UVestigeRelationshipComponent* Loaded = Fresh.FindComponentByClass<UVestigeRelationshipComponent>();
			ASSERT_THAT(IsNotNull(Loaded));
			int32 LeftOut = 0;
			ASSERT_THAT(IsTrue(Loaded->ReadSaveRecord(Record, LeftOut)));

			ASSERT_THAT(IsTrue(Loaded->HasMet(Id)));
			ASSERT_THAT(IsTrue(Loaded->GetTalksToday(Id, 4) == 0));
			ASSERT_THAT(IsTrue(LeftOut == 0));
		}

		TEST_METHOD(AnUnknownSavedCharacterIsSkipped)
		{
			FVestigeRelationshipSaveData Data;
			FVestigeCharacterRelationshipSaveData& Gone = Data.Characters.AddDefaulted_GetRef();
			Gone.Character = TEXT("Gone.Nobody");
			Gone.bMet = true;
			FVestigeCharacterRelationshipSaveData& Known = Data.Characters.AddDefaulted_GetRef();
			Known.Character = TestSpeakerId;
			Known.bMet = true;
			TestRunner->AddExpectedMessagePlain(TEXT("no longer exists"), ELogVerbosity::Warning);

			int32 LeftOut = 0;
			ASSERT_THAT(IsTrue(Relationships->ReadSaveRecord(FVestigeSaveRecord::Write(Data), LeftOut)));

			ASSERT_THAT(IsTrue(LeftOut == 1));
			ASSERT_THAT(IsTrue(Relationships->HasMet(Speaker->GetPrimaryAssetId())));
			ASSERT_THAT(IsFalse(Relationships->HasMet(FPrimaryAssetId(UVestigeCharacterData::AssetType, TEXT("Gone.Nobody")))));
		}

		TEST_METHOD(ALoadReplacesWhatWasKnown)
		{
			const FPrimaryAssetId Id = Speaker->GetPrimaryAssetId();
			Relationships->RecordTalk(Id, 2);
			int32 LeftOut = 0;

			ASSERT_THAT(IsTrue(Relationships->ReadSaveRecord(FVestigeSaveRecord::Write(FVestigeRelationshipSaveData()), LeftOut)));

			ASSERT_THAT(IsFalse(Relationships->HasMet(Id)));
			ASSERT_THAT(IsTrue(Relationships->GetTalksToday(Id, 2) == 0));
		}

		TEST_METHOD(ANewGameForgetsEveryone)
		{
			const FPrimaryAssetId Id = Speaker->GetPrimaryAssetId();
			Relationships->RecordTalk(Id, 2);

			Relationships->StartNewGame();

			ASSERT_THAT(IsFalse(Relationships->HasMet(Id)));
			ASSERT_THAT(IsTrue(Relationships->GetTalksToday(Id, 2) == 0));
		}

		// ---- The end of a talk, and the replies listed ----

		TEST_METHOD(TheReservedEndNextEndsTheTalkAtOnce)
		{
			AddDialogue(TEXT("Test.End"), 10,
				{MakeLine(TEXT("First"), TEXT("first"), VestigeDialogue::EndTalk), MakeLine(TEXT("Second"), TEXT("second"))});
			ASSERT_THAT(IsTrue(Start()));
			ASSERT_THAT(IsTrue(LineIs(TEXT("first"))));

			Talk->Advance();

			ASSERT_THAT(IsFalse(Talk->IsInConversation()));
			ASSERT_THAT(IsFalse(Clock->IsHeld()));
		}

		TEST_METHOD(AReplyWhoseNextIsTheReservedEndEndsTheTalk)
		{
			FVestigeDialogueLine Question = MakeLine(TEXT("Ask"), TEXT("ask"));
			Question.Choices.Add(MakeChoice(TEXT("Done"), TEXT("done"), VestigeDialogue::EndTalk));
			AddDialogue(TEXT("Test.EndReply"), 10, {Question, MakeLine(TEXT("Never"), TEXT("never"))});
			ASSERT_THAT(IsTrue(Start()));

			Talk->Choose(0);

			ASSERT_THAT(IsFalse(Talk->IsInConversation()));
		}

		TEST_METHOD(MoreThanThreeHoldingRepliesListTheFirstThreeAndWarn)
		{
			FVestigeDialogueLine Question = MakeLine(TEXT("Ask"), TEXT("ask"));
			for (const TCHAR* Name : {TEXT("A"), TEXT("B"), TEXT("C"), TEXT("D")})
			{
				Question.Choices.Add(MakeChoice(Name, Name, TEXT("Ask")));
			}
			AddDialogue(TEXT("Test.Four"), 10, {Question});
			TestRunner->AddExpectedMessagePlain(TEXT("replies holding at once"), ELogVerbosity::Warning);

			ASSERT_THAT(IsTrue(Start()));

			const TArray<FText> Replies = Talk->GetChoiceTexts();
			ASSERT_THAT(IsTrue(Replies.Num() == VestigeDialogue::MaxChoices));
			ASSERT_THAT(IsTrue(Replies[2].ToString() == TEXT("C")));
		}

		/** A dialogue of Hob that is not in the catalogue, for the validation tests. */
		UVestigeDialogueData& MakeDialogueToValidate(TArray<FVestigeDialogueLine> Lines)
		{
			UVestigeDialogueData& Dialogue = Spawner.SpawnObject<UVestigeDialogueData>();
			Dialogue.SetContentIdForTests(TEXT("Test.Validate"));
			Dialogue.SetSpeakerForTests(HobId());
			Dialogue.SetLinesForTests(MoveTemp(Lines));
			return Dialogue;
		}

		TEST_METHOD(ALineMayHoldMoreThanThreeRepliesOnlyWhenTheExtraOnesHaveConditions)
		{
			UVestigeDialogueCondition_Flag& Condition = Spawner.SpawnObject<UVestigeDialogueCondition_Flag>();
			Condition.SetForTests(MakeFlag(TEXT("Asked")), true);
			FVestigeDialogueLine Question = MakeLine(TEXT("Ask"), TEXT("ask"));
			for (const TCHAR* Name : {TEXT("A"), TEXT("B"), TEXT("C"), TEXT("D")})
			{
				Question.Choices.Add(MakeChoice(Name, Name, TEXT("Ask")));
			}
			Question.Choices[3].Conditions.Add(&Condition);

			FDataValidationContext WithCondition;
			const EDataValidationResult Allowed = MakeDialogueToValidate({Question}).IsDataValid(WithCondition);
			ASSERT_THAT(IsTrue(Allowed != EDataValidationResult::Invalid && WithCondition.GetNumErrors() == 0));

			Question.Choices[3].Conditions.Reset();
			FDataValidationContext WithoutCondition;
			const EDataValidationResult Refused = MakeDialogueToValidate({Question}).IsDataValid(WithoutCondition);
			ASSERT_THAT(IsTrue(Refused == EDataValidationResult::Invalid && WithoutCondition.GetNumErrors() >= 1));
		}

		TEST_METHOD(TheReservedEndIsAValidNext)
		{
			FVestigeDialogueLine Question = MakeLine(TEXT("Ask"), TEXT("ask"), VestigeDialogue::EndTalk);
			Question.Choices.Add(MakeChoice(TEXT("Done"), TEXT("done"), VestigeDialogue::EndTalk));
			FDataValidationContext Context;

			const EDataValidationResult Result = MakeDialogueToValidate({Question}).IsDataValid(Context);

			ASSERT_THAT(IsTrue(Result != EDataValidationResult::Invalid && Context.GetNumErrors() == 0));
		}

		// ---- Trust conditions and outcomes ----

		TEST_METHOD(TheTrustConditionHoldsFromItsThreshold)
		{
			UVestigeDialogueCondition_Trust& Condition = Spawner.SpawnObject<UVestigeDialogueCondition_Trust>();
			Condition.SetMinTrustForTests(3);
			FVestigeDialogueContext Context = MakeContext(8.f, true, 0);

			Context.Trust = 2;
			ASSERT_THAT(IsFalse(Condition.IsMet(Context)));
			Context.Trust = 3;
			ASSERT_THAT(IsTrue(Condition.IsMet(Context)));
			Context.Trust = 12;
			ASSERT_THAT(IsTrue(Condition.IsMet(Context)));
		}

		TEST_METHOD(TheFlagConditionCountsDaysSinceItWasSet)
		{
			const FGameplayTag Today = MakeFlag(TEXT("Asked"));
			const FGameplayTag Yesterday = MakeFlag(TEXT("Step.Attentive"));
			const FGameplayTag Long = MakeFlag(TEXT("Step.WouldLike"));
			const FGameplayTag Never = MakeFlag(TEXT("Step.Memory"));
			FVestigeDialogueContext Context = MakeContext(8.f, true, 0);
			Context.Day = 10;
			Context.FlagDays = {{Today, 10}, {Yesterday, 9}, {Long, 5}};
			const auto Holds = [this, &Context](const FGameplayTag& Flag, bool bSet, int32 MinDays, int32 MaxDays)
			{
				UVestigeDialogueCondition_Flag& Condition = Spawner.SpawnObject<UVestigeDialogueCondition_Flag>();
				Condition.SetForTests(Flag, bSet, MinDays, MaxDays);
				return Condition.IsMet(Context);
			};

			// Set, with no limit: the day it was set does not matter.
			ASSERT_THAT(IsTrue(Holds(Today, true, 0, -1)));
			ASSERT_THAT(IsTrue(Holds(Long, true, 0, -1)));
			ASSERT_THAT(IsFalse(Holds(Never, true, 0, -1)));
			// Not set.
			ASSERT_THAT(IsFalse(Holds(Today, false, 0, -1)));
			ASSERT_THAT(IsTrue(Holds(Never, false, 0, -1)));
			// Set today only.
			ASSERT_THAT(IsTrue(Holds(Today, true, 0, 0)));
			ASSERT_THAT(IsFalse(Holds(Yesterday, true, 0, 0)));
			// Set at least a day ago.
			ASSERT_THAT(IsFalse(Holds(Today, true, 1, -1)));
			ASSERT_THAT(IsTrue(Holds(Yesterday, true, 1, -1)));
			ASSERT_THAT(IsTrue(Holds(Long, true, 1, -1)));
			// Set within a range of days (both limits).
			ASSERT_THAT(IsTrue(Holds(Yesterday, true, 0, 1)));
			ASSERT_THAT(IsFalse(Holds(Long, true, 0, 1)));
			ASSERT_THAT(IsTrue(Holds(Long, true, 4, 6)));
			// Not set in the last two days: a flag set longer ago counts as not set, one set within them as set.
			ASSERT_THAT(IsFalse(Holds(Today, false, 0, 2)));
			ASSERT_THAT(IsFalse(Holds(Yesterday, false, 0, 2)));
			ASSERT_THAT(IsTrue(Holds(Long, false, 0, 2)));
			ASSERT_THAT(IsTrue(Holds(Never, false, 0, 2)));
		}

		TEST_METHOD(TheDateConditionChecksTheDaySeasonAndRange)
		{
			const FGameplayTag Spring = MakeTag(TEXT("Vestige.Season.Spring"));
			const FGameplayTag Summer = MakeTag(TEXT("Vestige.Season.Summer"));
			const FGameplayTag NoSeason;
			FVestigeDialogueContext Context = MakeContext(8.f, true, 0);
			const auto Holds = [this, &Context](int32 Day, const FGameplayTag& Season, int32 DayOfSeason, int32 FromDay,
				const FGameplayTag& WantedSeason, int32 FromDayOfSeason, int32 ToDayOfSeason)
			{
				UVestigeDialogueCondition_Date& Condition = Spawner.SpawnObject<UVestigeDialogueCondition_Date>();
				Condition.SetForTests(FromDay, WantedSeason, FromDayOfSeason, ToDayOfSeason);
				Context.Day = Day;
				Context.Season = Season;
				Context.DayOfSeason = DayOfSeason;
				return Condition.IsMet(Context);
			};

			// From a day on, in any season.
			ASSERT_THAT(IsFalse(Holds(1, Spring, 1, 2, NoSeason, 1, 0)));
			ASSERT_THAT(IsTrue(Holds(2, Spring, 2, 2, NoSeason, 1, 0)));
			ASSERT_THAT(IsTrue(Holds(40, Summer, 12, 2, NoSeason, 1, 0)));
			// Spring days 19 to 25 only.
			ASSERT_THAT(IsFalse(Holds(18, Spring, 18, 2, Spring, 19, 25)));
			ASSERT_THAT(IsTrue(Holds(19, Spring, 19, 2, Spring, 19, 25)));
			ASSERT_THAT(IsTrue(Holds(25, Spring, 25, 2, Spring, 19, 25)));
			ASSERT_THAT(IsFalse(Holds(26, Spring, 26, 2, Spring, 19, 25)));
			ASSERT_THAT(IsFalse(Holds(49, Summer, 21, 2, Spring, 19, 25)));
			// 0 for the end of the range is the last day of the season.
			ASSERT_THAT(IsTrue(Holds(28, Spring, 28, 2, Spring, 19, 0)));
		}

		TEST_METHOD(TheMilestoneConditionReadsTheContext)
		{
			const FGameplayTag Harvested = VestigeGameplayTags::Milestone_Farm_Harvested;
			const FGameplayTag Sold = VestigeGameplayTags::Milestone_Farm_Sold;
			UVestigeDialogueCondition_Milestone& MustHaveHarvested = Spawner.SpawnObject<UVestigeDialogueCondition_Milestone>();
			MustHaveHarvested.SetForTests(Harvested, true);
			UVestigeDialogueCondition_Milestone& MustNotHaveSold = Spawner.SpawnObject<UVestigeDialogueCondition_Milestone>();
			MustNotHaveSold.SetForTests(Sold, false);
			FVestigeDialogueContext Context = MakeContext(8.f, true, 0);

			ASSERT_THAT(IsFalse(MustHaveHarvested.IsMet(Context)));
			ASSERT_THAT(IsTrue(MustNotHaveSold.IsMet(Context)));
			Context.Milestones.AddTag(Harvested);
			ASSERT_THAT(IsTrue(MustHaveHarvested.IsMet(Context)));
			ASSERT_THAT(IsTrue(MustNotHaveSold.IsMet(Context)));
			Context.Milestones.AddTag(Sold);
			ASSERT_THAT(IsFalse(MustNotHaveSold.IsMet(Context)));
		}

		TEST_METHOD(AddTrustAndSetFlagAskTheRelationship)
		{
			const FPrimaryAssetId Id = Speaker->GetPrimaryAssetId();
			FVestigeDialogueContext Context = MakeContext(8.f, true, 1);
			Context.Player = PlayerState;
			Context.Day = 4;
			UVestigeDialogueOutcome_AddTrust& AddTwo = Spawner.SpawnObject<UVestigeDialogueOutcome_AddTrust>();
			AddTwo.SetAmountForTests(2);
			UVestigeDialogueOutcome_SetFlag& SetAsked = Spawner.SpawnObject<UVestigeDialogueOutcome_SetFlag>();
			SetAsked.SetFlagForTests(MakeFlag(TEXT("Asked")));

			AddTwo.Apply(Context);
			AddTwo.Apply(Context);
			SetAsked.Apply(Context);

			ASSERT_THAT(IsTrue(Relationships->GetEarnedTrust(Id) == 4));
			ASSERT_THAT(IsTrue(Relationships->GetTrust(*Speaker) == 4));
			const int32* Day = Relationships->GetFlagDays(Id).Find(MakeFlag(TEXT("Asked")));
			ASSERT_THAT(IsTrue(Day && *Day == 4));

			// Setting it again moves the day.
			Context.Day = 6;
			SetAsked.Apply(Context);
			ASSERT_THAT(IsTrue(*Relationships->GetFlagDays(Id).Find(MakeFlag(TEXT("Asked"))) == 6));
		}

		TEST_METHOD(AnOutcomeWithoutAPlayerOrASpeakerDoesNothing)
		{
			const FPrimaryAssetId Id = Speaker->GetPrimaryAssetId();
			UVestigeDialogueOutcome_AddTrust& Add = Spawner.SpawnObject<UVestigeDialogueOutcome_AddTrust>();
			UVestigeDialogueOutcome_SetFlag& Set = Spawner.SpawnObject<UVestigeDialogueOutcome_SetFlag>();
			Set.SetFlagForTests(MakeFlag(TEXT("Asked")));
			FVestigeDialogueContext NoPlayer = MakeContext(8.f, true, 1);
			FVestigeDialogueContext NoSpeaker = MakeContext(8.f, true, 1);
			NoSpeaker.Player = PlayerState;
			NoSpeaker.Speaker = nullptr;

			Add.Apply(NoPlayer);
			Set.Apply(NoPlayer);
			Add.Apply(NoSpeaker);
			Set.Apply(NoSpeaker);

			ASSERT_THAT(IsTrue(Relationships->GetEarnedTrust(Id) == 0));
			ASSERT_THAT(IsTrue(Relationships->GetFlagDays(Id).IsEmpty()));
		}

		TEST_METHOD(TheContextCarriesTrustFlagsMilestonesAndTheDate)
		{
			// A talk that needs three trust, a flag, a milestone and a day from the third on, in spring.
			Speaker->SetTrustFromMilestonesForTests({{VestigeGameplayTags::Milestone_Farm_Harvested, 1}});
			UVestigeDialogueCondition_Trust& Trust = Spawner.SpawnObject<UVestigeDialogueCondition_Trust>();
			Trust.SetMinTrustForTests(3);
			UVestigeDialogueCondition_Flag& Flag = Spawner.SpawnObject<UVestigeDialogueCondition_Flag>();
			Flag.SetForTests(MakeFlag(TEXT("Asked")), true, 1);
			UVestigeDialogueCondition_Milestone& Milestone = Spawner.SpawnObject<UVestigeDialogueCondition_Milestone>();
			Milestone.SetForTests(VestigeGameplayTags::Milestone_Farm_Harvested, true);
			UVestigeDialogueCondition_Date& Date = Spawner.SpawnObject<UVestigeDialogueCondition_Date>();
			Date.SetForTests(3, MakeTag(TEXT("Vestige.Season.Spring")));
			AddDialogue(TEXT("Test.Needs"), 50, {MakeLine(TEXT("Needs"), TEXT("needs"))}, {&Trust, &Flag, &Milestone, &Date});
			AddDialogue(TEXT("Test.Fallback"), 10, {MakeLine(TEXT("Fallback"), TEXT("fallback"))});
			SetClock(3, MorningHour);
			Relationships->AddTrust(*Speaker, 2);
			Relationships->SetFlag(*Speaker, MakeFlag(TEXT("Asked")), 2);

			// Two points earned and the milestone not reached: the talk is not there yet.
			ASSERT_THAT(IsTrue(Start()));
			ASSERT_THAT(IsTrue(LineIs(TEXT("fallback"))));
			Talk->EndConversation();

			Milestones->Reach(VestigeGameplayTags::Milestone_Farm_Harvested);
			ASSERT_THAT(IsTrue(Start()));
			ASSERT_THAT(IsTrue(LineIs(TEXT("needs"))));
		}

		TEST_METHOD(ABadFlagMilestoneDateOrAmountFailsValidation)
		{
			const auto IsInvalid = [](const UObject& Object)
			{
				FDataValidationContext Context;
				return Object.IsDataValid(Context) == EDataValidationResult::Invalid;
			};
			const FGameplayTag Asked = MakeFlag(TEXT("Asked"));

			UVestigeDialogueCondition_Flag& NoFlag = Spawner.SpawnObject<UVestigeDialogueCondition_Flag>();
			ASSERT_THAT(IsTrue(IsInvalid(NoFlag)));
			UVestigeDialogueCondition_Flag& BackwardsDays = Spawner.SpawnObject<UVestigeDialogueCondition_Flag>();
			BackwardsDays.SetForTests(Asked, true, 2, 1);
			ASSERT_THAT(IsTrue(IsInvalid(BackwardsDays)));
			UVestigeDialogueCondition_Flag& GoodFlag = Spawner.SpawnObject<UVestigeDialogueCondition_Flag>();
			GoodFlag.SetForTests(Asked, false, 0, 2);
			ASSERT_THAT(IsFalse(IsInvalid(GoodFlag)));

			UVestigeDialogueCondition_Milestone& NoMilestone = Spawner.SpawnObject<UVestigeDialogueCondition_Milestone>();
			ASSERT_THAT(IsTrue(IsInvalid(NoMilestone)));

			UVestigeDialogueCondition_Trust& NegativeTrust = Spawner.SpawnObject<UVestigeDialogueCondition_Trust>();
			NegativeTrust.SetMinTrustForTests(-1);
			ASSERT_THAT(IsTrue(IsInvalid(NegativeTrust)));

			UVestigeDialogueCondition_Date& DayZero = Spawner.SpawnObject<UVestigeDialogueCondition_Date>();
			DayZero.SetForTests(0);
			ASSERT_THAT(IsTrue(IsInvalid(DayZero)));
			UVestigeDialogueCondition_Date& BackwardsRange = Spawner.SpawnObject<UVestigeDialogueCondition_Date>();
			BackwardsRange.SetForTests(2, FGameplayTag(), 10, 5);
			ASSERT_THAT(IsTrue(IsInvalid(BackwardsRange)));

			UVestigeDialogueOutcome_AddTrust& NoPoints = Spawner.SpawnObject<UVestigeDialogueOutcome_AddTrust>();
			NoPoints.SetAmountForTests(0);
			ASSERT_THAT(IsTrue(IsInvalid(NoPoints)));
			UVestigeDialogueOutcome_SetFlag& NoFlagToSet = Spawner.SpawnObject<UVestigeDialogueOutcome_SetFlag>();
			ASSERT_THAT(IsTrue(IsInvalid(NoFlagToSet)));
			UVestigeDialogueOutcome_SetFlag& GoodSet = Spawner.SpawnObject<UVestigeDialogueOutcome_SetFlag>();
			GoodSet.SetFlagForTests(Asked);
			ASSERT_THAT(IsFalse(IsInvalid(GoodSet)));
		}

		TEST_METHOD(AnInvalidConditionMakesTheDialogueInvalid)
		{
			UVestigeDialogueCondition_Flag& NoFlag = Spawner.SpawnObject<UVestigeDialogueCondition_Flag>();
			UVestigeDialogueData& Dialogue = MakeDialogueToValidate({MakeLine(TEXT("One"), TEXT("one"))});
			Dialogue.SetConditionsForTests({&NoFlag});
			FDataValidationContext Context;

			ASSERT_THAT(IsTrue(Dialogue.IsDataValid(Context) == EDataValidationResult::Invalid));
		}

		TEST_METHOD(ALineIdOfEndFailsValidation)
		{
			UVestigeDialogueData& Dialogue = MakeDialogueToValidate({MakeLine(VestigeDialogue::EndTalk, TEXT("one"))});
			FDataValidationContext Context;

			ASSERT_THAT(IsTrue(Dialogue.IsDataValid(Context) == EDataValidationResult::Invalid && Context.GetNumErrors() >= 1));
		}

		// ---- Trust ----

		TEST_METHOD(TrustIsEarnedPointsPlusTheCharactersMilestones)
		{
			Speaker->SetTrustFromMilestonesForTests(
				{{VestigeGameplayTags::Milestone_Farm_Harvested, 2}, {VestigeGameplayTags::Milestone_Farm_Sold, 1}});
			ASSERT_THAT(IsTrue(Relationships->GetTrust(*Speaker) == 0));

			Relationships->AddTrust(*Speaker, 3);
			ASSERT_THAT(IsTrue(Relationships->GetTrust(*Speaker) == 3));
			Milestones->Reach(VestigeGameplayTags::Milestone_Farm_Harvested);
			ASSERT_THAT(IsTrue(Relationships->GetTrust(*Speaker) == 5));
			// A first the speaker gives nothing for adds nothing.
			Milestones->Reach(VestigeGameplayTags::Milestone_Farm_Planted);
			ASSERT_THAT(IsTrue(Relationships->GetTrust(*Speaker) == 5));
			Milestones->Reach(VestigeGameplayTags::Milestone_Farm_Sold);
			ASSERT_THAT(IsTrue(Relationships->GetTrust(*Speaker) == 6));
			// Reaching one again counts nothing twice.
			Milestones->Reach(VestigeGameplayTags::Milestone_Farm_Sold);
			ASSERT_THAT(IsTrue(Relationships->GetTrust(*Speaker) == 6));
			// Only the earned points are the relationship's own.
			ASSERT_THAT(IsTrue(Relationships->GetEarnedTrust(Speaker->GetPrimaryAssetId()) == 3));
		}

		TEST_METHOD(AMilestoneReachedBeforeMeetingCounts)
		{
			Speaker->SetTrustFromMilestonesForTests({{VestigeGameplayTags::Milestone_Farm_Harvested, 1}});
			Milestones->Reach(VestigeGameplayTags::Milestone_Farm_Harvested);

			ASSERT_THAT(IsFalse(Relationships->HasMet(Speaker->GetPrimaryAssetId())));
			ASSERT_THAT(IsTrue(Relationships->GetTrust(*Speaker) == 1));
		}

		TEST_METHOD(AddingNoTrustChangesNothing)
		{
			Relationships->AddTrust(*Speaker, 0);
			Relationships->AddTrust(*Speaker, -2);

			ASSERT_THAT(IsTrue(Relationships->GetEarnedTrust(Speaker->GetPrimaryAssetId()) == 0));
		}

		TEST_METHOD(TrustAndFlagsAreSavedWithTheirDays)
		{
			const FPrimaryAssetId Id = Speaker->GetPrimaryAssetId();
			Relationships->AddTrust(*Speaker, 4);
			Relationships->SetFlag(*Speaker, MakeFlag(TEXT("Asked")), 3);
			Relationships->SetFlag(*Speaker, MakeFlag(TEXT("Step.Attentive")), 5);
			const FVestigeSaveRecord Record = Relationships->WriteSaveRecord();

			AVestigePlayerState& Fresh = Spawner.SpawnActor<AVestigePlayerState>();
			Fresh.DispatchBeginPlay();
			UVestigeRelationshipComponent* Loaded = Fresh.FindComponentByClass<UVestigeRelationshipComponent>();
			ASSERT_THAT(IsNotNull(Loaded));
			int32 LeftOut = 0;
			ASSERT_THAT(IsTrue(Loaded->ReadSaveRecord(Record, LeftOut)));

			ASSERT_THAT(IsTrue(LeftOut == 0));
			ASSERT_THAT(IsTrue(Loaded->GetEarnedTrust(Id) == 4));
			ASSERT_THAT(IsTrue(Loaded->GetFlagDays(Id).Num() == 2));
			ASSERT_THAT(IsTrue(*Loaded->GetFlagDays(Id).Find(MakeFlag(TEXT("Asked"))) == 3));
			ASSERT_THAT(IsTrue(*Loaded->GetFlagDays(Id).Find(MakeFlag(TEXT("Step.Attentive"))) == 5));
			// Written for the trust and the flags though the player has not met them.
			ASSERT_THAT(IsFalse(Loaded->HasMet(Id)));
			ASSERT_THAT(IsTrue(Loaded->GetSaveVersion() == 1));
		}

		TEST_METHOD(ARecordWithoutTrustOrFlagsLoadsAsNone)
		{
			// What a save from before the trust track holds: a met character and nothing more.
			FVestigeRelationshipSaveData Data;
			FVestigeCharacterRelationshipSaveData& Known = Data.Characters.AddDefaulted_GetRef();
			Known.Character = TestSpeakerId;
			Known.bMet = true;
			int32 LeftOut = 0;

			ASSERT_THAT(IsTrue(Relationships->ReadSaveRecord(FVestigeSaveRecord::Write(Data), LeftOut)));

			const FPrimaryAssetId Id = Speaker->GetPrimaryAssetId();
			ASSERT_THAT(IsTrue(Relationships->HasMet(Id)));
			ASSERT_THAT(IsTrue(Relationships->GetEarnedTrust(Id) == 0));
			ASSERT_THAT(IsTrue(Relationships->GetFlagDays(Id).IsEmpty()));
			ASSERT_THAT(IsTrue(LeftOut == 0));
		}

		TEST_METHOD(AnUnknownSavedFlagIsSkipped)
		{
			FVestigeRelationshipSaveData Data;
			FVestigeCharacterRelationshipSaveData& Known = Data.Characters.AddDefaulted_GetRef();
			Known.Character = TestSpeakerId;
			Known.Flags.Add({TEXT("Vestige.Relationship.Asked"), 2});
			Known.Flags.Add({TEXT("Vestige.Relationship.NoSuchFlag"), 3});
			TestRunner->AddExpectedMessagePlain(TEXT("no longer exists"), ELogVerbosity::Warning);
			int32 LeftOut = 0;

			ASSERT_THAT(IsTrue(Relationships->ReadSaveRecord(FVestigeSaveRecord::Write(Data), LeftOut)));

			ASSERT_THAT(IsTrue(LeftOut == 1));
			const TMap<FGameplayTag, int32>& Flags = Relationships->GetFlagDays(Speaker->GetPrimaryAssetId());
			ASSERT_THAT(IsTrue(Flags.Num() == 1 && Flags.Contains(MakeFlag(TEXT("Asked")))));
		}

		TEST_METHOD(ANewGameForgetsTrustAndFlags)
		{
			const FPrimaryAssetId Id = Speaker->GetPrimaryAssetId();
			Relationships->AddTrust(*Speaker, 4);
			Relationships->SetFlag(*Speaker, MakeFlag(TEXT("Asked")), 3);

			Relationships->StartNewGame();

			ASSERT_THAT(IsTrue(Relationships->GetEarnedTrust(Id) == 0));
			ASSERT_THAT(IsTrue(Relationships->GetFlagDays(Id).IsEmpty()));
		}

		// ---- The farmer ----

		TEST_METHOD(TheFarmerTurnsToTheSpeaker)
		{
			// The farmer's facing is the targeting component's aim, which needs a grid; a farmer with no controller isn't possessed.
			AFarmGrid& Grid = Spawner.SpawnActor<AFarmGrid>();
			Grid.DispatchBeginPlay();
			UClass* FarmerClass =
				LoadClass<AVestigePlayerCharacter>(nullptr, TEXT("/Game/Vestige/Blueprints/BP_PlayerCharacter.BP_PlayerCharacter_C"));
			ASSERT_THAT(IsNotNull(FarmerClass));
			APlayerController& Controller = Spawner.SpawnActor<APlayerController>();
			Controller.PlayerState = PlayerState;
			AVestigePlayerCharacter& Farmer = Spawner.SpawnActor<AVestigePlayerCharacter>(FActorSpawnParameters(), FarmerClass);
			Controller.Possess(&Farmer);
			Farmer.DispatchBeginPlay();
			Farmer.SetActorLocation(FVector(400.f, 400.f, 0.f));
			SpeakerActor->SetActorLocation(FVector(400.f, 700.f, 0.f));
			const UVestigeCharacterMovementComponent* Movement = Farmer.GetCharacterMovement<UVestigeCharacterMovementComponent>();
			ASSERT_THAT(IsNotNull(Movement));
			AddDialogue(TEXT("Test.Talk"), 10, {MakeLine(TEXT("One"), TEXT("one"))});

			ASSERT_THAT(IsTrue(Start()));

			// The speaker is due +Y from the farmer; the eight facing directions put him within 22.5 degrees of it.
			const FVector Facing = Movement->GetIdleFacing();
			ASSERT_THAT(IsFalse(Facing.IsNearlyZero()));
			const double ToSpeakerDegrees = FMath::RadiansToDegrees(FMath::Acos(Facing.GetSafeNormal() | FVector(0.0, 1.0, 0.0)));
			ASSERT_THAT(IsTrue(ToSpeakerDegrees <= 22.5));
		}

		// ---- Hob's real data ----

		const UVestigeCharacterData* FindHob() const { return Catalog->Find<UVestigeCharacterData>(HobId()); }

		/** The Hob conversations that apply to Context, highest priority first. */
		TArray<const UVestigeDialogueData*> FindHobTalks(const FVestigeDialogueContext& Context) const
		{
			TArray<const UVestigeContentData*> All;
			Catalog->FindAll(UVestigeDialogueData::AssetType, All);
			TArray<const UVestigeDialogueData*> Matching;
			for (const UVestigeContentData* Content : All)
			{
				const UVestigeDialogueData* Dialogue = Cast<UVestigeDialogueData>(Content);
				if (Dialogue && Dialogue->GetSpeaker() == HobId() && Dialogue->AreConditionsMet(Context))
				{
					Matching.Add(Dialogue);
				}
			}
			Matching.Sort(
				[](const UVestigeDialogueData& A, const UVestigeDialogueData& B)
				{
					return A.GetPriority() > B.GetPriority();
				});
			return Matching;
		}

		TArray<const UVestigeDialogueData*> GetHobTalks() const
		{
			TArray<const UVestigeContentData*> All;
			Catalog->FindAll(UVestigeDialogueData::AssetType, All);
			TArray<const UVestigeDialogueData*> Talks;
			for (const UVestigeContentData* Content : All)
			{
				const UVestigeDialogueData* Dialogue = Cast<UVestigeDialogueData>(Content);
				if (Dialogue && Dialogue->GetSpeaker() == HobId())
				{
					Talks.Add(Dialogue);
				}
			}
			return Talks;
		}

		bool StartWithHob() { return Talk->StartConversation(*SpeakerActor, *FindHob()); }

		TEST_METHOD(HobHasCharacterDataAndConversations)
		{
			const UVestigeCharacterData* Hob = FindHob();
			ASSERT_THAT(IsNotNull(Hob));
			ASSERT_THAT(IsTrue(Hob->GetDisplayName().ToString() == TEXT("Hob")));
			ASSERT_THAT(IsTrue(Hob->GetCharacterTags().HasTag(FGameplayTag::RequestGameplayTag(TEXT("Vestige.Character.Robot")))));
			// The first meeting, the greetings (everyday, warm and memory, four bands each), again today, the three steps (two attentive
			// talks), the nine topics and the two watered-morning lines.
			ASSERT_THAT(IsTrue(GetHobTalks().Num() == 29));
		}

		TEST_METHOD(HobsFirstTalkIsTheFirstMeeting)
		{
			ASSERT_THAT(IsNotNull(FindHob()));
			SetClock(1, 7.f);

			ASSERT_THAT(IsTrue(StartWithHob()));

			ASSERT_THAT(IsTrue(LineKey() == TEXT("Hob.Meet.010")));
			ASSERT_THAT(IsTrue(Relationships->HasMet(HobId())));
		}

		TEST_METHOD(TheFirstMeetingOffersThreeRepliesAndAnswersTheOneChosen)
		{
			ASSERT_THAT(IsNotNull(FindHob()));
			ASSERT_THAT(IsTrue(StartWithHob()));
			ASSERT_THAT(IsFalse(Talk->IsLastLine()));

			Talk->Advance();
			ASSERT_THAT(IsTrue(LineKey() == TEXT("Hob.Meet.020")));
			ASSERT_THAT(IsTrue(Talk->GetChoiceTexts().Num() == 3));
			Talk->Advance();
			ASSERT_THAT(IsTrue(LineKey() == TEXT("Hob.Meet.020")));

			Talk->Choose(1);
			ASSERT_THAT(IsTrue(LineKey() == TEXT("Hob.Meet.Answer.Name")));
			Talk->Advance();

			// Every answer leads to the same last line.
			ASSERT_THAT(IsTrue(LineKey() == TEXT("Hob.Meet.030")));
			ASSERT_THAT(IsTrue(Talk->IsLastLine()));
			Talk->Advance();
			ASSERT_THAT(IsFalse(Talk->IsInConversation()));
			ASSERT_THAT(IsFalse(Clock->IsHeld()));
		}

		TEST_METHOD(EveryFirstMeetingReplyReachesTheLastLine)
		{
			ASSERT_THAT(IsNotNull(FindHob()));
			for (int32 Reply = 0; Reply < 3; ++Reply)
			{
				Relationships->StartNewGame();
				ASSERT_THAT(IsTrue(StartWithHob()));
				Talk->Advance();
				Talk->Choose(Reply);
				Talk->Advance();
				ASSERT_THAT(IsTrue(LineKey() == TEXT("Hob.Meet.030")));
				Talk->EndConversation();
			}
		}

		TEST_METHOD(AfterTheFirstMeetingTheSameDayGivesAgainToday)
		{
			ASSERT_THAT(IsNotNull(FindHob()));
			ASSERT_THAT(IsTrue(StartWithHob()));
			Talk->EndConversation();

			ASSERT_THAT(IsTrue(StartWithHob()));

			ASSERT_THAT(IsTrue(LineKey().ToString().StartsWith(TEXT("Hob.Again."))));
			ASSERT_THAT(IsTrue(Talk->IsLastLine()));
		}

		/** Every farm topic answered, so the day's talk is the greeting and not a topic (from day 2 a topic is due). */
		void AnswerEveryHobTopic()
		{
			for (const TCHAR* Topic : {TEXT("SeasonEnd"), TEXT("Harvest"), TEXT("Bin"), TEXT("Bloom"), TEXT("Sinkwort"), TEXT("Planting"),
										  TEXT("Dispensers"), TEXT("FarRows"), TEXT("Well")})
			{
				Relationships->SetFlag(*FindHob(), MakeFlag(*(FString(TEXT("Topic.")) + Topic)), 1);
			}
		}

		TEST_METHOD(TheNextDayGivesTheGreetingOfTheHour)
		{
			ASSERT_THAT(IsNotNull(FindHob()));
			ASSERT_THAT(IsTrue(StartWithHob()));
			Talk->EndConversation();
			AnswerEveryHobTopic();

			struct FBand
			{
				float Hour;
				const TCHAR* Prefix;
			};
			const FBand Bands[] = {{6.f, TEXT("Hob.Greet.Morning.")}, {11.5f, TEXT("Hob.Greet.Morning.")},
				{12.f, TEXT("Hob.Greet.Afternoon.")}, {17.5f, TEXT("Hob.Greet.Afternoon.")}, {18.f, TEXT("Hob.Greet.Evening.")},
				{23.5f, TEXT("Hob.Greet.Evening.")}, {24.f, TEXT("Hob.Greet.Late.")}, {26.f, TEXT("Hob.Greet.Late.")}};
			int32 Day = 2;
			for (const FBand& Band : Bands)
			{
				SetClock(Day++, Band.Hour);
				ASSERT_THAT(IsTrue(StartWithHob()));
				ASSERT_THAT(IsTrue(LineKey().ToString().StartsWith(Band.Prefix), *FString::Printf(TEXT("hour %.1f"), Band.Hour)));
				Talk->EndConversation();
			}
		}

		TEST_METHOD(HobAlwaysHasSomethingToSay)
		{
			// Whether met or not, talked today or not, at every half hour until the day ends: exactly one conversation at the top priority.
			ASSERT_THAT(IsNotNull(FindHob()));
			const UVestigeCharacterData& Hob = *FindHob();
			struct FState
			{
				bool bMet;
				int32 TalksToday;
			};
			const FState States[] = {{false, 0}, {true, 0}, {true, 1}, {true, 4}};
			for (const FState& State : States)
			{
				for (float Hour = 6.f; Hour < Clock->GetDayEndHour(); Hour += 0.5f)
				{
					FVestigeDialogueContext Context;
					Context.Speaker = &Hob;
					Context.Day = 3;
					Context.TimeOfDay = Hour;
					Context.bHasMet = State.bMet;
					Context.TalksToday = State.TalksToday;
					const TArray<const UVestigeDialogueData*> Matching = FindHobTalks(Context);
					const FString Where = FString::Printf(TEXT("met %d, talks %d, hour %.1f"), State.bMet, State.TalksToday, Hour);
					ASSERT_THAT(IsFalse(Matching.IsEmpty(), *Where));
					const bool bTie = Matching.Num() > 1 && Matching[0]->GetPriority() == Matching[1]->GetPriority();
					ASSERT_THAT(IsFalse(bTie, *Where));
				}
			}
		}

		TEST_METHOD(EveryDialogueIsValid)
		{
			TArray<FPrimaryAssetId> Ids;
			UAssetManager::Get().GetPrimaryAssetIdList(UVestigeDialogueData::AssetType, Ids);
			ASSERT_THAT(IsTrue(Ids.Num() >= 6));
			for (const FPrimaryAssetId& Id : Ids)
			{
				const UObject* Dialogue = UAssetManager::Get().GetPrimaryAssetPath(Id).TryLoad();
				ASSERT_THAT(IsNotNull(Dialogue, *Id.ToString()));
				FDataValidationContext Context;
				const EDataValidationResult Result = Dialogue->IsDataValid(Context);
				// No warning either: that is how a text that isn't from the string table is reported.
				ASSERT_THAT(IsTrue(Result != EDataValidationResult::Invalid && Context.GetNumErrors() == 0 && Context.GetNumWarnings() == 0,
					*Id.ToString()));
			}
		}

		TEST_METHOD(EveryLineFitsTheBoxAndIsFromTheStringTable)
		{
			TArray<const UVestigeContentData*> All;
			Catalog->FindAll(UVestigeDialogueData::AssetType, All);
			ASSERT_THAT(IsTrue(All.Num() >= 29));
			for (const UVestigeContentData* Content : All)
			{
				const UVestigeDialogueData* Dialogue = Cast<UVestigeDialogueData>(Content);
				ASSERT_THAT(IsNotNull(Dialogue));
				for (const FVestigeDialogueLine& Line : Dialogue->GetLines())
				{
					ASSERT_THAT(IsFalse(Line.Texts.IsEmpty(), *Line.LineId.ToString()));
					for (const FText& Text : Line.Texts)
					{
						ASSERT_THAT(IsTrue(Text.IsFromStringTable(), *Line.LineId.ToString()));
						ASSERT_THAT(IsTrue(Text.ToString().Len() <= VestigeDialogue::MaxLineLength, *Line.LineId.ToString()));
						// The string table key is the writer's line ID, or the pool's ID with a number.
						// A merged greeting pool has an ID of its own and keeps the writer's keys of the lines it holds.
						const bool bMergedPool = Line.Texts.Num() > 1 && GetTableKey(Text).ToString().StartsWith(TEXT("Hob.Greet."));
						ASSERT_THAT(IsTrue(bMergedPool || GetTableKey(Text).ToString().StartsWith(Line.LineId.ToString()),
							*Line.LineId.ToString()));
					}
					for (const FVestigeDialogueChoice& Choice : Line.Choices)
					{
						ASSERT_THAT(IsTrue(Choice.Text.IsFromStringTable(), *Choice.ReplyId.ToString()));
						ASSERT_THAT(IsTrue(Choice.Text.ToString().Len() <= VestigeDialogue::MaxReplyLength, *Choice.ReplyId.ToString()));
						// The order reply entered a second time (for the attentive step) has an ID of its own and the first one's text.
						const FString TextKey = GetTableKey(Choice.Text).ToString();
						const bool bSecondOrderReply = Choice.ReplyId.ToString().EndsWith(TEXT(".Reply.OrderView")) &&
							TextKey == Choice.ReplyId.ToString().Replace(TEXT("OrderView"), TEXT("Order"));
						ASSERT_THAT(IsTrue(bSecondOrderReply || TextKey == Choice.ReplyId.ToString(), *Choice.ReplyId.ToString()));
					}
				}
			}
		}

		// ---- The box's keys ----

		/** The keys Action is mapped to in Context. */
		static TArray<FKey> GetKeys(const UInputMappingContext& Context, const UInputAction& Action)
		{
			TArray<FKey> Keys;
			for (const FEnhancedActionKeyMapping& Mapping : Context.GetMappings())
			{
				if (Mapping.Action == &Action)
				{
					Keys.Add(Mapping.Key);
				}
			}
			return Keys;
		}

		TEST_METHOD(TheGamepadInteractKeyAlsoAdvances)
		{
			// The key that opens a talk carries it on, so interact moving to another button can't leave the box without it.
			const UInputMappingContext* Farming =
				LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/Vestige/Input/IMC_Farming.IMC_Farming"));
			const UInputMappingContext* DialogueKeys =
				LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/Vestige/Input/IMC_Dialogue.IMC_Dialogue"));
			const UInputAction* Interact = LoadObject<UInputAction>(nullptr, TEXT("/Game/Vestige/Input/IA_Interact.IA_Interact"));
			const UInputAction* Next = LoadObject<UInputAction>(nullptr, TEXT("/Game/Vestige/Input/IA_DialogueNext.IA_DialogueNext"));
			ASSERT_THAT(IsNotNull(Farming));
			ASSERT_THAT(IsNotNull(DialogueKeys));
			ASSERT_THAT(IsNotNull(Interact));
			ASSERT_THAT(IsNotNull(Next));

			const TArray<FKey> InteractKeys = GetKeys(*Farming, *Interact);
			const TArray<FKey> NextKeys = GetKeys(*DialogueKeys, *Next);
			int32 GamepadKeys = 0;
			for (const FKey& Key : InteractKeys)
			{
				if (Key.IsGamepadKey())
				{
					++GamepadKeys;
					ASSERT_THAT(IsTrue(NextKeys.Contains(Key), *Key.ToString()));
				}
			}
			ASSERT_THAT(IsTrue(GamepadKeys >= 1));
		}

		TEST_METHOD(TheBoxsKeysAreFSpaceEnterAndAGamepadButtonToAdvanceAndEscapeAndStartToEnd)
		{
			const UInputMappingContext* DialogueKeys =
				LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/Vestige/Input/IMC_Dialogue.IMC_Dialogue"));
			const UInputAction* Next = LoadObject<UInputAction>(nullptr, TEXT("/Game/Vestige/Input/IA_DialogueNext.IA_DialogueNext"));
			const UInputAction* End = LoadObject<UInputAction>(nullptr, TEXT("/Game/Vestige/Input/IA_DialogueEnd.IA_DialogueEnd"));
			ASSERT_THAT(IsNotNull(DialogueKeys));
			ASSERT_THAT(IsNotNull(Next));
			ASSERT_THAT(IsNotNull(End));

			const TArray<FKey> NextKeys = GetKeys(*DialogueKeys, *Next);
			const TArray<FKey> EndKeys = GetKeys(*DialogueKeys, *End);

			ASSERT_THAT(IsTrue(NextKeys.Contains(EKeys::F)));
			ASSERT_THAT(IsTrue(NextKeys.Contains(EKeys::SpaceBar)));
			ASSERT_THAT(IsTrue(NextKeys.Contains(EKeys::Enter)));
			ASSERT_THAT(IsTrue(NextKeys.Contains(EKeys::Gamepad_FaceButton_Bottom)));
			ASSERT_THAT(IsTrue(EndKeys.Contains(EKeys::Escape)));
			ASSERT_THAT(IsTrue(EndKeys.Contains(EKeys::Gamepad_Special_Right)));
			// Nothing both advances and ends.
			for (const FKey& Key : EndKeys)
			{
				ASSERT_THAT(IsFalse(NextKeys.Contains(Key), *Key.ToString()));
			}
		}
	};
}

#endif
