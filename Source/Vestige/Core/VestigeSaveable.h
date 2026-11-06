// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Core/VestigeSaveRecord.h"
#include "UObject/Interface.h"
#include "VestigeSaveable.generated.h"

UINTERFACE(meta = (CannotImplementInterfaceInBlueprint))
class UVestigeSaveable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Anything whose state is saved (Systems.md, section 1): the clock, the fields, the inventory, the stamina. It registers
 * with UVestigeSaveRegistry, which hands it its part of the save when it registers, asks it for its part when the game is
 * saved, and keeps a world owner's part when its map closes. Its save struct lives next to it, and only it reads its part.
 */
class IVestigeSaveable
{
	GENERATED_BODY()

public:
	/**
	 * The key its part is kept under, namespaced like a content ID (Base.Clock). Never changes once a build has shipped,
	 * and no two owners share one.
	 */
	virtual FName GetSaveKey() const = 0;

	/** The layout version of its save struct. Raised when the struct changes in a way that would load older parts wrong. */
	virtual int32 GetSaveVersion() const = 0;

	/** Its current state as a record of version GetSaveVersion. */
	virtual FVestigeSaveRecord WriteSaveRecord() const = 0;

	/**
	 * Restores its state from Record, which has GetSaveVersion's layout, without replaying actions or broadcasting
	 * gameplay events like OnNewDay, and adds to OutLeftOutCount the saved entries it left out (content that no longer
	 * exists). Returns false, changing nothing, if the record can't be read as its save struct.
	 */
	virtual bool ReadSaveRecord(const FVestigeSaveRecord& Record, int32& OutLeftOutCount) = 0;

	/** Takes the state it has when no save holds its part, as at the start of a new game. */
	virtual void StartNewGame() = 0;
};
