// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "VestigeContentCatalog.generated.h"

class UVestigeContentData;
class UWorld;
struct FStreamableHandle;

/**
 * Every content definition (crops, tools, items: every primary asset type whose base class is a UVestigeContentData
 * subclass), loaded once per game with its Visuals bundle in Initialize and kept until the game ends, so systems find
 * content here and nothing else loads a definition. Finding never loads. Its load is a preload handle of its own, so each
 * game instance (each Play in Editor session, each test) owns and frees its load. Systems.md, section 2.
 */
UCLASS()
class UVestigeContentCatalog : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** The catalogue of World's game instance, or nullptr when the world has none (an editor preview world). */
	static UVestigeContentCatalog* Get(const UWorld* World);

	/** Loads every content definition with its Visuals bundle, waiting until they are in memory. */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Releases the load and forgets the definitions. */
	virtual void Deinitialize() override;

	/** The definition with this primary asset ID, or nullptr if the catalogue has none. Never loads. */
	const UVestigeContentData* Find(const FPrimaryAssetId& Id) const;

	/** The definition with this primary asset ID if it is a T, or nullptr. Never loads. */
	template <typename T> const T* Find(const FPrimaryAssetId& Id) const { return Cast<T>(Find(Id)); }

#if WITH_DEV_AUTOMATION_TESTS
	/** For tests only: adds Content (a transient asset) or replaces the one with its ID. Refuses content without a valid ID. */
	bool AddForTests(const UVestigeContentData& Content);

	/** For tests only: the handle of the catalogue's load, or nullptr if it loaded nothing. */
	TSharedPtr<FStreamableHandle> GetLoadHandleForTests() const { return LoadHandle; }
#endif

private:
	/** The primary asset types of content: those with a UVestigeContentData base class, without Blueprint classes, not editor-only. */
	static TArray<FPrimaryAssetType> FindContentTypes();

	/** Every content definition, by primary asset ID. */
	UPROPERTY()
	TMap<FPrimaryAssetId, TObjectPtr<const UVestigeContentData>> Content;

	/** Keeps the definitions' Visuals bundles in memory. Released in Deinitialize. */
	TSharedPtr<FStreamableHandle> LoadHandle;
};
