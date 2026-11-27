// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "Characters/VestigeConversationComponent.h"
#include "Characters/VestigeRelationshipComponent.h"
#include "Components/ActorTestSpawner.h"
#include "Content/VestigeCharacterData.h"
#include "Content/VestigeContentCatalog.h"
#include "Content/VestigeDialogueData.h"
#include "CQTest.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Internationalization/Text.h"
#include "Player/VestigePlayerCharacter.h"
#include "Player/VestigePlayerState.h"
#include "UI/VestigeDialogueChoiceButton.h"
#include "UI/VestigeDialogueWidget.h"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace VestigeDialogueBoxTests
{
	const TCHAR* const BoxPath = TEXT("/Game/Vestige/UI/WBP_Dialogue.WBP_Dialogue_C");
	// A function, not a static: UVestigeCharacterData::AssetType is another file's static, which a unity build may initialize after this one.
	FPrimaryAssetId HobId()
	{
		return FPrimaryAssetId(UVestigeCharacterData::AssetType, TEXT("Base.Hob"));
	}

	/** The key a text has in its string table, or an empty name. */
	FName GetTableKey(const FText& Text)
	{
		FName TableId;
		FString Key;
		return FTextInspector::GetTableIdAndKey(Text, TableId, Key) ? FName(*Key) : NAME_None;
	}

	/** The dialogue box on a test world, with a local player, so it finds the player state's conversation as it does in play. */
	TEST_CLASS(DialogueBox, "Vestige")
	{
		FActorTestSpawner Spawner;
		UVestigeContentCatalog* Catalog = nullptr;
		AVestigePlayerState* PlayerState = nullptr;
		UVestigeConversationComponent* Talk = nullptr;
		const UVestigeCharacterData* Hob = nullptr;
		AActor* SpeakerActor = nullptr;
		UVestigeDialogueWidget* Box = nullptr;

		BEFORE_EACH()
		{
			Spawner.InitializeGameSubsystems();
			Catalog = UVestigeContentCatalog::Get(&Spawner.GetWorld());
			ASSERT_THAT(IsNotNull(Catalog));
			Hob = Catalog->Find<UVestigeCharacterData>(HobId());
			ASSERT_THAT(IsNotNull(Hob));
			PlayerState = &Spawner.SpawnActor<AVestigePlayerState>();
			PlayerState->DispatchBeginPlay();
			Talk = PlayerState->FindComponentByClass<UVestigeConversationComponent>();
			ASSERT_THAT(IsNotNull(Talk));
			SpeakerActor = &Spawner.SpawnActor<AActor>();

			// A widget finds the player state through its local player's controller.
			ASSERT_THAT(IsNotNull(GEngine));
			ULocalPlayer* LocalPlayer = NewObject<ULocalPlayer>(GEngine, GEngine->LocalPlayerClass);
			APlayerController& Controller = Spawner.SpawnActor<APlayerController>();
			Controller.PlayerState = PlayerState;
			Controller.Player = LocalPlayer;
			LocalPlayer->PlayerController = &Controller;

			UClass* BoxClass = LoadClass<UVestigeDialogueWidget>(nullptr, BoxPath);
			ASSERT_THAT(IsNotNull(BoxClass));
			Box = CreateWidget<UVestigeDialogueWidget>(&Controller, BoxClass);
			ASSERT_THAT(IsNotNull(Box));
		}

		/** Starts the talk with Hob and shows the box, in the order the game does. */
		bool OpenTalk()
		{
			if (!Talk->StartConversation(*SpeakerActor, *Hob))
			{
				return false;
			}
			Box->ActivateWidget();
			return Box->IsActivated();
		}

		FName LineKey() const { return GetTableKey(Talk->GetLineText()); }

		/** Opens the first meeting and moves to the line with the replies, with its typing unfinished. */
		bool OpenTheReplies()
		{
			if (!OpenTalk())
			{
				return false;
			}
			Box->PressNext();
			Box->PressNext();
			return LineKey() == TEXT("Hob.Meet.020");
		}

		TEST_METHOD(TheBoxStartsTypingTheLine)
		{
			ASSERT_THAT(IsTrue(OpenTalk()));

			ASSERT_THAT(IsTrue(Box->IsTyping()));
			ASSERT_THAT(IsTrue(Box->GetShownLineForTests().IsEmpty()));
			ASSERT_THAT(IsTrue(LineKey() == TEXT("Hob.Meet.010")));
		}

		TEST_METHOD(APressWhileTypingShowsTheWholeLine)
		{
			ASSERT_THAT(IsTrue(OpenTalk()));

			Box->PressNext();

			// The press only finishes the line: the talk is still on it.
			ASSERT_THAT(IsFalse(Box->IsTyping()));
			ASSERT_THAT(IsTrue(Box->GetShownLineForTests().ToString() == Talk->GetLineText().ToString()));
			ASSERT_THAT(IsTrue(LineKey() == TEXT("Hob.Meet.010")));
		}

		TEST_METHOD(TheNextPressAdvances)
		{
			ASSERT_THAT(IsTrue(OpenTalk()));
			Box->PressNext();

			Box->PressNext();

			ASSERT_THAT(IsTrue(LineKey() == TEXT("Hob.Meet.020")));
			// The new line types out from the start.
			ASSERT_THAT(IsTrue(Box->IsTyping()));
			ASSERT_THAT(IsTrue(Box->GetShownLineForTests().IsEmpty()));
		}

		TEST_METHOD(RepliesShowOnlyAfterTheLineIsTyped)
		{
			ASSERT_THAT(IsTrue(OpenTheReplies()));
			ASSERT_THAT(IsTrue(Box->IsTyping()));
			ASSERT_THAT(IsFalse(Box->IsReplyShownForTests(0)));

			Box->PressNext();

			ASSERT_THAT(IsFalse(Box->IsTyping()));
			ASSERT_THAT(IsTrue(Box->IsReplyShownForTests(0)));
			ASSERT_THAT(IsTrue(Box->IsReplyShownForTests(1)));
			ASSERT_THAT(IsTrue(Box->IsReplyShownForTests(2)));
			ASSERT_THAT(IsFalse(Box->IsReplyShownForTests(3)));
			// Still on the same line: the press finished the typing and nothing else.
			ASSERT_THAT(IsTrue(LineKey() == TEXT("Hob.Meet.020")));
		}

		TEST_METHOD(TheRepliesShowTheirTexts)
		{
			ASSERT_THAT(IsTrue(OpenTheReplies()));
			Box->PressNext();

			const TArray<TObjectPtr<UVestigeDialogueChoiceButton>>& Buttons = Box->GetChoiceButtonsForTests();
			const TArray<FText>& Choices = Talk->GetChoiceTexts();
			ASSERT_THAT(IsTrue(Buttons.Num() == 3 && Choices.Num() == 3));
			for (int32 Index = 0; Index < 3; ++Index)
			{
				ASSERT_THAT(IsTrue(Buttons[Index]->GetChoiceIndex() == Index));
			}
		}

		TEST_METHOD(TheFocusIsOnTheFirstReply)
		{
			ASSERT_THAT(IsTrue(OpenTheReplies()));

			// With no replies the box itself takes the keys, so the game doesn't.
			ASSERT_THAT(IsTrue(Box->GetDesiredFocusTarget() == Box));

			Box->PressNext();
			const TArray<TObjectPtr<UVestigeDialogueChoiceButton>>& Buttons = Box->GetChoiceButtonsForTests();
			ASSERT_THAT(IsFalse(Buttons.IsEmpty()));
			ASSERT_THAT(IsTrue(Box->GetDesiredFocusTarget() == Buttons[0]));

			// Going on from the replies takes the focus back to the box.
			Box->PressNext();
			ASSERT_THAT(IsTrue(LineKey() == TEXT("Hob.Meet.Answer.Thanks")));
			ASSERT_THAT(IsTrue(Box->GetDesiredFocusTarget() == Box));
		}

		TEST_METHOD(AReplyClickChoosesIt)
		{
			ASSERT_THAT(IsTrue(OpenTheReplies()));
			Box->PressNext();
			const TArray<TObjectPtr<UVestigeDialogueChoiceButton>>& Buttons = Box->GetChoiceButtonsForTests();
			ASSERT_THAT(IsTrue(Buttons.Num() == 3));

			Buttons[1]->OnChoiceClicked.Broadcast(Buttons[1]->GetChoiceIndex());

			ASSERT_THAT(IsTrue(LineKey() == TEXT("Hob.Meet.Answer.Name")));
			ASSERT_THAT(IsFalse(Box->IsReplyShownForTests(0)));
			ASSERT_THAT(IsTrue(Box->IsTyping()));
		}

		TEST_METHOD(ANextPressOnTheRepliesChoosesTheFirstWhenNoneIsFocused)
		{
			ASSERT_THAT(IsTrue(OpenTheReplies()));
			Box->PressNext();

			Box->PressNext();

			ASSERT_THAT(IsTrue(LineKey() == TEXT("Hob.Meet.Answer.Thanks")));
		}

		TEST_METHOD(TheHintNamesTheKeyAndWhatItDoes)
		{
			ASSERT_THAT(IsTrue(OpenTalk()));
			const FString Typing = Box->GetHintForTests().ToString();
			ASSERT_THAT(IsTrue(Typing.Contains(TEXT("F")) && Typing.Contains(TEXT("Next"))));

			Box->PressNext();
			Box->PressNext();
			Box->PressNext();
			const FString Replies = Box->GetHintForTests().ToString();
			ASSERT_THAT(IsTrue(Replies.Contains(TEXT("Choose"))));
		}

		TEST_METHOD(TheHintSaysCloseOnTheLastLine)
		{
			// The first meeting is over, so the same day's talk is one last line.
			ASSERT_THAT(IsTrue(Talk->StartConversation(*SpeakerActor, *Hob)));
			Talk->EndConversation();
			ASSERT_THAT(IsTrue(OpenTalk()));
			ASSERT_THAT(IsTrue(Talk->IsLastLine()));

			ASSERT_THAT(IsTrue(Box->GetHintForTests().ToString().Contains(TEXT("Close"))));
		}

		TEST_METHOD(TheBoxMakesAtMostThreeReplyButtons)
		{
			UVestigeCharacterData& Tester = Spawner.SpawnObject<UVestigeCharacterData>();
			Tester.SetContentIdForTests(TEXT("Test.Speaker"));
			ASSERT_THAT(IsTrue(Catalog->AddForTests(Tester)));
			FVestigeDialogueLine Question;
			Question.LineId = TEXT("Ask");
			Question.Texts = {FText::AsCultureInvariant(TEXT("ask"))};
			for (const TCHAR* Name : {TEXT("A"), TEXT("B"), TEXT("C"), TEXT("D")})
			{
				FVestigeDialogueChoice Choice;
				Choice.ReplyId = Name;
				Choice.Text = FText::AsCultureInvariant(Name);
				Choice.Next = TEXT("Ask");
				Question.Choices.Add(Choice);
			}
			UVestigeDialogueData& Dialogue = Spawner.SpawnObject<UVestigeDialogueData>();
			Dialogue.SetContentIdForTests(TEXT("Test.FourReplies"));
			Dialogue.SetSpeakerForTests(Tester.GetPrimaryAssetId());
			Dialogue.SetLinesForTests({Question});
			Catalog->AddForTests(Dialogue);
			// Four replies hold at once, which is a data mistake: the line lists the first three and says so.
			TestRunner->AddExpectedMessagePlain(TEXT("replies holding at once"), ELogVerbosity::Warning);
			ASSERT_THAT(IsTrue(Talk->StartConversation(*SpeakerActor, Tester)));
			ASSERT_THAT(IsTrue(Talk->GetChoiceTexts().Num() == VestigeDialogue::MaxChoices));
			Box->ActivateWidget();

			Box->PressNext();

			ASSERT_THAT(IsTrue(Box->GetChoiceButtonsForTests().Num() == VestigeDialogue::MaxChoices));
			ASSERT_THAT(IsTrue(Box->IsReplyShownForTests(2)));
			ASSERT_THAT(IsFalse(Box->IsReplyShownForTests(3)));
		}

		TEST_METHOD(TheBoxClosesWhenTheTalkEnds)
		{
			ASSERT_THAT(IsTrue(OpenTalk()));

			Talk->EndConversation();

			ASSERT_THAT(IsFalse(Box->IsActivated()));
		}

		TEST_METHOD(TheEndPressEndsTheTalkAndClosesTheBox)
		{
			ASSERT_THAT(IsTrue(OpenTheReplies()));

			Box->PressEnd();

			ASSERT_THAT(IsFalse(Talk->IsInConversation()));
			ASSERT_THAT(IsFalse(Box->IsActivated()));
		}

		TEST_METHOD(TheLastLineClosesTheBox)
		{
			ASSERT_THAT(IsTrue(Talk->StartConversation(*SpeakerActor, *Hob)));
			Talk->EndConversation();
			ASSERT_THAT(IsTrue(OpenTalk()));
			Box->PressNext();

			Box->PressNext();

			ASSERT_THAT(IsFalse(Talk->IsInConversation()));
			ASSERT_THAT(IsFalse(Box->IsActivated()));
		}

		TEST_METHOD(AClosedBoxIgnoresPresses)
		{
			ASSERT_THAT(IsTrue(OpenTalk()));
			Talk->EndConversation();
			ASSERT_THAT(IsTrue(Talk->StartConversation(*SpeakerActor, *Hob)));

			// A box that closed no longer follows the talk, so it can't move the next one.
			Box->PressNext();
			Box->PressEnd();

			ASSERT_THAT(IsTrue(Talk->IsInConversation()));
		}

		TEST_METHOD(TheBoxTakesTheMenuInput)
		{
			const TOptional<FUIInputConfig> Config = Box->GetDesiredInputConfig();

			ASSERT_THAT(IsTrue(Config.IsSet()));
			ASSERT_THAT(IsTrue(Config->GetInputMode() == ECommonInputMode::Menu));
		}

		TEST_METHOD(TheBoxContextOutranksThePawns)
		{
			// Read by reflection: the priority is a protected setting of Common UI's activatable widget.
			const FIntProperty* BoxPriority = FindFProperty<FIntProperty>(UCommonActivatableWidget::StaticClass(), TEXT("InputMappingPriority"));
			ASSERT_THAT(IsNotNull(BoxPriority));
			const int32 Priority = BoxPriority->GetPropertyValue_InContainer(Box->GetClass()->GetDefaultObject());
			ASSERT_THAT(IsTrue(Priority == VestigeDialogue::InputMappingPriority));

			UClass* FarmerClass =
				LoadClass<AVestigePlayerCharacter>(nullptr, TEXT("/Game/Vestige/Blueprints/BP_PlayerCharacter.BP_PlayerCharacter_C"));
			ASSERT_THAT(IsNotNull(FarmerClass));
			const AVestigePlayerCharacter* Farmer = FarmerClass->GetDefaultObject<AVestigePlayerCharacter>();
			ASSERT_THAT(IsFalse(Farmer->GetMappingContextsForTests().IsEmpty()));
			for (const FVestigeMappingContextAndPriority& Entry : Farmer->GetMappingContextsForTests())
			{
				ASSERT_THAT(IsTrue(Priority > Entry.Priority));
			}
		}
	};
}

#endif
