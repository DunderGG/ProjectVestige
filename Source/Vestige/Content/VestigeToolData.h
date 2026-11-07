// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Content/VestigeContentData.h"
#include "Templates/SubclassOf.h"
#include "VestigeToolData.generated.h"

class UGameplayAbility;
class UStaticMesh;

/**
 * The definition of one tool: the ability that is its behaviour, how it looks in the hand and what a use costs,
 * plus the ID and name every content asset has. One asset per tool, found through the Asset Manager by its ContentId.
 * The farming component grants the tool's ability while the tool is in hand, with this asset as the ability's source.
 */
UCLASS()
class UVestigeToolData : public UVestigeContentData
{
	GENERATED_BODY()

public:
	/** The primary asset type tools are registered under in Config/DefaultGame.ini. */
	static const FPrimaryAssetType AssetType;

	/** The ability that does what the tool does, or nullptr if none is set. */
	TSubclassOf<UGameplayAbility> GetAbility() const { return Ability; }

	float GetStaminaCost() const { return StaminaCost; }
	float GetUseSeconds() const { return UseSeconds; }

	/** The mesh shown in the farmer's hand, or nullptr if the tool has none or it isn't loaded. */
	UStaticMesh* GetHeldMesh() const;

#if WITH_EDITOR
	/** Reports a missing or abstract Ability. */
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

#if WITH_DEV_AUTOMATION_TESTS
	/** For tests only. Tools get their ability in the editor. */
	void SetAbilityForTests(TSubclassOf<UGameplayAbility> InAbility) { Ability = InAbility; }
#endif

protected:
	virtual FPrimaryAssetType GetContentType() const override { return AssetType; }

private:
	/**
	 * What the tool does: a tool ability (a subclass of Vestige Gameplay Ability Use Tool, such as Farm Gameplay Ability
	 * Till), granted while the tool is in hand. A new tool behaviour is a new ability class.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Tool", meta = (AllowPrivateAccess = "true", AllowAbstract = "false"))
	TSubclassOf<UGameplayAbility> Ability;

	/** Stamina one use takes. A use that changes nothing on the farm costs nothing. */
	UPROPERTY(EditDefaultsOnly, Category = "Tool", meta = (AllowPrivateAccess = "true", ClampMin = 0))
	float StaminaCost = 0.f;

	/** How long one use takes, in seconds, before the next can start. Longer while the player is out of stamina. */
	UPROPERTY(EditDefaultsOnly, Category = "Tool", meta = (AllowPrivateAccess = "true", ClampMin = 0, Units = "s"))
	float UseSeconds = 0.4f;

	/** Mesh held in the right hand. The farmer Blueprint's HeldTool component sets its grip offset and rotation. */
	UPROPERTY(EditDefaultsOnly, Category = "Visuals", meta = (AllowPrivateAccess = "true", AssetBundles = "Visuals"))
	TSoftObjectPtr<UStaticMesh> HeldMesh;
};
