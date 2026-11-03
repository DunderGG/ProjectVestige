// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "VestigeContentData.generated.h"

/**
 * Base class for content definitions (crops, tools) found through the Asset Manager.
 * Owns what all content shares: the stable ContentId that builds the primary asset ID, and the display name.
 */
UCLASS(Abstract)
class UVestigeContentData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Asset bundle holding a content asset's meshes. Loading content with it loads them too. */
	static const FName VisualsBundle;

	/**
	 * Loads the content asset with this ID and its Visuals bundle, waiting until both are in memory.
	 * Only for play start or loading screens. Returns nullptr, with a warning, if no such asset exists.
	 */
	static const UVestigeContentData* LoadWithVisuals(const FPrimaryAssetId& ContentAssetId);

#if WITH_EDITOR
	/** Reports a missing or badly formed ContentId, so a mistake shows up when the asset is saved or validated. */
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;

	/**
	 * For data validation of a property that names other content: says what is wrong with Id as a reference to content of
	 * ExpectedType (unset, another type, or no asset has it), as the end of a sentence that starts with the property's name.
	 * Empty when Id names a known asset of that type.
	 */
	static TOptional<FText> FindContentReferenceError(const FPrimaryAssetId& Id, const FPrimaryAssetType& ExpectedType);
#endif

	/** Built from the content type and ContentId, so renaming or moving the asset keeps the ID. Invalid while ContentId is empty. */
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/**
	 * True if Id has the content ID format: a namespace and a name joined by exactly one dot (Base.Turnip, MyMod.BlueCorn),
	 * each starting with a capital letter and made only of ASCII letters and digits.
	 */
	static bool IsWellFormedContentId(FName Id);

	/**
	 * Extension point: the ID that saves and mods know this content by (Base.Turnip), so a Blueprint can tell which crop or
	 * tool an event names.
	 */
	UFUNCTION(BlueprintPure, Category = "Content")
	FName GetContentId() const { return ContentId; }

	const FText& GetDisplayName() const { return DisplayName; }

#if WITH_DEV_AUTOMATION_TESTS
	/** For tests only. Content assets get their ID in the editor, and it never changes after that. */
	void SetContentIdForTests(FName InContentId) { ContentId = InContentId; }
#endif

protected:
	/** The primary asset type of this kind of content, as registered in Config/DefaultGame.ini. */
	virtual FPrimaryAssetType GetContentType() const PURE_VIRTUAL(UVestigeContentData::GetContentType, return FPrimaryAssetType(););

private:
	/** Stable ID of the form Namespace.Name (Base.Turnip). Saves and lookups use it; never change it once shipped. */
	UPROPERTY(EditDefaultsOnly, Category = "Content", meta = (AllowPrivateAccess = "true"))
	FName ContentId;

	/** Name shown to the player. */
	UPROPERTY(EditDefaultsOnly, Category = "Content", meta = (AllowPrivateAccess = "true"))
	FText DisplayName;
};
