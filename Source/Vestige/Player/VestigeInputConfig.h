// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "VestigeInputConfig.generated.h"

class UInputAction;

/** One input action and the input tag code binds it by. */
USTRUCT()
struct FVestigeInputAction
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<const UInputAction> InputAction = nullptr;

	/** A tag under Vestige.Input, one of VestigeGameplayTags' input tags. */
	UPROPERTY(EditDefaultsOnly, Category = "Input", meta = (Categories = "Vestige.Input"))
	FGameplayTag InputTag;
};

/**
 * Which input action each input tag means (Systems.md, section 5), so code binds by tag and never names an input asset.
 * DA_InputConfig_Player (Content/Vestige/Input) is the player's, named by BP_PlayerCharacter, which hard-references it: it
 * is tiny and needed the moment the pawn is possessed. The keys live in the mapping contexts, not here.
 */
UCLASS(Const)
class UVestigeInputConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	/**
	 * The action for Tag, or nullptr when the config has none. bLogNotFound logs an error then, for callers that must find
	 * it (binding); the key hints pass false.
	 */
	const UInputAction* FindNativeInputActionForTag(const FGameplayTag& Tag, bool bLogNotFound) const;

	/** The entries in their order, which is also the order of the key hints. */
	const TArray<FVestigeInputAction>& GetNativeInputActions() const { return NativeInputActions; }

#if WITH_EDITOR
	/**
	 * Reports an entry without an action, a tag that isn't under Vestige.Input, a tag or an action listed twice, an action
	 * whose value isn't Boolean, and an action with player-mappable settings but no mapping name.
	 */
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

#if WITH_DEV_AUTOMATION_TESTS
	/** For tests only. The config gets its entries in the editor. */
	void SetNativeInputActionsForTests(TArray<FVestigeInputAction> InNativeInputActions);
#endif

private:
	/** The actions code binds by tag, each to a call on the component that owns it. */
	UPROPERTY(EditDefaultsOnly, Category = "Input", meta = (AllowPrivateAccess = "true", TitleProperty = "InputTag"))
	TArray<FVestigeInputAction> NativeInputActions;
};
