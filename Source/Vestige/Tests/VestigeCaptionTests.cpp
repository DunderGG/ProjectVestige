// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Characters/VestigeConversationComponent.h"
#include "Components/ActorTestSpawner.h"
#include "Components/Widget.h"
#include "Content/VestigeCharacterData.h"
#include "Content/VestigeContentCatalog.h"
#include "Content/VestigeDialogueData.h"
#include "CQTest.h"
#include "Engine/World.h"
#include "Internationalization/Text.h"
#include "Player/VestigePlayerState.h"
#include "Tests/VestigeTestCaptionWidget.h"
#include "TimerManager.h"
#include "UI/VestigeCaptionWidget.h"
#include "UI/VestigeHudMessages.h"
#include "UI/VestigeHudWidget.h"

// The caption that shows a line said in passing (Systems/06-UserInterface.md, "The caption"). Its behavior runs on a test subclass
// with the two text blocks WBP_Caption binds, so these tests do not wait for the widget designer; TheHudHasTheCaption is the one
// that does, and fails until WBP_Caption is placed in WBP_Hud.

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace VestigeCaptionTests
{
	const TCHAR* const HudWidgetPath = TEXT("/Game/Vestige/UI/WBP_Hud.WBP_Hud_C");
	const FName TestSpeakerId(TEXT("Test.Speaker"));
	const TCHAR* const SpeakerName = TEXT("Tester");
	const TCHAR* const LineText = TEXT("A line in passing.");

	TEST_CLASS(Caption, "Vestige")
	{
		FActorTestSpawner Spawner;
		UVestigeContentCatalog* Catalog = nullptr;
		UVestigeCharacterData* Speaker = nullptr;
		UVestigeConversationComponent* Talk = nullptr;
		AActor* SpeakerActor = nullptr;
		UVestigeTestCaptionWidget* Widget = nullptr;

		/** A speaker with one line to say in passing, the player's conversation component, and a caption that follows it. */
		BEFORE_EACH()
		{
			Spawner.InitializeGameSubsystems();
			Catalog = UVestigeContentCatalog::Get(&Spawner.GetWorld());
			ASSERT_THAT(IsNotNull(Catalog));

			AVestigePlayerState& PlayerState = Spawner.SpawnActor<AVestigePlayerState>();
			PlayerState.DispatchBeginPlay();
			Talk = PlayerState.FindComponentByClass<UVestigeConversationComponent>();
			ASSERT_THAT(IsNotNull(Talk));

			Speaker = &Spawner.SpawnObject<UVestigeCharacterData>();
			Speaker->SetContentIdForTests(TestSpeakerId);
			Speaker->SetDisplayNameForTests(FText::AsCultureInvariant(SpeakerName));
			ASSERT_THAT(IsTrue(Catalog->AddForTests(*Speaker)));
			SpeakerActor = &Spawner.SpawnActor<AActor>();

			FVestigeDialogueLine Line;
			Line.LineId = TEXT("Test.Passing");
			Line.Texts.Add(FText::AsCultureInvariant(LineText));
			UVestigeDialogueData& Dialogue = Spawner.SpawnObject<UVestigeDialogueData>();
			Dialogue.SetContentIdForTests(TEXT("Test.Passing"));
			Dialogue.SetSpeakerForTests(Speaker->GetPrimaryAssetId());
			Dialogue.SetPriorityForTests(10);
			Dialogue.SetDeliveryForTests(EVestigeDialogueDelivery::Passing);
			Dialogue.SetLinesForTests({Line});
			Catalog->AddForTests(Dialogue);

			Widget = CreateWidget<UVestigeTestCaptionWidget>(&Spawner.GetWorld(), UVestigeTestCaptionWidget::StaticClass());
			ASSERT_THAT(IsNotNull(Widget));
			Widget->MakeParts();
		}

		bool Say() { return Talk->SayPassingLine(*SpeakerActor, *Speaker); }

		/**
		 * Runs the world's timers for Seconds. A test world's timer manager ticks once a frame and starts a timer set since its last
		 * tick only at its next one, so this ticks twice, each in a frame of its own: the first starts the timers, the second runs them.
		 */
		void TickTimers(double Seconds)
		{
			FTimerManager& Timers = Spawner.GetWorld().GetTimerManager();
			++GFrameCounter;
			Timers.Tick(0.f);
			++GFrameCounter;
			Timers.Tick(static_cast<float>(Seconds));
		}

		// Fails until WBP_Caption is made in the widget designer and placed in WBP_Hud's overlay (Docs/PlayTest.md, the caption).
		TEST_METHOD(TheHudHasTheCaption)
		{
			UClass* HudClass = LoadClass<UVestigeHudWidget>(nullptr, HudWidgetPath);
			ASSERT_THAT(IsNotNull(HudClass));
			UVestigeHudWidget* Hud = CreateWidget<UVestigeHudWidget>(&Spawner.GetWorld(), HudClass);
			ASSERT_THAT(IsNotNull(Hud));
			ASSERT_THAT(IsNotNull(Hud->WidgetTree.Get()));

			TArray<UWidget*> Widgets;
			Hud->WidgetTree->GetAllWidgets(Widgets);
			int32 Captions = 0;
			for (const UWidget* InTree : Widgets)
			{
				Captions += Cast<UVestigeCaptionWidget>(InTree) ? 1 : 0;
			}

			ASSERT_THAT(IsTrue(Captions == 1));
		}

		TEST_METHOD(TheCaptionIsHiddenUntilALineIsSaid)
		{
			Widget->SetConversationForTests(Talk);

			ASSERT_THAT(IsFalse(Widget->IsCaptionShown()));
			ASSERT_THAT(IsTrue(Widget->GetVisibility() == ESlateVisibility::Collapsed));
		}

		TEST_METHOD(TheCaptionShowsTheSpeakerAndTheLine)
		{
			Widget->SetConversationForTests(Talk);

			ASSERT_THAT(IsTrue(Say()));

			ASSERT_THAT(IsTrue(Widget->IsCaptionShown()));
			ASSERT_THAT(IsTrue(Widget->GetSpeakerNameForTests().ToString() == SpeakerName));
			ASSERT_THAT(IsTrue(Widget->GetLineForTests().ToString() == LineText));
			ASSERT_THAT(IsTrue(Widget->GetVisibility() == ESlateVisibility::HitTestInvisible));
		}

		TEST_METHOD(TheCaptionShowsALineAlreadyBeingSaid)
		{
			// A caption made while a line shows, as when the HUD is rebuilt in the middle of it.
			ASSERT_THAT(IsTrue(Say()));

			Widget->SetConversationForTests(Talk);

			ASSERT_THAT(IsTrue(Widget->IsCaptionShown()));
			ASSERT_THAT(IsTrue(Widget->GetLineForTests().ToString() == LineText));
		}

		TEST_METHOD(TheCaptionHidesWhenTheLineEnds)
		{
			Widget->SetConversationForTests(Talk);
			ASSERT_THAT(IsTrue(Say()));
			ASSERT_THAT(IsTrue(Widget->IsCaptionShown()));

			TickTimers(Talk->GetPassingLineDuration() + 0.1);

			ASSERT_THAT(IsFalse(Talk->IsSayingPassingLine()));
			ASSERT_THAT(IsFalse(Widget->IsCaptionShown()));
			ASSERT_THAT(IsTrue(Widget->GetVisibility() == ESlateVisibility::Collapsed));
		}

		TEST_METHOD(TheCaptionStopsFollowingWhenToldToFollowNothing)
		{
			Widget->SetConversationForTests(Talk);
			Widget->SetConversationForTests(nullptr);

			ASSERT_THAT(IsTrue(Say()));

			// The line is said and the caption never hears of it.
			ASSERT_THAT(IsFalse(Widget->IsCaptionShown()));
		}

		TEST_METHOD(TheCaptionDoesNotTakeTheMouse)
		{
			// Never a visibility that hits: not before, not while it shows.
			const auto TakesNoMouse = [this]()
			{
				const ESlateVisibility Visibility = Widget->GetVisibility();
				return Visibility == ESlateVisibility::Collapsed || Visibility == ESlateVisibility::HitTestInvisible;
			};
			ASSERT_THAT(IsTrue(TakesNoMouse()));
			Widget->SetConversationForTests(Talk);
			ASSERT_THAT(IsTrue(Say()));

			ASSERT_THAT(IsTrue(Widget->GetVisibility() == ESlateVisibility::HitTestInvisible));
			ASSERT_THAT(IsTrue(TakesNoMouse()));
		}

		TEST_METHOD(TheCaptionFadesWithTheLinesTime)
		{
			Widget->SetConversationForTests(Talk);
			ASSERT_THAT(IsTrue(Say()));
			const double Start = Talk->GetPassingLineStartTime();
			const double Duration = Talk->GetPassingLineDuration();
			UWorld& World = Spawner.GetWorld();
			ASSERT_THAT(IsTrue(Duration > 1.0));

			// Just appeared: clear.
			ASSERT_THAT(IsTrue(FMath::IsNearlyZero(Widget->GetRenderOpacity())));

			// Half way through the fade in.
			World.TimeSeconds = Start + VestigeHudMessageFade::FadeInSeconds * 0.5;
			Widget->TickForTests(0.016f);
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Widget->GetRenderOpacity(), 0.5f, 0.01f)));

			// Fully there for most of its time, the line's own and not the messages'.
			World.TimeSeconds = Start + Duration * 0.5;
			Widget->TickForTests(0.016f);
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Widget->GetRenderOpacity(), 1.f)));

			// Half way through the fade out, at the line's end.
			World.TimeSeconds = Start + Duration - VestigeHudMessageFade::FadeOutSeconds * 0.5;
			Widget->TickForTests(0.016f);
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Widget->GetRenderOpacity(), 0.5f, 0.01f)));
		}
	};
}

#endif
