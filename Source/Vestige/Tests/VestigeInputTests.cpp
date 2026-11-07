// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/VestigeGameplayTags.h"
#include "Components/ActorTestSpawner.h"
#include "CQTest.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Player/VestigeInputConfig.h"
#include "Player/VestigePlayerCharacter.h"
#include "PlayerMappableKeySettings.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace VestigeInputTests
{
	TEST_CLASS(Input, "Vestige")
	{
		FActorTestSpawner Spawner;

		/** The game's character Blueprint, which names the player's input config and mapping contexts. */
		static UClass* LoadFarmerClass()
		{
			return LoadClass<AVestigePlayerCharacter>(nullptr, TEXT("/Game/Vestige/Blueprints/BP_PlayerCharacter.BP_PlayerCharacter_C"));
		}

		static const AVestigePlayerCharacter* GetFarmerDefaults()
		{
			const UClass* FarmerClass = LoadFarmerClass();
			return FarmerClass ? FarmerClass->GetDefaultObject<AVestigePlayerCharacter>() : nullptr;
		}

		/** Every mapping of the farmer's contexts, in context order and then mapping order. */
		static TArray<FEnhancedActionKeyMapping> GetFarmerMappings(const AVestigePlayerCharacter& Defaults)
		{
			TArray<FEnhancedActionKeyMapping> Mappings;
			for (const FVestigeMappingContextAndPriority& Entry : Defaults.GetMappingContextsForTests())
			{
				if (Entry.MappingContext)
				{
					Mappings.Append(Entry.MappingContext->GetMappings());
				}
			}
			return Mappings;
		}

		static bool IsKeyboardOrMouseKey(const FKey& Key) { return !Key.IsGamepadKey() && !Key.IsTouch() && !Key.IsGesture(); }

		static bool IsHotbarSlotTag(const FGameplayTag& Tag)
		{
			for (const FNativeGameplayTag* SlotTag : VestigeGameplayTags::Input_HotbarSlots)
			{
				if (SlotTag->GetTag() == Tag)
				{
					return true;
				}
			}
			return false;
		}

		/** The input tags the character binds. */
		static TArray<FGameplayTag> GetBoundTags()
		{
			TArray<FGameplayTag> Tags = {VestigeGameplayTags::Input_UseTool, VestigeGameplayTags::Input_Interact,
				VestigeGameplayTags::Input_HotbarNext, VestigeGameplayTags::Input_HotbarPrevious};
			for (const FNativeGameplayTag* SlotTag : VestigeGameplayTags::Input_HotbarSlots)
			{
				Tags.Add(SlotTag->GetTag());
			}
			return Tags;
		}

		static bool HasBinding(const UEnhancedInputComponent& Input, const UInputAction* Action, ETriggerEvent Event)
		{
			for (const TUniquePtr<FEnhancedInputActionEventBinding>& Binding : Input.GetActionEventBindings())
			{
				if (Binding->GetAction() == Action && Binding->GetTriggerEvent() == Event)
				{
					return true;
				}
			}
			return false;
		}

		static bool HasBindingForActionNamed(const UEnhancedInputComponent& Input, const TCHAR* ActionName)
		{
			for (const TUniquePtr<FEnhancedInputActionEventBinding>& Binding : Input.GetActionEventBindings())
			{
				if (Binding->GetAction() && Binding->GetAction()->GetName() == ActionName)
				{
					return true;
				}
			}
			return false;
		}

		TEST_METHOD(ThePlayerBindsEveryConfigEntry)
		{
			UClass* FarmerClass = LoadFarmerClass();
			ASSERT_THAT(IsNotNull(FarmerClass));
			AVestigePlayerCharacter& Farmer = Spawner.SpawnActor<AVestigePlayerCharacter>(FActorSpawnParameters(), FarmerClass);
			const UVestigeInputConfig* Config = Farmer.GetInputConfigForTests();
			ASSERT_THAT(IsNotNull(Config));
			for (const FGameplayTag& Tag : GetBoundTags())
			{
				ASSERT_THAT(IsNotNull(Config->FindNativeInputActionForTag(Tag, true)));
			}
			UEnhancedInputComponent* EnhancedInput = NewObject<UEnhancedInputComponent>(&Farmer);

			Farmer.SetupPlayerInputComponentForTests(*EnhancedInput);

			for (const FVestigeInputAction& Entry : Config->GetNativeInputActions())
			{
				ASSERT_THAT(IsTrue(HasBinding(*EnhancedInput, Entry.InputAction, ETriggerEvent::Started)));
			}
			const UInputAction* UseTool = Config->FindNativeInputActionForTag(VestigeGameplayTags::Input_UseTool, true);
			ASSERT_THAT(IsTrue(HasBinding(*EnhancedInput, UseTool, ETriggerEvent::Completed)));

			// The template still binds moving, looking and jumping through its own properties.
			ASSERT_THAT(IsTrue(HasBindingForActionNamed(*EnhancedInput, TEXT("IA_Move"))));
			ASSERT_THAT(IsTrue(HasBindingForActionNamed(*EnhancedInput, TEXT("IA_Look"))));
			ASSERT_THAT(IsTrue(HasBindingForActionNamed(*EnhancedInput, TEXT("IA_MouseLook"))));
			ASSERT_THAT(IsTrue(HasBindingForActionNamed(*EnhancedInput, TEXT("IA_Jump"))));
		}

		TEST_METHOD(EveryFunctionHasAKeyboardAndAGamepadKey)
		{
			const AVestigePlayerCharacter* Defaults = GetFarmerDefaults();
			ASSERT_THAT(IsNotNull(Defaults));
			const UVestigeInputConfig* Config = Defaults->GetInputConfigForTests();
			ASSERT_THAT(IsNotNull(Config));
			const TArray<FEnhancedActionKeyMapping> Mappings = GetFarmerMappings(*Defaults);

			for (const FVestigeInputAction& Entry : Config->GetNativeInputActions())
			{
				int32 FirstKeyboard = INDEX_NONE;
				int32 FirstGamepad = INDEX_NONE;
				for (int32 Index = 0; Index < Mappings.Num(); ++Index)
				{
					if (Mappings[Index].Action != Entry.InputAction)
					{
						continue;
					}
					if (IsKeyboardOrMouseKey(Mappings[Index].Key) && FirstKeyboard == INDEX_NONE)
					{
						FirstKeyboard = Index;
					}
					else if (Mappings[Index].Key.IsGamepadKey() && FirstGamepad == INDEX_NONE)
					{
						FirstGamepad = Index;
					}
				}
				ASSERT_THAT(IsTrue(FirstKeyboard != INDEX_NONE));

				// The slot keys are keyboard conveniences: the gamepad reaches the same slots by stepping.
				if (IsHotbarSlotTag(Entry.InputTag))
				{
					continue;
				}
				ASSERT_THAT(IsTrue(FirstGamepad != INDEX_NONE));

				// Keyboard first: Enhanced Input's user settings fill an action's key slots in mapping order.
				ASSERT_THAT(IsTrue(FirstKeyboard < FirstGamepad));
			}
		}

		TEST_METHOD(NoKeyDoesTwoThings)
		{
			const AVestigePlayerCharacter* Defaults = GetFarmerDefaults();
			ASSERT_THAT(IsNotNull(Defaults));
			ASSERT_THAT(IsTrue(Defaults->GetMappingContextsForTests().Num() == 3));

			TMap<FKey, const UInputAction*> ActionByKey;
			for (const FEnhancedActionKeyMapping& Mapping : GetFarmerMappings(*Defaults))
			{
				const UInputAction** Existing = ActionByKey.Find(Mapping.Key);
				ASSERT_THAT(IsTrue(!Existing || *Existing == Mapping.Action));
				ActionByKey.Add(Mapping.Key, Mapping.Action);
			}
		}

		TEST_METHOD(EveryFarmingActionHasAUniqueMappableName)
		{
			const AVestigePlayerCharacter* Defaults = GetFarmerDefaults();
			ASSERT_THAT(IsNotNull(Defaults));
			const UVestigeInputConfig* Config = Defaults->GetInputConfigForTests();
			ASSERT_THAT(IsNotNull(Config));

			// The name is the key a saved remap is stored under, so it is the input tag's name and never changes.
			TSet<FName> Names;
			for (const FVestigeInputAction& Entry : Config->GetNativeInputActions())
			{
				ASSERT_THAT(IsNotNull(Entry.InputAction.Get()));
				const UPlayerMappableKeySettings* Settings = Entry.InputAction->GetPlayerMappableKeySettings();
				ASSERT_THAT(IsNotNull(Settings));
				ASSERT_THAT(IsTrue(Settings->Name == Entry.InputTag.GetTagName()));
				ASSERT_THAT(IsFalse(Settings->DisplayName.IsEmpty()));
				ASSERT_THAT(IsFalse(Names.Contains(Settings->Name)));
				Names.Add(Settings->Name);
			}
		}

#if WITH_EDITOR
		TEST_METHOD(ThePlayersConfigValidates)
		{
			const AVestigePlayerCharacter* Defaults = GetFarmerDefaults();
			ASSERT_THAT(IsNotNull(Defaults));
			const UVestigeInputConfig* Config = Defaults->GetInputConfigForTests();
			ASSERT_THAT(IsNotNull(Config));
			ASSERT_THAT(AreEqual(Config->GetPathName(), FString(TEXT("/Game/Vestige/Input/DA_InputConfig_Player.DA_InputConfig_Player"))));

			FDataValidationContext Context;
			ASSERT_THAT(IsTrue(Config->IsDataValid(Context) != EDataValidationResult::Invalid));
			ASSERT_THAT(AreEqual(Context.GetNumErrors(), 0u));
		}

		TEST_METHOD(AConfigWithADuplicateTagIsInvalid)
		{
			const AVestigePlayerCharacter* Defaults = GetFarmerDefaults();
			ASSERT_THAT(IsNotNull(Defaults));
			const UVestigeInputConfig* Player = Defaults->GetInputConfigForTests();
			ASSERT_THAT(IsNotNull(Player));
			const UInputAction* UseTool = Player->FindNativeInputActionForTag(VestigeGameplayTags::Input_UseTool, true);
			const UInputAction* Interact = Player->FindNativeInputActionForTag(VestigeGameplayTags::Input_Interact, true);
			ASSERT_THAT(IsNotNull(UseTool));
			ASSERT_THAT(IsNotNull(Interact));

			UVestigeInputConfig* Config = NewObject<UVestigeInputConfig>();
			Config->SetNativeInputActionsForTests(
				{{UseTool, VestigeGameplayTags::Input_UseTool}, {Interact, VestigeGameplayTags::Input_UseTool}});
			FDataValidationContext Context;

			ASSERT_THAT(IsTrue(Config->IsDataValid(Context) == EDataValidationResult::Invalid));
			ASSERT_THAT(AreEqual(Context.GetNumErrors(), 1u));
		}
#endif
	};
}

#endif
