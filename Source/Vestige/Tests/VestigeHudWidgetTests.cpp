// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/VestigeGameplayTags.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Components/ActorTestSpawner.h"
#include "Components/PanelWidget.h"
#include "CommonGameViewportClient.h"
#include "CommonInputSettings.h"
#include "CommonInputTypeEnum.h"
#include "Components/TextBlock.h"
#include "Core/VestigeFeedbackSubsystem.h"
#include "Content/FarmCropData.h"
#include "Content/VestigeCalendar.h"
#include "Blueprint/WidgetTree.h"
#include "CQTest.h"
#include "Farming/FarmField.h"
#include "Farming/FarmFieldSubsystem.h"
#include "Farming/FarmGrid.h"
#include "Farming/FarmTargetingComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/GameModeBase.h"
#include "Input/UIActionBindingHandle.h"
#include "Player/VestigePlayerCharacter.h"
#include "UObject/UnrealType.h"
#include "Sound/SoundWave.h"
#include "UI/VestigeHUD.h"
#include "UI/VestigeDialogueWidget.h"
#include "UI/VestigeFeedbackResponses.h"
#include "UI/VestigeHudMessageWidget.h"
#include "UI/VestigeHudMessages.h"
#include "UI/VestigeHudWidget.h"
#include "UI/VestigeLayoutWidget.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "VestigeHudWidgetTests"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace VestigeHudWidgetTests
{
	const TCHAR* const HudWidgetPath = TEXT("/Game/Vestige/UI/WBP_Hud.WBP_Hud_C");
	const TCHAR* const MessageWidgetPath = TEXT("/Game/Vestige/UI/WBP_HudMessage.WBP_HudMessage_C");
	const TCHAR* const GameModePath = TEXT("/Game/Vestige/Blueprints/BP_GameMode.BP_GameMode_C");
	constexpr double Lifetime = 3.0;

	/** The text a message box shows. */
	FText GetMessageBoxText(const UWidget* Box)
	{
		const UUserWidget* MessageBox = Cast<UUserWidget>(Box);
		const UTextBlock* Text = MessageBox ? Cast<UTextBlock>(MessageBox->GetWidgetFromName(TEXT("MessageText"))) : nullptr;
		return Text ? Text->GetText() : FText::GetEmpty();
	}

	/** The widgets are presentation, so these check that the assets hold together and that each part follows what it is told. */
	TEST_CLASS(UI, "Vestige")
	{
		FActorTestSpawner Spawner;
		UVestigeHudWidget* Hud = nullptr;

		BEFORE_EACH()
		{
			UClass* HudClass = LoadClass<UVestigeHudWidget>(nullptr, HudWidgetPath);
			ASSERT_THAT(IsNotNull(HudClass));
			Hud = CreateWidget<UVestigeHudWidget>(&Spawner.GetWorld(), HudClass);
			ASSERT_THAT(IsNotNull(Hud));
		}

		TEST_METHOD(TheGameModeShowsTheHudWidget)
		{
			const UClass* GameModeClass = LoadClass<AGameModeBase>(nullptr, GameModePath);
			ASSERT_THAT(IsNotNull(GameModeClass));

			const UClass* HudActorClass = GameModeClass->GetDefaultObject<AGameModeBase>()->HUDClass;
			ASSERT_THAT(IsTrue(HudActorClass && HudActorClass->IsChildOf<AVestigeHUD>()));

			const TSubclassOf<UVestigeHudWidget> WidgetClass = HudActorClass->GetDefaultObject<AVestigeHUD>()->GetHudWidgetClass();
			ASSERT_THAT(IsNotNull(WidgetClass.Get()));
			ASSERT_THAT(IsNotNull(WidgetClass->GetDefaultObject<UVestigeHudWidget>()->GetMessageWidgetClass().Get()));
		}

		TEST_METHOD(OnlyTheMessageBoxesTick)
		{
			const UWidgetBlueprintGeneratedClass* HudClass =
				Cast<UWidgetBlueprintGeneratedClass>(LoadClass<UUserWidget>(nullptr, HudWidgetPath));
			const UWidgetBlueprintGeneratedClass* MessageClass =
				Cast<UWidgetBlueprintGeneratedClass>(LoadClass<UUserWidget>(nullptr, MessageWidgetPath));
			ASSERT_THAT(IsNotNull(HudClass));
			ASSERT_THAT(IsNotNull(MessageClass));

			ASSERT_THAT(IsFalse(HudClass->ClassRequiresNativeTick()));
			ASSERT_THAT(IsTrue(MessageClass->ClassRequiresNativeTick()));
		}

		TEST_METHOD(TheHudDoesNotTakeTheMouse) { ASSERT_THAT(IsTrue(Hud->GetVisibility() == ESlateVisibility::HitTestInvisible)); }

		TEST_METHOD(EachMessageShowsInItsOwnBox)
		{
			FVestigeHudMessages Messages;
			Messages.Add(LOCTEXT("Slept", "You slept."), 1.0, 5);
			Messages.Add(LOCTEXT("Exhausted", "You wake up exhausted."), 1.0, 5);
			Messages.Add(LOCTEXT("Saved", "Game saved."), 1.0, 5);
			Hud->ShowMessages(Messages.GetMessages(), Lifetime);

			const UPanelWidget* MessageList = Cast<UPanelWidget>(Hud->GetWidgetFromName(TEXT("MessageList")));
			ASSERT_THAT(IsNotNull(MessageList));
			ASSERT_THAT(IsTrue(MessageList->GetChildrenCount() == 3));
			for (int32 Index = 0; Index < 3; ++Index)
			{
				ASSERT_THAT(IsTrue(MessageList->GetChildAt(Index)->GetVisibility() == ESlateVisibility::HitTestInvisible));
			}
			ASSERT_THAT(IsTrue(GetMessageBoxText(MessageList->GetChildAt(2)).EqualTo(LOCTEXT("Saved", "Game saved."))));

			Messages.RemoveExpired(1.0 + Lifetime, Lifetime);
			Messages.Add(LOCTEXT("Harvested", "Harvested 2 Turnip."), 5.0, 5);
			Hud->ShowMessages(Messages.GetMessages(), Lifetime);

			ASSERT_THAT(IsTrue(MessageList->GetChildrenCount() == 3));
			ASSERT_THAT(IsTrue(MessageList->GetChildAt(0)->GetVisibility() == ESlateVisibility::HitTestInvisible));
			ASSERT_THAT(IsTrue(GetMessageBoxText(MessageList->GetChildAt(0)).EqualTo(LOCTEXT("Harvested", "Harvested 2 Turnip."))));
			ASSERT_THAT(IsTrue(MessageList->GetChildAt(1)->GetVisibility() == ESlateVisibility::Collapsed));
			ASSERT_THAT(IsTrue(MessageList->GetChildAt(2)->GetVisibility() == ESlateVisibility::Collapsed));
		}

		TEST_METHOD(AMessageFadesInAndOut)
		{
			UClass* MessageClass = LoadClass<UVestigeHudMessageWidget>(nullptr, MessageWidgetPath);
			ASSERT_THAT(IsNotNull(MessageClass));
			UVestigeHudMessageWidget* MessageBox = CreateWidget<UVestigeHudMessageWidget>(&Spawner.GetWorld(), MessageClass);
			ASSERT_THAT(IsNotNull(MessageBox));

			constexpr double StartTime = 10.0;
			MessageBox->ShowMessage(LOCTEXT("Saved", "Game saved."), StartTime, Lifetime);
			for (const double Now : {StartTime, StartTime + VestigeHudMessageFade::FadeInSeconds / 2.0, StartTime + Lifetime / 2.0,
					 StartTime + Lifetime - VestigeHudMessageFade::FadeOutSeconds / 2.0, StartTime + Lifetime})
			{
				MessageBox->UpdateOpacity(Now);
				ASSERT_THAT(IsTrue(
					FMath::IsNearlyEqual(MessageBox->GetRenderOpacity(), FVestigeHudMessages::GetOpacity(StartTime, Now, Lifetime))));
			}
			MessageBox->UpdateOpacity(StartTime + Lifetime / 2.0);
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(MessageBox->GetRenderOpacity(), 1.f)));
		}

		TEST_METHOD(ThePromptHidesWhenEmpty)
		{
			const UWidget* PromptBox = Hud->GetWidgetFromName(TEXT("PromptBox"));
			ASSERT_THAT(IsNotNull(PromptBox));

			Hud->SetPromptText(FText::GetEmpty());
			ASSERT_THAT(IsTrue(PromptBox->GetVisibility() == ESlateVisibility::Collapsed));

			Hud->SetPromptText(LOCTEXT("Sleep", "F Sleep"));
			ASSERT_THAT(IsTrue(PromptBox->GetVisibility() == ESlateVisibility::HitTestInvisible));

			Hud->SetPromptText(FText::GetEmpty());
			ASSERT_THAT(IsTrue(PromptBox->GetVisibility() == ESlateVisibility::Collapsed));
		}

		TEST_METHOD(TheHudHidesThePromptAndHintsDuringATalk)
		{
			const UWidget* PromptBox = Hud->GetWidgetFromName(TEXT("PromptBox"));
			const UWidget* KeyHints = Hud->GetWidgetFromName(TEXT("KeyHintsText"));
			const UWidget* StatusText = Hud->GetWidgetFromName(TEXT("StatusText"));
			ASSERT_THAT(IsNotNull(PromptBox));
			ASSERT_THAT(IsNotNull(KeyHints));
			ASSERT_THAT(IsNotNull(StatusText));
			Hud->SetPromptText(LOCTEXT("Talk", "F Talk to Hob"));
			ASSERT_THAT(IsTrue(PromptBox->GetVisibility() == ESlateVisibility::HitTestInvisible));

			Hud->SetTalking(true);

			ASSERT_THAT(IsTrue(PromptBox->GetVisibility() == ESlateVisibility::Collapsed));
			ASSERT_THAT(IsTrue(KeyHints->GetVisibility() == ESlateVisibility::Collapsed));
			// The clock, date and stamina stay.
			ASSERT_THAT(IsTrue(StatusText->GetVisibility() != ESlateVisibility::Collapsed));

			// A prompt that changes during the talk stays hidden.
			Hud->SetPromptText(LOCTEXT("Sleep", "F Sleep"));
			ASSERT_THAT(IsTrue(PromptBox->GetVisibility() == ESlateVisibility::Collapsed));

			Hud->SetTalking(false);

			ASSERT_THAT(IsTrue(PromptBox->GetVisibility() == ESlateVisibility::HitTestInvisible));
			ASSERT_THAT(IsTrue(KeyHints->GetVisibility() == ESlateVisibility::HitTestInvisible));
		}

		TEST_METHOD(AnEmptyPromptStaysHiddenAfterATalk)
		{
			const UWidget* PromptBox = Hud->GetWidgetFromName(TEXT("PromptBox"));
			ASSERT_THAT(IsNotNull(PromptBox));

			Hud->SetTalking(true);
			Hud->SetTalking(false);

			ASSERT_THAT(IsTrue(PromptBox->GetVisibility() == ESlateVisibility::Collapsed));
		}

		TEST_METHOD(TheGameViewportClientIsCommonUIs)
		{
			// Common UI's action router needs its own viewport client to see the keys; the engine reads the setting at startup.
			const UEngine* EngineDefaults = GetDefault<UEngine>();
			ASSERT_THAT(IsTrue(EngineDefaults->GameViewportClientClassName.ToString().EndsWith(TEXT("CommonUI.CommonGameViewportClient"))));
			if (GEngine && GEngine->GameViewportClientClass)
			{
				ASSERT_THAT(IsTrue(GEngine->GameViewportClientClass->IsChildOf(UCommonGameViewportClient::StaticClass())));
			}
		}

		TEST_METHOD(EnhancedInputSupportIsOn)
		{
			// Without it a box's input mapping context is never added.
			ASSERT_THAT(IsTrue(GetDefault<UCommonInputSettings>()->GetEnableEnhancedInputSupport()));
		}

		TEST_METHOD(TheGameModeShowsTheLayoutAndTheBox)
		{
			const UClass* GameModeClass = LoadClass<AGameModeBase>(nullptr, GameModePath);
			ASSERT_THAT(IsNotNull(GameModeClass));
			const UClass* HudActorClass = GameModeClass->GetDefaultObject<AGameModeBase>()->HUDClass;
			ASSERT_THAT(IsTrue(HudActorClass && HudActorClass->IsChildOf<AVestigeHUD>()));
			const AVestigeHUD* HudDefaults = HudActorClass->GetDefaultObject<AVestigeHUD>();

			ASSERT_THAT(IsNotNull(HudDefaults->GetLayoutWidgetClass().Get()));
			ASSERT_THAT(IsNotNull(HudDefaults->GetDialogueWidgetClass().Get()));
			ASSERT_THAT(IsFalse(HudDefaults->GetDialogueWidgetClass()->HasAnyClassFlags(CLASS_Abstract)));
		}

		TEST_METHOD(TheHudTakesTheGameInputAndTheBoxTheMenuInput)
		{
			const TOptional<FUIInputConfig> HudConfig = Hud->GetDesiredInputConfig();
			ASSERT_THAT(IsTrue(HudConfig.IsSet()));
			ASSERT_THAT(IsTrue(HudConfig->GetInputMode() == ECommonInputMode::Game));

			const UClass* GameModeClass = LoadClass<AGameModeBase>(nullptr, GameModePath);
			ASSERT_THAT(IsNotNull(GameModeClass));
			const AVestigeHUD* HudDefaults = GameModeClass->GetDefaultObject<AGameModeBase>()->HUDClass->GetDefaultObject<AVestigeHUD>();
			const UVestigeDialogueWidget* BoxDefaults = HudDefaults->GetDialogueWidgetClass()->GetDefaultObject<UVestigeDialogueWidget>();
			const TOptional<FUIInputConfig> BoxConfig = BoxDefaults->GetDesiredInputConfig();
			ASSERT_THAT(IsTrue(BoxConfig.IsSet()));
			ASSERT_THAT(IsTrue(BoxConfig->GetInputMode() == ECommonInputMode::Menu));
		}

		TEST_METHOD(TheHudIsInvisibleToTheMouseWhenActive)
		{
			// Common UI sets the visibility again when its stack activates the widget; the Game input mode must not become a click-eater.
			Hud->ActivateWidget();

			ASSERT_THAT(IsTrue(Hud->IsActivated()));
			ASSERT_THAT(IsTrue(Hud->GetVisibility() == ESlateVisibility::HitTestInvisible));
		}

		TEST_METHOD(TheStaminaLineWarnsWhileOutOfStamina)
		{
			const UTextBlock* StaminaText = Cast<UTextBlock>(Hud->GetWidgetFromName(TEXT("StaminaText")));
			ASSERT_THAT(IsNotNull(StaminaText));

			Hud->SetStamina(LOCTEXT("Full", "Stamina 100 / 100"), 100, 100, false, false);
			const FLinearColor Normal = StaminaText->GetColorAndOpacity().GetSpecifiedColor();

			Hud->SetStamina(LOCTEXT("Empty", "Stamina 0 / 100"), 0, 100, true, false);
			ASSERT_THAT(IsFalse(StaminaText->GetColorAndOpacity().GetSpecifiedColor().Equals(Normal)));
			ASSERT_THAT(IsTrue(StaminaText->GetText().EqualTo(LOCTEXT("Empty", "Stamina 0 / 100"))));

			Hud->SetStamina(LOCTEXT("Full", "Stamina 100 / 100"), 100, 100, false, false);
			ASSERT_THAT(IsTrue(StaminaText->GetColorAndOpacity().GetSpecifiedColor().Equals(Normal)));
		}

		TEST_METHOD(TheHintDeviceFollowsTheInputType)
		{
			// The game has no touch controls, so a touch leaves the keyboard hints.
			ASSERT_THAT(IsTrue(AVestigeHUD::GetHintDevice(ECommonInputType::Gamepad) == EVestigeHintDevice::Gamepad));
			ASSERT_THAT(IsTrue(AVestigeHUD::GetHintDevice(ECommonInputType::MouseAndKeyboard) == EVestigeHintDevice::KeyboardAndMouse));
			ASSERT_THAT(IsTrue(AVestigeHUD::GetHintDevice(ECommonInputType::Touch) == EVestigeHintDevice::KeyboardAndMouse));
		}

		TEST_METHOD(TheFeedbackResponsesFormatTheReport)
		{
			UVestigeFeedbackResponses* Responses = NewObject<UVestigeFeedbackResponses>();
			FVestigeFeedbackResponse DayBegins;
			DayBegins.Message = LOCTEXT("DayBegins", "Day {Day} begins.");
			Responses->SetResponsesForTests({{VestigeGameplayTags::Feedback_Day_Slept, DayBegins}});

			const FVestigeFeedbackResponse* Response = Responses->FindResponse(VestigeGameplayTags::Feedback_Day_Slept);
			ASSERT_THAT(IsNotNull(Response));
			const FVestigeFeedbackReport Report{VestigeGameplayTags::Feedback_Day_Slept, {{TEXT("Day"), 4}}};
			ASSERT_THAT(IsTrue(Responses->FormatMessage(*Response, Report).ToString() == TEXT("Day 4 begins.")));
			ASSERT_THAT(IsNull(Responses->FindResponse(VestigeGameplayTags::Feedback_Day_StayedUp)));
		}

		/** A made-up calendar of 28-day seasons, so the date texts don't depend on the game's names. */
		static FVestigeCalendar MakeCalendar(const TArray<FVestigeSeason>& Seasons) { return FVestigeCalendar::MakeForTests(28, Seasons); }

		TEST_METHOD(TheDateShowsTheSeasonAndTheDay)
		{
			FVestigeSeason Spring;
			Spring.Tag = FGameplayTag::RequestGameplayTag(FName(TEXT("Vestige.Season.Spring")));
			Spring.DisplayName = LOCTEXT("TestSpring", "Spring");
			const FVestigeCalendar Calendar = MakeCalendar({Spring});

			const auto Format = [&Calendar](int32 Day)
			{
				const FVestigeDate Date = Calendar.GetDate(Day);
				return AVestigeHUD::FormatDate(Date, Calendar.GetSeasonName(Date)).ToString();
			};

			ASSERT_THAT(AreEqual(Format(3), FString(TEXT("Spring, Day 3"))));
			ASSERT_THAT(AreEqual(Format(1), FString(TEXT("Spring, Day 1"))));
			ASSERT_THAT(AreEqual(Format(28), FString(TEXT("Spring, Day 28"))));
			// The days left are for the morning message, not the status line.
			ASSERT_THAT(IsFalse(Format(27).Contains(TEXT("left"))));
			ASSERT_THAT(IsFalse(Format(28).Contains(TEXT("last"))));
		}

		TEST_METHOD(TheDateWithoutASeasonShowsTheDayCount)
		{
			const FVestigeCalendar Calendar = MakeCalendar({});
			const FVestigeDate Date = Calendar.GetDate(30);

			ASSERT_THAT(AreEqual(AVestigeHUD::FormatDate(Date, Calendar.GetSeasonName(Date)).ToString(), FString(TEXT("Day 30"))));
		}

		/** A crop that takes Days to ripen. */
		UFarmCropData& MakeCrop(int32 Days)
		{
			UFarmCropData& Crop = Spawner.SpawnObject<UFarmCropData>();
			Crop.SetDaysToMatureForTests(Days);
			return Crop;
		}

		TEST_METHOD(SeedsInHandShowTheirDaysToRipen)
		{
			const FText Seeds = FText::FromString(TEXT("Cabbage Seeds"));

			ASSERT_THAT(AreEqual(
				AVestigeHUD::FormatItemInHand(Seeds, &MakeCrop(10)).ToString(), FString(TEXT("Cabbage Seeds (10 days to ripen)"))));
		}

		TEST_METHOD(ASeedOfOneDayShowsOneDay)
		{
			const FText Seeds = FText::FromString(TEXT("Radish Seeds"));

			ASSERT_THAT(
				AreEqual(AVestigeHUD::FormatItemInHand(Seeds, &MakeCrop(1)).ToString(), FString(TEXT("Radish Seeds (1 day to ripen)"))));
		}

		TEST_METHOD(OtherItemsInHandShowTheirNameOnly)
		{
			const FText Hoe = FText::FromString(TEXT("Hoe"));

			ASSERT_THAT(AreEqual(AVestigeHUD::FormatItemInHand(Hoe, nullptr).ToString(), FString(TEXT("Hoe"))));
		}

		TEST_METHOD(TheCanInHandShowsItsWater)
		{
			const FText Can = FText::FromString(TEXT("Watering Can"));

			ASSERT_THAT(AreEqual(AVestigeHUD::FormatItemInHand(Can, nullptr, 5, 8).ToString(), FString(TEXT("Watering Can (5/8)"))));
			ASSERT_THAT(AreEqual(AVestigeHUD::FormatItemInHand(Can, nullptr, 0, 8).ToString(), FString(TEXT("Watering Can (0/8)"))));
		}

		TEST_METHOD(TheLateWarningAgreesWithItsNumbers)
		{
			const UClass* GameModeClass = LoadClass<AGameModeBase>(nullptr, GameModePath);
			ASSERT_THAT(IsNotNull(GameModeClass));
			const UClass* HudActorClass = GameModeClass->GetDefaultObject<AGameModeBase>()->HUDClass;
			ASSERT_THAT(IsTrue(HudActorClass && HudActorClass->IsChildOf<AVestigeHUD>()));
			const UVestigeFeedbackResponses* Responses = HudActorClass->GetDefaultObject<AVestigeHUD>()->GetFeedbackResponses();
			ASSERT_THAT(IsNotNull(Responses));
			const FVestigeFeedbackResponse* Response = Responses->FindResponse(VestigeGameplayTags::Feedback_Farm_TooLate);
			ASSERT_THAT(IsNotNull(Response));

			const auto Format = [Response](int32 DaysToRipe, int32 DaysToDeadline)
			{
				FFormatNamedArguments Values;
				Values.Add(TEXT("DaysToRipe"), DaysToRipe);
				Values.Add(TEXT("DaysToDeadline"), DaysToDeadline);
				Values.Add(TEXT("DeadlineSeasonInSentence"), FText::FromString(TEXT("summer")));
				return FText::Format(Response->Message, Values).ToString();
			};

			const FString One = Format(1, 1);
			ASSERT_THAT(IsTrue(One.Contains(TEXT("1 day to ripen")), One));
			ASSERT_THAT(IsTrue(One.Contains(TEXT("1 day is left")), One));
			const FString Many = Format(10, 8);
			ASSERT_THAT(IsTrue(Many.Contains(TEXT("10 days to ripen")), Many));
			ASSERT_THAT(IsTrue(Many.Contains(TEXT("8 days are left")), Many));
		}

		TEST_METHOD(TheWitheredLineAgreesWithItsCount)
		{
			const UClass* GameModeClass = LoadClass<AGameModeBase>(nullptr, GameModePath);
			ASSERT_THAT(IsNotNull(GameModeClass));
			const UClass* HudActorClass = GameModeClass->GetDefaultObject<AGameModeBase>()->HUDClass;
			ASSERT_THAT(IsTrue(HudActorClass && HudActorClass->IsChildOf<AVestigeHUD>()));
			const UVestigeFeedbackResponses* Responses = HudActorClass->GetDefaultObject<AVestigeHUD>()->GetFeedbackResponses();
			ASSERT_THAT(IsNotNull(Responses));
			const FVestigeFeedbackResponse* Response = Responses->FindResponse(VestigeGameplayTags::Feedback_Farm_Withered);
			ASSERT_THAT(IsNotNull(Response));

			const auto Format = [Response](int32 Count)
			{
				FFormatNamedArguments Values;
				Values.Add(TEXT("Count"), Count);
				Values.Add(TEXT("SeasonInSentence"), FText::FromString(TEXT("summer")));
				return FText::Format(Response->Message, Values).ToString();
			};

			ASSERT_THAT(AreEqual(Format(1), FString(TEXT("1 crop withered. It doesn't grow in summer."))));
			ASSERT_THAT(AreEqual(Format(3), FString(TEXT("3 crops withered. They don't grow in summer."))));
		}

#if WITH_EDITOR
		TEST_METHOD(TheFeedbackResponsesRefuseALoopingSound)
		{
			const auto Validate = [](bool bLooping)
			{
				USoundWave* Sound = NewObject<USoundWave>();
				Sound->bLooping = bLooping;
				FVestigeFeedbackResponse Row;
				Row.Message = LOCTEXT("TestMessage", "Test.");
				Row.Sound = Sound;
				UVestigeFeedbackResponses* Responses = NewObject<UVestigeFeedbackResponses>();
				Responses->SetResponsesForTests({{VestigeGameplayTags::Feedback_Water_CanFilled, Row}});
				FDataValidationContext Context;
				return Responses->IsDataValid(Context);
			};

			ASSERT_THAT(IsTrue(Validate(true) == EDataValidationResult::Invalid));
			ASSERT_THAT(IsTrue(Validate(false) != EDataValidationResult::Invalid));
		}
#endif

		TEST_METHOD(TheMoneyLineShowsCoinsAndTheAmount)
		{
			ASSERT_THAT(AreEqual(AVestigeHUD::FormatMoney(100).ToString(), FString(TEXT("Coins 100"))));
			ASSERT_THAT(AreEqual(AVestigeHUD::FormatMoney(0).ToString(), FString(TEXT("Coins 0"))));
			// The number is written as the current culture writes it ("1,500" in English, "1 500" in Swedish), so the test
			// doesn't hard-code a culture.
			ASSERT_THAT(AreEqual(AVestigeHUD::FormatMoney(1500).ToString(), FString(TEXT("Coins ")) + FText::AsNumber(1500).ToString()));
			ASSERT_THAT(
				AreEqual(AVestigeHUD::FormatMoney(1234567).ToString(), FString(TEXT("Coins ")) + FText::AsNumber(1234567).ToString()));
		}

		// Fails until MoneyText is added to WBP_Hud in the widget designer by hand (Docs/Systems/25-Selling.md, editor step 5).
		TEST_METHOD(TheHudWidgetHasTheMoneyLine)
		{
			UTextBlock* MoneyText = Cast<UTextBlock>(Hud->GetWidgetFromName(TEXT("MoneyText")));
			ASSERT_THAT(IsNotNull(MoneyText));

			Hud->SetMoneyText(AVestigeHUD::FormatMoney(250));

			ASSERT_THAT(AreEqual(MoneyText->GetText().ToString(), FString(TEXT("Coins 250"))));
		}

		/** The game's HUD Blueprint's defaults, through the game mode, as the game makes it. */
		const AVestigeHUD* GetGameHudDefaults()
		{
			const UClass* GameModeClass = LoadClass<AGameModeBase>(nullptr, GameModePath);
			const UClass* HudActorClass = GameModeClass ? GameModeClass->GetDefaultObject<AGameModeBase>()->HUDClass.Get() : nullptr;
			return HudActorClass && HudActorClass->IsChildOf<AVestigeHUD>() ? HudActorClass->GetDefaultObject<AVestigeHUD>() : nullptr;
		}

		TEST_METHOD(TheTileLineNamesEachContaminationLevelAndNothingOnCleanSoil)
		{
			const AVestigeHUD* HudDefaults = GetGameHudDefaults();
			ASSERT_THAT(IsNotNull(HudDefaults));

			ASSERT_THAT(IsTrue(HudDefaults->GetTileText(0).IsEmpty()));
			ASSERT_THAT(IsTrue(HudDefaults->GetTileText(-1).IsEmpty()));
			ASSERT_THAT(AreEqual(HudDefaults->GetTileText(1).ToString(), FString(TEXT("Lightly contaminated"))));
			ASSERT_THAT(AreEqual(HudDefaults->GetTileText(2).ToString(), FString(TEXT("Heavily contaminated"))));
			// A level past the names, from a fourth level in the settings, keeps the last name.
			ASSERT_THAT(AreEqual(HudDefaults->GetTileText(3).ToString(), FString(TEXT("Heavily contaminated"))));
		}

		// Fails until TileText is added to WBP_Hud in the widget designer by hand (Docs/Systems/27-Contamination.md, the HUD line).
		TEST_METHOD(TheHudWidgetHasTheTileLine)
		{
			UTextBlock* TileText = Cast<UTextBlock>(Hud->GetWidgetFromName(TEXT("TileText")));
			ASSERT_THAT(IsNotNull(TileText));

			Hud->SetTileText(LOCTEXT("Light", "Lightly contaminated"));
			ASSERT_THAT(AreEqual(TileText->GetText().ToString(), FString(TEXT("Lightly contaminated"))));
			Hud->SetTileText(FText::GetEmpty());
			ASSERT_THAT(IsTrue(TileText->GetText().IsEmpty()));
		}

		// The tile line's binding (Systems/27-Contamination.md, "Binding lifetime"). The HUD is spawned without a player, so
		// it makes no widget of its own: the test gives it the WBP_Hud made above and a pawn to follow.

		/**
		 * The widget's TileText. Until it is added to WBP_Hud by hand, the widget gets one bound by reflection, as the
		 * designer's binding would, so these tests can read the line either way.
		 */
		UTextBlock* TileTextBlock()
		{
			// The bound property, not the tree: a block made here isn't in the widget's hierarchy, so a name search misses it.
			FObjectProperty* Property = FindFProperty<FObjectProperty>(UVestigeHudWidget::StaticClass(), TEXT("TileText"));
			check(Property);
			if (UTextBlock* Existing = Cast<UTextBlock>(Property->GetObjectPropertyValue_InContainer(Hud)))
			{
				return Existing;
			}
			UTextBlock* Text = Hud->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TileText"));
			Property->SetObjectPropertyValue_InContainer(Hud, Text);
			return Text;
		}

		FString TileLine() { return TileTextBlock()->GetText().ToString(); }

		/** A grid of the default 8 x 8 field Base.Farm at the world's origin, begun play unless told not to. */
		AFarmGrid& SpawnGrid(bool bBeginPlay = true)
		{
			AFarmGrid& Grid = Spawner.SpawnActor<AFarmGrid>();
			if (bBeginPlay)
			{
				Grid.DispatchBeginPlay();
			}
			return Grid;
		}

		/** The game's farmer at Location facing +X, begun play, so its targeting component has found the world's grid. */
		AVestigePlayerCharacter& SpawnFarmer(const FVector& Location = FVector(250.f, 50.f, 0.f))
		{
			UClass* FarmerClass =
				LoadClass<AVestigePlayerCharacter>(nullptr, TEXT("/Game/Vestige/Blueprints/BP_PlayerCharacter.BP_PlayerCharacter_C"));
			check(FarmerClass);
			AVestigePlayerCharacter& Farmer = Spawner.SpawnActor<AVestigePlayerCharacter>(FActorSpawnParameters(), FarmerClass);
			Farmer.DispatchBeginPlay();
			Farmer.SetActorLocationAndRotation(Location, FRotator::ZeroRotator);
			return Farmer;
		}

		/** A HUD that has begun play without a player, shows the fixture's widget and follows Pawn. */
		AVestigeHUD& SpawnHudFollowing(APawn* Pawn)
		{
			TileTextBlock();
			AVestigeHUD& HudActor = Spawner.SpawnActor<AVestigeHUD>();
			HudActor.DispatchBeginPlay();
			HudActor.SetHudWidgetForTests(Hud);
			HudActor.FollowPawnForTests(Pawn);
			return HudActor;
		}

		static UFarmTargetingComponent& TargetingOf(APawn& Pawn) { return *Pawn.FindComponentByClass<UFarmTargetingComponent>(); }

		/** Whether HudActor listens to any of Field's tile events. */
		static bool FollowsField(const UFarmField& Field, const AVestigeHUD& HudActor)
		{
			return Field.OnTileChanged.IsBoundToObject(&HudActor) || Field.OnTilesChanged.IsBoundToObject(&HudActor);
		}

		TEST_METHOD(WithoutAGridTheTileLineShowsNothing)
		{
			Spawner.InitializeGameSubsystems();
			AVestigePlayerCharacter& Farmer = SpawnFarmer();
			ASSERT_THAT(IsNull(TargetingOf(Farmer).GetGrid()));
			Hud->SetTileText(LOCTEXT("Stale", "Stale"));

			SpawnHudFollowing(&Farmer);

			ASSERT_THAT(IsTrue(TileLine().IsEmpty()));
		}

		TEST_METHOD(TheTileLineWaitsForAGridsFieldThatIsReadyLater)
		{
			Spawner.InitializeGameSubsystems();
			AFarmGrid& Grid = SpawnGrid(false);
			AVestigePlayerCharacter& Farmer = SpawnFarmer();
			ASSERT_THAT(IsTrue(TargetingOf(Farmer).GetGrid() == &Grid));
			ASSERT_THAT(IsFalse(Grid.IsFieldReady()));

			AVestigeHUD& HudActor = SpawnHudFollowing(&Farmer);
			ASSERT_THAT(IsTrue(Grid.OnFieldReady.IsBoundToObject(&HudActor)));
			ASSERT_THAT(IsTrue(TileLine().IsEmpty()));

			Grid.DispatchBeginPlay();

			UFarmField* Field = Grid.GetField();
			ASSERT_THAT(IsNotNull(Field));
			ASSERT_THAT(IsFalse(Grid.OnFieldReady.IsBoundToObject(&HudActor)));
			ASSERT_THAT(IsTrue(FollowsField(*Field, HudActor)));
			ASSERT_THAT(IsTrue(TargetingOf(Farmer).GetTargetTile() == TOptional<FIntPoint>(FIntPoint(3, 0))));
			Field->SetContamination(FIntPoint(3, 0), 1);
			ASSERT_THAT(AreEqual(TileLine(), FString(TEXT("Lightly contaminated"))));
		}

		TEST_METHOD(TheTileLineFollowsTheTargetAndItsTile)
		{
			Spawner.InitializeGameSubsystems();
			AFarmGrid& Grid = SpawnGrid();
			UFarmField& Field = *Grid.GetField();
			Field.SetContamination(FIntPoint(3, 0), 1);
			Field.SetContamination(FIntPoint(4, 0), 2);
			AVestigePlayerCharacter& Farmer = SpawnFarmer();
			SpawnHudFollowing(&Farmer);
			ASSERT_THAT(AreEqual(TileLine(), FString(TEXT("Lightly contaminated"))));

			// The target moves one tile on.
			Farmer.SetActorLocation(FVector(350.f, 50.f, 0.f));
			ASSERT_THAT(IsTrue(TargetingOf(Farmer).GetTargetTile() == TOptional<FIntPoint>(FIntPoint(4, 0))));
			ASSERT_THAT(AreEqual(TileLine(), FString(TEXT("Heavily contaminated"))));

			// Another tile's change leaves the line; the target tile's change redraws it.
			Field.SetContamination(FIntPoint(3, 0), 0);
			ASSERT_THAT(AreEqual(TileLine(), FString(TEXT("Heavily contaminated"))));
			Field.SetContamination(FIntPoint(4, 0), 0);
			ASSERT_THAT(IsTrue(TileLine().IsEmpty()));
			Field.SetContamination(FIntPoint(4, 0), 1);
			ASSERT_THAT(AreEqual(TileLine(), FString(TEXT("Lightly contaminated"))));

			// Off the field, no target and no line.
			Farmer.SetActorLocation(FVector(-500.f, -500.f, 0.f));
			ASSERT_THAT(IsTrue(TileLine().IsEmpty()));
		}

		TEST_METHOD(ATargetOnAnotherGridsFieldLetsGoOfTheOldField)
		{
			Spawner.InitializeGameSubsystems();
			AFarmGrid& OldGrid = SpawnGrid();
			UFarmField* OldField = OldGrid.GetField();
			AVestigePlayerCharacter& OldFarmer = SpawnFarmer();
			AVestigeHUD& HudActor = SpawnHudFollowing(&OldFarmer);
			ASSERT_THAT(IsTrue(FollowsField(*OldField, HudActor)));

			// A new game drops the field; the map's next grid makes a new one, and the next pawn targets that grid.
			UFarmFieldSubsystem::Get(&Spawner.GetWorld())->StartNewGame();
			OldGrid.Destroy();
			AFarmGrid& NewGrid = SpawnGrid();
			UFarmField* NewField = NewGrid.GetField();
			ASSERT_THAT(IsTrue(NewField && NewField != OldField));
			AVestigePlayerCharacter& NewFarmer = SpawnFarmer();
			ASSERT_THAT(IsTrue(TargetingOf(NewFarmer).GetGrid() == &NewGrid));

			HudActor.FollowPawnForTests(&NewFarmer);

			ASSERT_THAT(IsFalse(FollowsField(*OldField, HudActor)));
			ASSERT_THAT(IsFalse(TargetingOf(OldFarmer).OnTargetTileChanged.IsBoundToObject(&HudActor)));
			ASSERT_THAT(IsTrue(FollowsField(*NewField, HudActor)));
			OldField->SetContamination(FIntPoint(3, 0), 2);
			ASSERT_THAT(IsTrue(TileLine().IsEmpty()));
			NewField->SetContamination(FIntPoint(3, 0), 2);
			ASSERT_THAT(AreEqual(TileLine(), FString(TEXT("Heavily contaminated"))));
		}

		TEST_METHOD(NothingCallsTheHudAfterThePawnChangesOrTheHudEnds)
		{
			Spawner.InitializeGameSubsystems();
			AFarmGrid& Grid = SpawnGrid(false);
			AVestigePlayerCharacter& Farmer = SpawnFarmer();

			// While it waits for the field: a pawn change lets go of the grid.
			AVestigeHUD& HudActor = SpawnHudFollowing(&Farmer);
			ASSERT_THAT(IsTrue(Grid.OnFieldReady.IsBoundToObject(&HudActor)));
			HudActor.FollowPawnForTests(nullptr);
			ASSERT_THAT(IsFalse(Grid.OnFieldReady.IsBoundToObject(&HudActor)));
			ASSERT_THAT(IsFalse(TargetingOf(Farmer).OnTargetTileChanged.IsBoundToObject(&HudActor)));

			// While it waits: the HUD's end lets go of the grid.
			AVestigeHUD& WaitingHud = SpawnHudFollowing(&Farmer);
			ASSERT_THAT(IsTrue(Grid.OnFieldReady.IsBoundToObject(&WaitingHud)));
			WaitingHud.Destroy();
			ASSERT_THAT(IsFalse(Grid.OnFieldReady.IsBoundToObject(&WaitingHud)));
			ASSERT_THAT(IsFalse(TargetingOf(Farmer).OnTargetTileChanged.IsBoundToObject(&WaitingHud)));

			// Following the field: a pawn change, then the HUD's end, let go of the field, which outlives them both.
			Grid.DispatchBeginPlay();
			UFarmField& Field = *Grid.GetField();
			HudActor.FollowPawnForTests(&Farmer);
			ASSERT_THAT(IsTrue(FollowsField(Field, HudActor)));
			HudActor.FollowPawnForTests(nullptr);
			ASSERT_THAT(IsFalse(FollowsField(Field, HudActor)));
			ASSERT_THAT(IsTrue(TileLine().IsEmpty()));

			HudActor.FollowPawnForTests(&Farmer);
			ASSERT_THAT(IsTrue(FollowsField(Field, HudActor)));
			HudActor.Destroy();
			ASSERT_THAT(IsFalse(FollowsField(Field, HudActor)));
			ASSERT_THAT(IsFalse(TargetingOf(Farmer).OnTargetTileChanged.IsBoundToObject(&HudActor)));
		}

		/** A responses asset with a day row that ends in the Paid clause and the Paid row, as the game's has. */
		static UVestigeFeedbackResponses* MakeClauseResponses(bool bWithPaidRow)
		{
			FVestigeFeedbackResponse DayRow;
			DayRow.Message = LOCTEXT("ClauseDayRow", "Day {Day} begins. {Paid}");
			FVestigeFeedbackResponse PaidRow;
			PaidRow.Message = LOCTEXT("ClausePaidRow", "The bin paid {Amount}.");
			TMap<FGameplayTag, FVestigeFeedbackResponse> Rows = {{VestigeGameplayTags::Feedback_Day_Slept, DayRow}};
			if (bWithPaidRow)
			{
				Rows.Add(VestigeGameplayTags::Feedback_Shipping_Paid, PaidRow);
			}
			UVestigeFeedbackResponses* Responses = NewObject<UVestigeFeedbackResponses>();
			Responses->SetResponsesForTests(MoveTemp(Rows));
			return Responses;
		}

		static FVestigeFeedbackReport MakeDayReport(const FGameplayTag& PaidTag)
		{
			FVestigeFeedbackReport Report{VestigeGameplayTags::Feedback_Day_Slept, {{TEXT("Day"), 4}}};
			Report.Clauses.Add(FVestigeFeedbackClause{TEXT("Paid"), PaidTag, {{TEXT("Amount"), 250}}});
			return Report;
		}

		TEST_METHOD(AClauseFillsItsArgumentWithItsRowsText)
		{
			const UVestigeFeedbackResponses* Responses = MakeClauseResponses(true);
			const FVestigeFeedbackResponse* Row = Responses->FindResponse(VestigeGameplayTags::Feedback_Day_Slept);
			ASSERT_THAT(IsNotNull(Row));

			const FText Message = Responses->FormatMessage(*Row, MakeDayReport(VestigeGameplayTags::Feedback_Shipping_Paid));

			ASSERT_THAT(AreEqual(Message.ToString(), FString(TEXT("Day 4 begins. The bin paid 250."))));
		}

		TEST_METHOD(AClauseWithoutATagIsEmptyAndLeavesNoTrailingSpace)
		{
			const UVestigeFeedbackResponses* Responses = MakeClauseResponses(true);
			const FVestigeFeedbackResponse* Row = Responses->FindResponse(VestigeGameplayTags::Feedback_Day_Slept);
			ASSERT_THAT(IsNotNull(Row));

			const FText Message = Responses->FormatMessage(*Row, MakeDayReport(FGameplayTag()));

			ASSERT_THAT(AreEqual(Message.ToString(), FString(TEXT("Day 4 begins."))));
		}

		TEST_METHOD(AClauseWithoutARowIsEmptyAndWarns)
		{
			const UVestigeFeedbackResponses* Responses = MakeClauseResponses(false);
			const FVestigeFeedbackResponse* Row = Responses->FindResponse(VestigeGameplayTags::Feedback_Day_Slept);
			ASSERT_THAT(IsNotNull(Row));
			TestRunner->AddExpectedMessagePlain(TEXT("has no text for the clause Paid"), ELogVerbosity::Warning);

			const FText Message = Responses->FormatMessage(*Row, MakeDayReport(VestigeGameplayTags::Feedback_Shipping_Paid));

			ASSERT_THAT(AreEqual(Message.ToString(), FString(TEXT("Day 4 begins."))));
		}

		TEST_METHOD(EveryFeedbackTagHasAResponse)
		{
			const UClass* GameModeClass = LoadClass<AGameModeBase>(nullptr, GameModePath);
			ASSERT_THAT(IsNotNull(GameModeClass));
			const UClass* HudActorClass = GameModeClass->GetDefaultObject<AGameModeBase>()->HUDClass;
			ASSERT_THAT(IsTrue(HudActorClass && HudActorClass->IsChildOf<AVestigeHUD>()));
			const UVestigeFeedbackResponses* Responses = HudActorClass->GetDefaultObject<AVestigeHUD>()->GetFeedbackResponses();
			ASSERT_THAT(IsNotNull(Responses));

			// Each native feedback tag with the values its reporter gives. Listed here rather than asked of the tag manager,
			// whose children of Vestige.Feedback include the in-between tags (Vestige.Feedback.Farm).
			const TArray<FString> NoValues;
			const TArray<FString> DateValues = {
				TEXT("Day"), TEXT("Season"), TEXT("SeasonInSentence"), TEXT("DayOfSeason"), TEXT("DaysLeft")};
			const TArray<FString> SeasonStartValues = {
				TEXT("Day"), TEXT("Season"), TEXT("SeasonInSentence"), TEXT("DayOfSeason"), TEXT("DaysLeft"), TEXT("PreviousSeason")};
			const TArray<FString> OutOfSeasonValues = {TEXT("Day"), TEXT("Season"), TEXT("SeasonInSentence"), TEXT("DayOfSeason"),
				TEXT("DaysLeft"), TEXT("DaysToRipe"), TEXT("PlantSeason"), TEXT("PlantSeasonInSentence")};
			const TArray<FString> TooLateValues = {TEXT("Day"), TEXT("Season"), TEXT("SeasonInSentence"), TEXT("DayOfSeason"),
				TEXT("DaysLeft"), TEXT("DaysToRipe"), TEXT("DaysToDeadline"), TEXT("DeadlineSeason"), TEXT("DeadlineSeasonInSentence")};
			const TArray<FString> WitheredValues = {
				TEXT("Day"), TEXT("Season"), TEXT("SeasonInSentence"), TEXT("DayOfSeason"), TEXT("DaysLeft"), TEXT("Count")};
			const TArray<FString> ItemAndCountValues = {TEXT("Item"), TEXT("Count")};
			const TArray<FString> AmountValues = {TEXT("Amount")};
			const TArray<FString> BoughtValues = {TEXT("Item"), TEXT("Count"), TEXT("Price")};
			const TArray<FString> PriceValues = {TEXT("Price")};
			// The day reports end with the Paid clause (what the shipping bin paid in the night).
			TArray<FString> DayReportValues = DateValues;
			DayReportValues.Add(TEXT("Paid"));
			TArray<FString> NewSeasonDayReportValues = SeasonStartValues;
			NewSeasonDayReportValues.Add(TEXT("Paid"));
			const TMap<FGameplayTag, TArray<FString>> Expected = {
				{VestigeGameplayTags::Feedback_Tool_NoTarget, NoValues},
				{VestigeGameplayTags::Feedback_Tool_NothingInHand, NoValues},
				{VestigeGameplayTags::Feedback_Farm_CantTill, NoValues},
				{VestigeGameplayTags::Feedback_Farm_NothingToPlant, NoValues},
				{VestigeGameplayTags::Feedback_Farm_CantPlant, NoValues},
				{VestigeGameplayTags::Feedback_Farm_OutOfSeason, OutOfSeasonValues},
				{VestigeGameplayTags::Feedback_Farm_TooLate, TooLateValues},
				{VestigeGameplayTags::Feedback_Farm_TooLate_LastDay, TooLateValues},
				{VestigeGameplayTags::Feedback_Farm_CantWater, NoValues},
				{VestigeGameplayTags::Feedback_Farm_Soaked, NoValues},
				{VestigeGameplayTags::Feedback_Farm_NothingRipe, NoValues},
				{VestigeGameplayTags::Feedback_Farm_Withered, WitheredValues},
				{VestigeGameplayTags::Feedback_Farm_ClearFirst, NoValues},
				{VestigeGameplayTags::Feedback_Farm_NeedsContamination, NoValues},
				{VestigeGameplayTags::Feedback_Farm_TooContaminated, NoValues},
				{VestigeGameplayTags::Feedback_Farm_Cleaned, NoValues},
				{VestigeGameplayTags::Feedback_Farm_Cleaned_Clean, NoValues},
				{VestigeGameplayTags::Feedback_Inventory_Full, NoValues},
				{VestigeGameplayTags::Feedback_Water_CanEmpty, NoValues},
				{VestigeGameplayTags::Feedback_Water_CanFull, NoValues},
				{VestigeGameplayTags::Feedback_Water_CanFilled, NoValues},
				{VestigeGameplayTags::Feedback_Water_WellDry, NoValues},
				{VestigeGameplayTags::Feedback_Water_WellRanDry, NoValues},
				{VestigeGameplayTags::Feedback_Shipping_Shipped, ItemAndCountValues},
				{VestigeGameplayTags::Feedback_Shipping_NothingToShip, NoValues},
				{VestigeGameplayTags::Feedback_Shipping_Paid, AmountValues},
				{VestigeGameplayTags::Feedback_Shop_Bought, BoughtValues},
				{VestigeGameplayTags::Feedback_Shop_NotEnoughMoney, PriceValues},
				{VestigeGameplayTags::Feedback_Stamina_Empty, NoValues},
				{VestigeGameplayTags::Feedback_Rest_GettingLate, NoValues},
				{VestigeGameplayTags::Feedback_Rest_PastMidnight, NoValues},
				{VestigeGameplayTags::Feedback_Rest_WokeExhausted, NoValues},
				{VestigeGameplayTags::Feedback_Rest_WokeRested, NoValues},
				{VestigeGameplayTags::Feedback_Day_Slept, DayReportValues},
				{VestigeGameplayTags::Feedback_Day_Slept_NewSeason, NewSeasonDayReportValues},
				{VestigeGameplayTags::Feedback_Day_Slept_LastDay, DayReportValues},
				{VestigeGameplayTags::Feedback_Day_StayedUp, DayReportValues},
				{VestigeGameplayTags::Feedback_Day_StayedUp_NewSeason, NewSeasonDayReportValues},
				{VestigeGameplayTags::Feedback_Day_StayedUp_LastDay, DayReportValues},
				{VestigeGameplayTags::Feedback_Save_Loaded, DateValues},
				{VestigeGameplayTags::Feedback_Save_Loaded_LastDay, DateValues},
				{VestigeGameplayTags::Feedback_Save_Saved, NoValues},
				{VestigeGameplayTags::Feedback_Save_Failed, NoValues},
				{VestigeGameplayTags::Feedback_Save_ContentMissing, NoValues},
				{VestigeGameplayTags::Feedback_Save_Unreadable, NoValues},
				{VestigeGameplayTags::Feedback_Save_TooNew, NoValues},
				{VestigeGameplayTags::Feedback_Save_PartTooNew, NoValues},
				{VestigeGameplayTags::Feedback_Dialogue_NothingToSay, NoValues},
			};

			ASSERT_THAT(IsTrue(Responses->GetResponses().Num() == Expected.Num()));
			for (const TPair<FGameplayTag, TArray<FString>>& Entry : Expected)
			{
				const FVestigeFeedbackResponse* Response = Responses->FindResponse(Entry.Key);
				ASSERT_THAT(IsNotNull(Response, Entry.Key.ToString()));

				ASSERT_THAT(IsFalse(Response->Message.IsEmpty(), Entry.Key.ToString()));

				const FTextFormat Format(Response->Message);
				ASSERT_THAT(IsTrue(Format.IsValid(), Entry.Key.ToString()));
				TArray<FString> Names;
				Format.GetFormatArgumentNames(Names);
				for (const FString& Name : Names)
				{
					// Case-sensitive, as FText::Format matches names, so a {day} typo fails.
					const bool bAllowed = Entry.Value.ContainsByPredicate(
						[&Name](const FString& Allowed)
						{
							return Allowed.Equals(Name, ESearchCase::CaseSensitive);
						});
					ASSERT_THAT(IsTrue(bAllowed, Entry.Key.ToString() + TEXT(" uses {") + Name + TEXT("}")));
				}
			}
		}
	};
}

#undef LOCTEXT_NAMESPACE

#endif
