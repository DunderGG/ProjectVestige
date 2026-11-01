// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "Content/VestigeContentData.h"
#include "CQTest.h"
#include "Farming/FarmingComponent.h"
#include "Time/VestigeDayCycle.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace VestigeExtensionPointTests
{
	/** A member of one of our classes that mod Blueprints use by its name. */
	struct FExtensionPoint
	{
		const UClass* Class = nullptr;
		FName Name;
	};

	/**
	 * The extension points that Architecture.md section 12 lists. Mod Blueprints bind to them by name, so renaming or hiding
	 * one breaks every mod that uses it. Changing these lists is changing the mod API: update that section too.
	 */
	TEST_CLASS(ExtensionPoints, "Vestige")
	{
		TEST_METHOD(EventsStayAssignableInBlueprints)
		{
			const FExtensionPoint Events[] = {
				{AVestigeDayCycle::StaticClass(), GET_MEMBER_NAME_CHECKED(AVestigeDayCycle, OnNewDayBlueprint)},
				{UFarmingComponent::StaticClass(), GET_MEMBER_NAME_CHECKED(UFarmingComponent, OnCropHarvested)},
				{UFarmingComponent::StaticClass(), GET_MEMBER_NAME_CHECKED(UFarmingComponent, OnToolUsed)},
			};
			for (const FExtensionPoint& Event : Events)
			{
				const FMulticastDelegateProperty* Property = FindFProperty<FMulticastDelegateProperty>(Event.Class, Event.Name);
				if (!Property || !Property->HasAnyPropertyFlags(CPF_BlueprintAssignable))
				{
					AddError(FString::Printf(
						TEXT("%s.%s is no longer an event Blueprints can bind to."), *Event.Class->GetName(), *Event.Name.ToString()));
				}
			}
		}

		TEST_METHOD(FunctionsStayCallableFromBlueprints)
		{
			const FExtensionPoint Functions[] = {
				{AVestigeDayCycle::StaticClass(), GET_FUNCTION_NAME_CHECKED(AVestigeDayCycle, GetDayCycle)},
				{UVestigeContentData::StaticClass(), GET_FUNCTION_NAME_CHECKED(UVestigeContentData, GetContentId)},
			};
			for (const FExtensionPoint& Function : Functions)
			{
				const UFunction* Found = Function.Class->FindFunctionByName(Function.Name);
				if (!Found || !Found->HasAnyFunctionFlags(FUNC_BlueprintCallable))
				{
					AddError(FString::Printf(
						TEXT("%s.%s is no longer a function Blueprints can call."), *Function.Class->GetName(), *Function.Name.ToString()));
				}
			}
		}
	};
}

#endif
