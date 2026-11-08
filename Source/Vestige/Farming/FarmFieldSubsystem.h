// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Core/VestigeSaveable.h"
#include "Farming/FarmFieldSaveData.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FarmFieldSubsystem.generated.h"

class UFarmCropData;
class UFarmField;
class UVestigeClockSubsystem;
class UVestigeContentCatalog;
class UVestigeSaveRegistry;

/**
 * The farm data of every map: one UFarmField per field, by its ID, kept for the whole run of the game. A game-instance
 * subsystem, so a field outlives its map: every field grows on the clock's new day, loaded or not, and a map opened again
 * finds its field as it was. Levels only show fields; AFarmGrid asks for its field here when it begins play.
 * Saves every field under Base.Fields through UVestigeSaveRegistry, finding the crops a save names in the content catalogue
 * when it restores.
 */
UCLASS()
class UFarmFieldSubsystem : public UGameInstanceSubsystem, public IVestigeSaveable
{
	GENERATED_BODY()

public:
	/** The fields of World's game instance, or nullptr in a world without one (editor and preview worlds). */
	static UFarmFieldSubsystem* Get(const UWorld* World);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * The field FieldId, made as Size grass tiles the first time it is asked for. A field of another size, such as one
	 * from a save made before its grid changed, is resized to Size with a warning, keeping the tiles that fit.
	 */
	UFarmField& FindOrAddField(FName FieldId, const FIntPoint& Size);

	/** The field FieldId, or nullptr if no grid or save has made it yet. */
	UFarmField* FindField(FName FieldId) const;

	/** Every field's ID, size and tiles, for saving. */
	TArray<FFarmFieldSaveData> GetSaveData() const;

	/**
	 * Restores every field a save holds, with each tile's crop looked up in Crops by ContentId. A field that already exists
	 * keeps its size, so its grid still matches it, and saved tiles outside it are skipped; a field that doesn't exist yet
	 * is made at its saved size. Returns how many entries it couldn't fully restore.
	 */
	int32 RestoreFromSaveData(const TArray<FFarmFieldSaveData>& SavedFields, const TMap<FName, const UFarmCropData*>& Crops);

	/**
	 * Drops every field, for a new game or before a save is restored. Grids still showing an old field stop following the
	 * farm until their level is replaced, so this happens before a map is opened or begins play.
	 */
	virtual void StartNewGame() override;

	virtual FName GetSaveKey() const override;
	virtual int32 GetSaveVersion() const override;
	virtual FVestigeSaveRecord WriteSaveRecord() const override;

	/** Replaces every field with the saved ones, with the crops their tiles name found in the content catalogue. */
	virtual bool ReadSaveRecord(const FVestigeSaveRecord& Record, int32& OutLeftOutCount) override;

private:
	UFarmField& AddField(FName FieldId, const FIntPoint& Size);

	/** Starts a new day on every field. Bound to UVestigeClockSubsystem::OnNewDay. */
	void AdvanceFields(int32 NewDay);

	UPROPERTY()
	TMap<FName, TObjectPtr<UFarmField>> Fields;

	UPROPERTY()
	TObjectPtr<UVestigeClockSubsystem> Clock;

	UPROPERTY()
	TObjectPtr<UVestigeContentCatalog> Catalog;

	UPROPERTY()
	TObjectPtr<UVestigeSaveRegistry> SaveRegistry;
};
