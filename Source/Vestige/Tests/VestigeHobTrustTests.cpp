// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "Characters/VestigeConversationComponent.h"
#include "Characters/VestigeRelationshipComponent.h"
#include "Components/ActorTestSpawner.h"
#include "Content/VestigeCalendar.h"
#include "Content/VestigeCharacterData.h"
#include "Content/VestigeContentCatalog.h"
#include "Content/VestigeDialogueContext.h"
#include "Content/VestigeDialogueData.h"
#include "Core/VestigeMilestoneSubsystem.h"
#include "CQTest.h"
#include "GameplayTagContainer.h"
#include "Internationalization/Text.h"
#include "Player/VestigePlayerState.h"
#include "Time/VestigeClockSaveData.h"
#include "Time/VestigeClockSubsystem.h"

// Hob's trust track on the real data: the conversations, their conditions and outcomes as the data assets hold them, played through
// the conversation component with the relationship and the milestones set up by the test (Systems/11-Dialogue.md, "Hob's trust track").

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace VestigeHobTrustTests
{
	// A function, not a static: UVestigeCharacterData::AssetType is another file's static, which a unity build may initialize after this one.
	FPrimaryAssetId HobId()
	{
		return FPrimaryAssetId(UVestigeCharacterData::AssetType, TEXT("Base.Hob"));
	}

	/** A farm topic, in the order they come when all are due, and the milestone that opens it ("" for none). */
	struct FTopic
	{
		const TCHAR* Name;
		const TCHAR* Milestone;
	};

	const FTopic TopicsInOrder[] = {{TEXT("SeasonEnd"), TEXT("")}, {TEXT("Harvest"), TEXT("Vestige.Milestone.Farm.Harvested")},
		{TEXT("Bin"), TEXT("Vestige.Milestone.Farm.Sold")}, {TEXT("Bloom"), TEXT("Vestige.Milestone.Farm.HarvestedMutated")},
		{TEXT("Sinkwort"), TEXT("Vestige.Milestone.Farm.PlantedCleaningCrop")}, {TEXT("Planting"), TEXT("Vestige.Milestone.Farm.Planted")},
		{TEXT("Dispensers"), TEXT("")}, {TEXT("FarRows"), TEXT("")}, {TEXT("Well"), TEXT("")}};

	/** A day in the first season's days 19 to 25, where the season-end topic is due. */
	constexpr int32 LateSpringDay = 20;

	/** The string table key of Text, or an empty name when it isn't from one. */
	FName GetTableKey(const FText& Text)
	{
		FName TableId;
		FString Key;
		return FTextInspector::GetTableIdAndKey(Text, TableId, Key) ? FName(*Key) : NAME_None;
	}

	FGameplayTag MakeTag(const FString& Name) { return FGameplayTag::RequestGameplayTag(FName(*Name)); }

	TEST_CLASS(HobTrust, "Vestige")
	{
		FActorTestSpawner Spawner;
		UVestigeContentCatalog* Catalog = nullptr;
		UVestigeClockSubsystem* Clock = nullptr;
		UVestigeMilestoneSubsystem* Milestones = nullptr;
		AVestigePlayerState* PlayerState = nullptr;
		UVestigeConversationComponent* Talk = nullptr;
		UVestigeRelationshipComponent* Relationships = nullptr;
		const UVestigeCharacterData* Hob = nullptr;
		AActor* HobActor = nullptr;

		/** The player has met Hob on day 1, on day 2 at 08:00, with nothing else done. */
		BEFORE_EACH()
		{
			Spawner.InitializeGameSubsystems();
			Catalog = UVestigeContentCatalog::Get(&Spawner.GetWorld());
			Clock = UVestigeClockSubsystem::Get(&Spawner.GetWorld());
			Milestones = UVestigeMilestoneSubsystem::Get(&Spawner.GetWorld());
			ASSERT_THAT(IsNotNull(Catalog));
			ASSERT_THAT(IsNotNull(Clock));
			ASSERT_THAT(IsNotNull(Milestones));

			PlayerState = &Spawner.SpawnActor<AVestigePlayerState>();
			PlayerState->DispatchBeginPlay();
			Talk = PlayerState->FindComponentByClass<UVestigeConversationComponent>();
			Relationships = PlayerState->FindComponentByClass<UVestigeRelationshipComponent>();
			ASSERT_THAT(IsNotNull(Talk));
			ASSERT_THAT(IsNotNull(Relationships));

			Hob = Catalog->Find<UVestigeCharacterData>(HobId());
			ASSERT_THAT(IsNotNull(Hob));
			HobActor = &Spawner.SpawnActor<AActor>();

			Relationships->RecordTalk(HobId(), 1);
			SetDay(2);
		}

		// ---- Helpers ----

		void SetDay(int32 Day, float Hour = 9.f)
		{
			FVestigeClockSaveData Data;
			Data.Day = Day;
			Data.TimeOfDay = Hour;
			Clock->RestoreFromSaveData(Data);
		}

		FGameplayTag Flag(const TCHAR* Name) const { return MakeTag(FString(TEXT("Vestige.Relationship.")) + Name); }

		void SetFlag(const TCHAR* Name, int32 Day) { Relationships->SetFlag(*Hob, Flag(Name), Day); }

		bool HasFlag(const TCHAR* Name) const { return Relationships->HasFlag(HobId(), Flag(Name)); }

		/** The day the flag was last set, or INDEX_NONE. */
		int32 GetFlagDay(const TCHAR* Name) const
		{
			const int32* Day = Relationships->GetFlagDays(HobId()).Find(Flag(Name));
			return Day ? *Day : INDEX_NONE;
		}

		void Reach(const TCHAR* Milestone) { Milestones->Reach(MakeTag(Milestone)); }

		/** Every farm topic answered, so none is due. */
		void AnswerEveryTopic()
		{
			for (const FTopic& Topic : TopicsInOrder)
			{
				SetFlag(*(FString(TEXT("Topic.")) + Topic.Name), 1);
			}
		}

		/** A new player with Hob met, no trust, no flags and no milestones. */
		void StartOver()
		{
			Relationships->StartNewGame();
			Milestones->StartNewGame();
			Relationships->RecordTalk(HobId(), 1);
		}

		/** Starts a talk with Hob. */
		bool Start() { return Talk->StartConversation(*HobActor, *Hob); }

		FName LineKey() const { return GetTableKey(Talk->GetLineText()); }

		bool KeyStartsWith(const TCHAR* Prefix) const { return LineKey().ToString().StartsWith(Prefix); }

		/** The key of the first line of the talk Hob gives on Day, which is then ended (an Escape). None when no talk starts. */
		FName TalkOn(int32 Day, float Hour = 9.f)
		{
			SetDay(Day, Hour);
			if (!Start())
			{
				return NAME_None;
			}
			const FName Key = LineKey();
			Talk->EndConversation();
			return Key;
		}

		/** The index of the listed reply whose text key ends with Suffix, or INDEX_NONE. */
		int32 FindReply(const TCHAR* Suffix) const
		{
			const TArray<FText> Replies = Talk->GetChoiceTexts();
			for (int32 Index = 0; Index < Replies.Num(); ++Index)
			{
				if (GetTableKey(Replies[Index]).ToString().EndsWith(Suffix))
				{
					return Index;
				}
			}
			return INDEX_NONE;
		}

		/** Chooses the listed reply whose text key ends with Suffix. Returns whether there was one. */
		bool Reply(const TCHAR* Suffix)
		{
			const int32 Index = FindReply(Suffix);
			if (Index == INDEX_NONE)
			{
				return false;
			}
			Talk->Choose(Index);
			return true;
		}

		/** Plays on, with the first reply wherever replies are listed, until the talk ends. */
		void PlayToTheEnd()
		{
			for (int32 Guard = 0; Guard < 20 && Talk->IsInConversation(); ++Guard)
			{
				if (Talk->GetChoiceTexts().IsEmpty())
				{
					Talk->Advance();
				}
				else
				{
					Talk->Choose(0);
				}
			}
		}

		const UVestigeDialogueData* FindDialogue(const TCHAR* ContentId) const
		{
			return Catalog->Find<UVestigeDialogueData>(FPrimaryAssetId(UVestigeDialogueData::AssetType, ContentId));
		}

		/** A context for Hob on Day at 09:00 with the given state of the relationship (flags and their days). */
		FVestigeDialogueContext MakeContext(int32 Day, bool bMet, int32 TalksToday, int32 Trust, const TMap<FGameplayTag, int32>& FlagDays,
			const FGameplayTagContainer& Reached, float Hour = 9.f) const
		{
			const FVestigeDate Date = Clock->GetCalendar().GetDate(Day);
			FVestigeDialogueContext Context;
			Context.Speaker = Hob;
			Context.Day = Day;
			Context.TimeOfDay = Hour;
			Context.bHasMet = bMet;
			Context.TalksToday = TalksToday;
			Context.Trust = Trust;
			Context.FlagDays = FlagDays;
			Context.Milestones = Reached;
			Context.Season = Date.Season;
			Context.DayOfSeason = Date.DayOfSeason;
			return Context;
		}

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

		/** The content ID of the conversation that wins for Context, or an empty name. */
		FName TopTalk(const FVestigeDialogueContext& Context) const
		{
			const TArray<const UVestigeDialogueData*> Matching = FindHobTalks(Context);
			return Matching.IsEmpty() ? NAME_None : Matching[0]->GetContentId();
		}

		// ---- The farm's firsts give trust ----

		TEST_METHOD(HobGetsAPointForEachFarmFirst)
		{
			const TMap<FGameplayTag, int32>& Points = Hob->GetTrustFromMilestones();

			ASSERT_THAT(IsTrue(Points.Num() == 3));
			for (const TCHAR* Milestone : {TEXT("Harvested"), TEXT("Sold"), TEXT("Cleaned")})
			{
				const int32* Found = Points.Find(MakeTag(FString(TEXT("Vestige.Milestone.Farm.")) + Milestone));
				ASSERT_THAT(IsTrue(Found && *Found == 1, Milestone));
			}
		}

		// ---- The steps ----

		TEST_METHOD(TheAttentiveTalkPlaysWhenTrustReachesThree)
		{
			Relationships->AddTrust(*Hob, 2);
			ASSERT_THAT(IsFalse(TalkOn(3) == TEXT("Hob.Step.Attentive.010")));
			ASSERT_THAT(IsFalse(HasFlag(TEXT("Step.Attentive"))));

			Relationships->AddTrust(*Hob, 1);
			ASSERT_THAT(IsTrue(TalkOn(4) == TEXT("Hob.Step.Attentive.010")));

			// Line 1 sets the flag (Escape after it still counts), and it never plays again.
			ASSERT_THAT(AreEqual(GetFlagDay(TEXT("Step.Attentive")), 4));
			ASSERT_THAT(IsFalse(TalkOn(5) == TEXT("Hob.Step.Attentive.010")));
		}

		TEST_METHOD(TheFarmsFirstsAloneCanOpenTheAttentiveTalk)
		{
			Reach(TEXT("Vestige.Milestone.Farm.Harvested"));
			Reach(TEXT("Vestige.Milestone.Farm.Sold"));
			ASSERT_THAT(IsFalse(TalkOn(3) == TEXT("Hob.Step.Attentive.010")));

			Reach(TEXT("Vestige.Milestone.Farm.Cleaned"));

			ASSERT_THAT(IsTrue(TalkOn(4) == TEXT("Hob.Step.Attentive.010")));
		}

		TEST_METHOD(TheAttentiveTalkNamesTheAskWhenThePlayerAsked)
		{
			Relationships->AddTrust(*Hob, 3);
			SetFlag(TEXT("Asked"), 2);
			SetDay(3);
			ASSERT_THAT(IsTrue(Start()));
			ASSERT_THAT(IsTrue(LineKey() == TEXT("Hob.Step.Attentive.010")));
			Talk->Advance();
			ASSERT_THAT(IsTrue(LineKey() == TEXT("Hob.Step.Attentive.020A")));
			PlayToTheEnd();

			StartOver();
			Relationships->AddTrust(*Hob, 3);
			SetDay(3);
			ASSERT_THAT(IsTrue(Start()));
			Talk->Advance();
			ASSERT_THAT(IsTrue(LineKey() == TEXT("Hob.Step.Attentive.020B")));
			Talk->Advance();
			ASSERT_THAT(IsTrue(LineKey() == TEXT("Hob.Step.Attentive.030")));
			Talk->Advance();
			ASSERT_THAT(IsTrue(LineKey() == TEXT("Hob.Step.Attentive.040")));
			Talk->Advance();
			ASSERT_THAT(IsFalse(Talk->IsInConversation()));
		}

		TEST_METHOD(TwoStepsReachedTogetherPlayOnConsecutiveDays)
		{
			Relationships->AddTrust(*Hob, 10);
			Reach(TEXT("Vestige.Milestone.Farm.Cleaned"));

			ASSERT_THAT(IsTrue(TalkOn(3) == TEXT("Hob.Step.Attentive.010")));
			// Later the same day the next step isn't played: it needs the step before a day ago.
			ASSERT_THAT(IsTrue(TalkOn(3).ToString().StartsWith(TEXT("Hob.Again."))));

			SetDay(4);
			ASSERT_THAT(IsTrue(Start()));
			ASSERT_THAT(IsTrue(LineKey() == TEXT("Hob.Step.Wish.010")));
			Talk->Advance();
			Talk->Advance();
			ASSERT_THAT(IsTrue(LineKey() == TEXT("Hob.Step.Wish.030")));
			ASSERT_THAT(IsTrue(Reply(TEXT("Hob.Step.Wish.Reply.No"))));
			PlayToTheEnd();
			ASSERT_THAT(IsTrue(TalkOn(4).ToString().StartsWith(TEXT("Hob.Again."))));

			ASSERT_THAT(IsTrue(TalkOn(5) == TEXT("Hob.Step.Memory.010")));
			ASSERT_THAT(AreEqual(GetFlagDay(TEXT("Step.Memory")), 5));
		}

		TEST_METHOD(TheOfferAcceptedSetsTheHelpFlagAndRefusedDoesNot)
		{
			SetFlag(TEXT("Step.Attentive"), 1);
			Relationships->AddTrust(*Hob, 7);
			SetDay(3);
			ASSERT_THAT(IsTrue(Start()));
			ASSERT_THAT(IsTrue(LineKey() == TEXT("Hob.Step.Wish.010")));
			Talk->Advance();
			Talk->Advance();
			ASSERT_THAT(IsTrue(Talk->GetChoiceTexts().Num() == 2));
			ASSERT_THAT(IsFalse(HasFlag(TEXT("Step.WouldLike"))));
			ASSERT_THAT(IsTrue(Reply(TEXT("Hob.Step.Wish.Reply.Yes"))));
			ASSERT_THAT(IsTrue(LineKey() == TEXT("Hob.Step.Wish.Answer.Yes")));
			Talk->Advance();
			ASSERT_THAT(IsFalse(Talk->IsInConversation()));

			ASSERT_THAT(AreEqual(GetFlagDay(TEXT("Help.Watering")), 3));
			ASSERT_THAT(AreEqual(GetFlagDay(TEXT("Step.WouldLike")), 3));

			StartOver();
			SetFlag(TEXT("Step.Attentive"), 1);
			Relationships->AddTrust(*Hob, 7);
			SetDay(3);
			ASSERT_THAT(IsTrue(Start()));
			Talk->Advance();
			Talk->Advance();
			ASSERT_THAT(IsTrue(Reply(TEXT("Hob.Step.Wish.Reply.No"))));
			ASSERT_THAT(IsTrue(LineKey() == TEXT("Hob.Step.Wish.Answer.No")));
			Talk->Advance();

			ASSERT_THAT(IsFalse(HasFlag(TEXT("Help.Watering"))));
			ASSERT_THAT(AreEqual(GetFlagDay(TEXT("Step.WouldLike")), 3));
		}

		TEST_METHOD(AnOfferEndedBeforeAReplyComesBackOnALaterDay)
		{
			SetFlag(TEXT("Step.Attentive"), 1);
			Relationships->AddTrust(*Hob, 7);

			ASSERT_THAT(IsTrue(TalkOn(3) == TEXT("Hob.Step.Wish.010")));
			ASSERT_THAT(IsFalse(HasFlag(TEXT("Step.WouldLike"))));
			// The same day it isn't made again.
			ASSERT_THAT(IsTrue(TalkOn(3).ToString().StartsWith(TEXT("Hob.Again."))));

			ASSERT_THAT(IsTrue(TalkOn(4) == TEXT("Hob.Step.Wish.010")));
		}

		TEST_METHOD(TheMemoryNeedsTheCleaningHarvest)
		{
			SetFlag(TEXT("Step.Attentive"), 1);
			SetFlag(TEXT("Step.WouldLike"), 1);
			Relationships->AddTrust(*Hob, 10);
			ASSERT_THAT(IsFalse(TalkOn(3) == TEXT("Hob.Step.Memory.010")));

			Reach(TEXT("Vestige.Milestone.Farm.Cleaned"));

			ASSERT_THAT(IsTrue(TalkOn(4) == TEXT("Hob.Step.Memory.010")));
			ASSERT_THAT(IsFalse(TalkOn(5) == TEXT("Hob.Step.Memory.010")));
		}

		TEST_METHOD(TheMemoryNeedsTenTrust)
		{
			SetFlag(TEXT("Step.Attentive"), 1);
			SetFlag(TEXT("Step.WouldLike"), 1);
			Relationships->AddTrust(*Hob, 8);
			Reach(TEXT("Vestige.Milestone.Farm.Cleaned"));
			ASSERT_THAT(IsFalse(TalkOn(3) == TEXT("Hob.Step.Memory.010")));

			Relationships->AddTrust(*Hob, 1);

			ASSERT_THAT(IsTrue(TalkOn(4) == TEXT("Hob.Step.Memory.010")));
		}

		TEST_METHOD(EveryMemoryReplyReachesTheLastLine)
		{
			for (int32 ReplyIndex = 0; ReplyIndex < 3; ++ReplyIndex)
			{
				StartOver();
				SetFlag(TEXT("Step.Attentive"), 1);
				SetFlag(TEXT("Step.WouldLike"), 1);
				Relationships->AddTrust(*Hob, 10);
				Reach(TEXT("Vestige.Milestone.Farm.Cleaned"));
				SetDay(3);
				ASSERT_THAT(IsTrue(Start()));
				ASSERT_THAT(IsTrue(LineKey() == TEXT("Hob.Step.Memory.010")));
				Talk->Advance();
				Talk->Advance();
				Talk->Advance();
				ASSERT_THAT(IsTrue(LineKey() == TEXT("Hob.Step.Memory.040")));
				ASSERT_THAT(IsTrue(Talk->GetChoiceTexts().Num() == 3));

				Talk->Choose(ReplyIndex);
				Talk->Advance();

				ASSERT_THAT(IsTrue(LineKey() == TEXT("Hob.Step.Memory.050")));
				Talk->Advance();
				ASSERT_THAT(IsFalse(Talk->IsInConversation()));
			}
		}

		// ---- The farm topics ----

		TEST_METHOD(ATopicIsNeverOfferedTwoDaysRunning)
		{
			ASSERT_THAT(IsTrue(TalkOn(2).ToString().StartsWith(TEXT("Hob.Topic."))));

			ASSERT_THAT(IsTrue(TalkOn(3).ToString().StartsWith(TEXT("Hob.Greet."))));
			ASSERT_THAT(IsTrue(TalkOn(4).ToString().StartsWith(TEXT("Hob.Topic."))));
		}

		TEST_METHOD(AnUnansweredTopicComesBackOnALaterDay)
		{
			ASSERT_THAT(IsTrue(TalkOn(2) == TEXT("Hob.Topic.Dispensers.Open")));
			ASSERT_THAT(IsFalse(HasFlag(TEXT("Topic.Dispensers"))));
			ASSERT_THAT(IsTrue(HasFlag(TEXT("TopicOffered"))));

			ASSERT_THAT(IsTrue(TalkOn(4) == TEXT("Hob.Topic.Dispensers.Open")));
		}

		TEST_METHOD(AnAnsweredTopicNeverComesBack)
		{
			const TCHAR* const Expected[] = {TEXT("Hob.Topic.Dispensers.Open"), TEXT("Hob.Topic.FarRows.Open"), TEXT("Hob.Topic.Well.Open")};
			int32 Day = 2;
			for (const TCHAR* Topic : Expected)
			{
				SetDay(Day);
				ASSERT_THAT(IsTrue(Start()));
				ASSERT_THAT(IsTrue(LineKey() == Topic));
				ASSERT_THAT(IsTrue(Reply(TEXT(".Reply.Leave"))));
				Talk->Advance();
				ASSERT_THAT(IsFalse(Talk->IsInConversation()));
				Day += 2;
			}

			// All three are answered: only greetings from now on, on any later day.
			ASSERT_THAT(IsTrue(TalkOn(8).ToString().StartsWith(TEXT("Hob.Greet."))));
			ASSERT_THAT(IsTrue(TalkOn(10).ToString().StartsWith(TEXT("Hob.Greet."))));
			ASSERT_THAT(IsTrue(TalkOn(40).ToString().StartsWith(TEXT("Hob.Greet."))));
		}

		/** Starts the talk of Topic on a day when only it is due, and checks it opens. */
		bool StartTopic(const FTopic& Topic, bool bAttentive)
		{
			StartOver();
			for (const FTopic& Other : TopicsInOrder)
			{
				if (FCString::Strcmp(Other.Name, Topic.Name) != 0)
				{
					SetFlag(*(FString(TEXT("Topic.")) + Other.Name), 1);
				}
			}
			if (Topic.Milestone[0] != 0)
			{
				Reach(Topic.Milestone);
			}
			if (bAttentive)
			{
				SetFlag(TEXT("Step.Attentive"), 1);
			}
			SetDay(LateSpringDay);
			return Start() && LineKey() == FName(*(FString(TEXT("Hob.Topic.")) + Topic.Name + TEXT(".Open")));
		}

		TEST_METHOD(EachTopicListsOneAskReplyThatGivesTrustAndOneOrderReply)
		{
			for (const FTopic& Topic : TopicsInOrder)
			{
				for (const bool bAttentive : {false, true})
				{
					const FString Where = FString::Printf(TEXT("%s, attentive %d"), Topic.Name, bAttentive);
					ASSERT_THAT(IsTrue(StartTopic(Topic, bAttentive), *Where));
					const int32 TrustBefore = Relationships->GetTrust(*Hob);

					// At most three are listed, and the ask and leave replies are always there; Bloom has no order reply.
					const int32 Listed = Talk->GetChoiceTexts().Num();
					ASSERT_THAT(IsTrue(Listed >= 2 && Listed <= 3, *Where));
					ASSERT_THAT(IsTrue(FindReply(TEXT(".Reply.Leave")) != INDEX_NONE, *Where));
					ASSERT_THAT(IsTrue(FindReply(TEXT(".Reply.Ask")) != INDEX_NONE, *Where));
					const bool bHasOrder = FindReply(TEXT(".Reply.Order")) != INDEX_NONE;
					ASSERT_THAT(IsTrue(bHasOrder == (FString(Topic.Name) != TEXT("Bloom")), *Where));
					ASSERT_THAT(IsTrue(Listed == (bHasOrder ? 3 : 2), *Where));

					// Asking is worth a point and sets the flag that the attentive talk reads.
					ASSERT_THAT(IsTrue(Reply(TEXT(".Reply.Ask")), *Where));
					ASSERT_THAT(IsTrue(LineKey().ToString().EndsWith(TEXT(".Answer.Ask")), *Where));
					ASSERT_THAT(AreEqual(Relationships->GetTrust(*Hob), TrustBefore + 1));
					ASSERT_THAT(IsTrue(HasFlag(TEXT("Asked")), *Where));
					ASSERT_THAT(IsTrue(Relationships->HasFlag(HobId(), MakeTag(FString(TEXT("Vestige.Relationship.Topic.")) + Topic.Name)), *Where));
					Talk->Advance();
					ASSERT_THAT(IsFalse(Talk->IsInConversation(), *Where));

					if (!bHasOrder)
					{
						continue;
					}

					// Giving the order is worth nothing, and Hob answers it differently once he is attentive.
					ASSERT_THAT(IsTrue(StartTopic(Topic, bAttentive), *Where));
					ASSERT_THAT(IsTrue(Reply(TEXT(".Reply.Order")), *Where));
					ASSERT_THAT(IsTrue(LineKey().ToString().EndsWith(bAttentive ? TEXT(".Answer.OrderView") : TEXT(".Answer.Order")), *Where));
					ASSERT_THAT(AreEqual(Relationships->GetTrust(*Hob), TrustBefore));
					ASSERT_THAT(IsFalse(HasFlag(TEXT("Asked")), *Where));
					ASSERT_THAT(IsTrue(Relationships->HasFlag(HobId(), MakeTag(FString(TEXT("Vestige.Relationship.Topic.")) + Topic.Name)), *Where));
					Talk->Advance();
					ASSERT_THAT(IsFalse(Talk->IsInConversation(), *Where));
				}
			}
		}

		TEST_METHOD(DueTopicsComeInTheDesignersOrder)
		{
			for (const FTopic& Topic : TopicsInOrder)
			{
				if (Topic.Milestone[0] != 0)
				{
					Reach(Topic.Milestone);
				}
			}
			int32 Day = LateSpringDay;
			for (const FTopic& Topic : TopicsInOrder)
			{
				SetDay(Day);
				ASSERT_THAT(IsTrue(Start(), Topic.Name));
				ASSERT_THAT(IsTrue(LineKey() == FName(*(FString(TEXT("Hob.Topic.")) + Topic.Name + TEXT(".Open"))), Topic.Name));
				ASSERT_THAT(IsTrue(Talk->GetChoiceTexts().Num() <= 3, Topic.Name));
				ASSERT_THAT(IsTrue(Reply(TEXT(".Reply.Leave")), Topic.Name));
				Talk->Advance();
				Day += 2;
			}

			ASSERT_THAT(IsTrue(TalkOn(Day).ToString().StartsWith(TEXT("Hob.Greet."))));
		}

		TEST_METHOD(TheSeasonEndTopicOnlyComesOnSpringDays19To25)
		{
			const UVestigeDialogueData* SeasonEnd = FindDialogue(TEXT("Base.HobTopicSeasonEnd"));
			ASSERT_THAT(IsNotNull(SeasonEnd));
			const int32 DaysPerSeason = Clock->GetCalendar().GetDaysPerSeason();
			const TMap<FGameplayTag, int32> NoFlags;
			const FGameplayTagContainer NoMilestones;

			const auto IsDueOn = [&](int32 Day)
			{
				return SeasonEnd->AreConditionsMet(MakeContext(Day, true, 0, 0, NoFlags, NoMilestones));
			};

			ASSERT_THAT(IsFalse(IsDueOn(18)));
			ASSERT_THAT(IsTrue(IsDueOn(19)));
			ASSERT_THAT(IsTrue(IsDueOn(25)));
			ASSERT_THAT(IsFalse(IsDueOn(26)));
			// The same days of the next season don't count.
			ASSERT_THAT(IsFalse(IsDueOn(DaysPerSeason + 20)));
		}

		TEST_METHOD(NoTopicOnDayOne)
		{
			const TMap<FGameplayTag, int32> NoFlags;
			FGameplayTagContainer Everything;
			for (const FTopic& Topic : TopicsInOrder)
			{
				if (Topic.Milestone[0] != 0)
				{
					Everything.AddTag(MakeTag(Topic.Milestone));
				}
			}

			for (const FTopic& Topic : TopicsInOrder)
			{
				const UVestigeDialogueData* Dialogue = FindDialogue(*(FString(TEXT("Base.HobTopic")) + Topic.Name));
				ASSERT_THAT(IsNotNull(Dialogue, Topic.Name));
				ASSERT_THAT(IsFalse(Dialogue->AreConditionsMet(MakeContext(1, true, 0, 0, NoFlags, Everything)), Topic.Name));
			}
		}

		// ---- Hob's dawn watering, as he says it ----

		TEST_METHOD(TheFirstWateredMorningOutranksATopic)
		{
			SetFlag(TEXT("Helped.Watering"), 5);

			ASSERT_THAT(IsTrue(TalkOn(5).ToString().StartsWith(TEXT("Hob.Watered."))));
			ASSERT_THAT(AreEqual(GetFlagDay(TEXT("WateredMentioned")), 5));

			// The topic that waited comes the next day, a day Hob didn't water.
			ASSERT_THAT(IsTrue(TalkOn(6) == TEXT("Hob.Topic.Dispensers.Open")));
		}

		TEST_METHOD(ALaterWateredMorningWaitsForTheTopic)
		{
			SetFlag(TEXT("WateredMentioned"), 2);
			SetFlag(TEXT("Helped.Watering"), 8);

			ASSERT_THAT(IsTrue(TalkOn(8) == TEXT("Hob.Topic.Dispensers.Open")));
		}

		TEST_METHOD(TheWateredLinePlaysOnlyOnTheDayHobWatered)
		{
			AnswerEveryTopic();
			SetFlag(TEXT("Helped.Watering"), 5);

			ASSERT_THAT(IsTrue(TalkOn(6).ToString().StartsWith(TEXT("Hob.Greet."))));
		}

		TEST_METHOD(ALaterWateredLineWaitsTwoDaysAfterTheLast)
		{
			AnswerEveryTopic();
			SetFlag(TEXT("Step.WouldLike"), 1);
			SetFlag(TEXT("Helped.Watering"), 5);
			ASSERT_THAT(IsTrue(TalkOn(5).ToString().StartsWith(TEXT("Hob.Watered."))));

			// Said on day 5: not on 6 or 7, again on 8.
			for (const int32 Day : {6, 7, 8})
			{
				SetFlag(TEXT("Helped.Watering"), Day);
				const bool bWatered = TalkOn(Day).ToString().StartsWith(TEXT("Hob.Watered."));
				ASSERT_THAT(IsTrue(bWatered == (Day == 8), *FString::Printf(TEXT("day %d"), Day)));
			}
		}

		TEST_METHOD(TheWarmGreetingsPlayOnWateredMorningsBetween)
		{
			TMap<FGameplayTag, int32> Flags = {{Flag(TEXT("Step.WouldLike")), 1}, {Flag(TEXT("Helped.Watering")), 7},
				{Flag(TEXT("WateredMentioned")), 5}, {Flag(TEXT("Topic.Well")), 1}, {Flag(TEXT("Topic.Dispensers")), 1},
				{Flag(TEXT("Topic.FarRows")), 1}};
			const FGameplayTagContainer NoMilestones;

			// Day 7 is two days after the last watered line: Hob helped, and the warm greeting is what he says.
			ASSERT_THAT(IsTrue(TopTalk(MakeContext(7, true, 0, 0, Flags, NoMilestones)) == TEXT("Base.HobGreetingWarmMorning")));

			// Three days after it, the watered line is back.
			Flags.Add(Flag(TEXT("WateredMentioned")), 4);
			ASSERT_THAT(IsTrue(TopTalk(MakeContext(7, true, 0, 0, Flags, NoMilestones)) == TEXT("Base.HobWateredLater")));
		}

		TEST_METHOD(TheWarmGreetingsKeepTheEverydayLines)
		{
			const TCHAR* const Bands[] = {TEXT("Morning"), TEXT("Afternoon"), TEXT("Evening"), TEXT("Late")};
			for (const TCHAR* Band : Bands)
			{
				const UVestigeDialogueData* Everyday = FindDialogue(*(FString(TEXT("Base.HobGreeting")) + Band));
				const UVestigeDialogueData* Warm = FindDialogue(*(FString(TEXT("Base.HobGreetingWarm")) + Band));
				const UVestigeDialogueData* Memory = FindDialogue(*(FString(TEXT("Base.HobGreetingMemory")) + Band));
				ASSERT_THAT(IsTrue(Everyday && Warm && Memory, Band));
				ASSERT_THAT(IsTrue(Everyday->GetLines().Num() == 1 && Warm->GetLines().Num() == 1 && Memory->GetLines().Num() == 1, Band));

				TSet<FName> WarmKeys;
				for (const FText& Text : Warm->GetLines()[0].Texts)
				{
					WarmKeys.Add(GetTableKey(Text));
				}
				TSet<FName> MemoryKeys;
				for (const FText& Text : Memory->GetLines()[0].Texts)
				{
					MemoryKeys.Add(GetTableKey(Text));
				}
				for (const FText& Text : Everyday->GetLines()[0].Texts)
				{
					ASSERT_THAT(IsTrue(WarmKeys.Contains(GetTableKey(Text)), Band));
					ASSERT_THAT(IsTrue(MemoryKeys.Contains(GetTableKey(Text)), Band));
				}
				// The warm pool is bigger than the everyday one, and the memory pool bigger again.
				ASSERT_THAT(IsTrue(Warm->GetLines()[0].Texts.Num() > Everyday->GetLines()[0].Texts.Num(), Band));
				ASSERT_THAT(IsTrue(Memory->GetLines()[0].Texts.Num() > Warm->GetLines()[0].Texts.Num(), Band));
			}
		}

		TEST_METHOD(AWarmerGreetingPlaysFromEachStep)
		{
			const FGameplayTagContainer NoMilestones;
			TMap<FGameplayTag, int32> Flags;
			for (const FTopic& Topic : TopicsInOrder)
			{
				Flags.Add(Flag(*(FString(TEXT("Topic.")) + Topic.Name)), 1);
			}

			ASSERT_THAT(IsTrue(TopTalk(MakeContext(9, true, 0, 0, Flags, NoMilestones)) == TEXT("Base.HobGreetingMorning")));
			Flags.Add(Flag(TEXT("Step.WouldLike")), 3);
			ASSERT_THAT(IsTrue(TopTalk(MakeContext(9, true, 0, 0, Flags, NoMilestones)) == TEXT("Base.HobGreetingWarmMorning")));
			Flags.Add(Flag(TEXT("Step.Memory")), 5);
			ASSERT_THAT(IsTrue(TopTalk(MakeContext(9, true, 0, 0, Flags, NoMilestones)) == TEXT("Base.HobGreetingMemoryMorning")));
		}

		// ---- Always something to say ----

		TEST_METHOD(HobAlwaysHasSomethingToSayAtEveryStepOfTheTrack)
		{
			// Every combination of the track's state, at every hour: one conversation at the top priority, never a tie.
			constexpr int32 Today = 12;
			const FGameplayTag Attentive = Flag(TEXT("Step.Attentive"));
			const FGameplayTag WouldLike = Flag(TEXT("Step.WouldLike"));
			const FGameplayTag Memory = Flag(TEXT("Step.Memory"));
			const FGameplayTag Helped = Flag(TEXT("Helped.Watering"));
			const FGameplayTag Mentioned = Flag(TEXT("WateredMentioned"));
			const FGameplayTag CleanedMilestone = MakeTag(TEXT("Vestige.Milestone.Farm.Cleaned"));

			struct FPeople
			{
				bool bMet;
				int32 TalksToday;
			};
			const FPeople Cases[] = {{false, 0}, {true, 0}, {true, 2}};

			for (const FPeople& Person : Cases)
			{
				for (int32 Bits = 0; Bits < 64; ++Bits)
				{
					const bool bAttentive = Bits & 1;
					const bool bWouldLike = Bits & 2;
					const bool bMemory = Bits & 4;
					const bool bAllTopics = Bits & 8;
					const bool bHelped = Bits & 16;
					const bool bCleaned = Bits & 32;
					for (const int32 MentionedDaysAgo : {-1, 0, 3})
					{
						for (const int32 Trust : {0, 10})
						{
							TMap<FGameplayTag, int32> Flags;
							if (bAttentive)
							{
								Flags.Add(Attentive, 5);
							}
							if (bWouldLike)
							{
								Flags.Add(WouldLike, 8);
							}
							if (bMemory)
							{
								Flags.Add(Memory, 10);
							}
							if (bHelped)
							{
								Flags.Add(Helped, Today);
							}
							if (MentionedDaysAgo >= 0)
							{
								Flags.Add(Mentioned, Today - MentionedDaysAgo);
							}
							if (bAllTopics)
							{
								for (const FTopic& Topic : TopicsInOrder)
								{
									Flags.Add(Flag(*(FString(TEXT("Topic.")) + Topic.Name)), 3);
								}
							}
							FGameplayTagContainer Reached;
							if (bCleaned)
							{
								Reached.AddTag(CleanedMilestone);
							}

							for (float Hour = 6.f; Hour < Clock->GetDayEndHour(); Hour += 1.f)
							{
								const FVestigeDialogueContext Context =
									MakeContext(Today, Person.bMet, Person.TalksToday, Trust, Flags, Reached, Hour);
								const TArray<const UVestigeDialogueData*> Matching = FindHobTalks(Context);
								const FString Where = FString::Printf(TEXT("met %d, talks %d, state bits %d, mentioned %d, trust %d, hour %.0f"),
									Person.bMet, Person.TalksToday, Bits, MentionedDaysAgo, Trust, Hour);
								ASSERT_THAT(IsFalse(Matching.IsEmpty(), *Where));
								const bool bTie = Matching.Num() > 1 && Matching[0]->GetPriority() == Matching[1]->GetPriority();
								ASSERT_THAT(IsFalse(bTie, *Where));
							}
						}
					}
				}
			}
		}
	};
}

#endif
