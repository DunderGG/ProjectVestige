// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Serialization/CustomVersion.h"
#include "VestigeSaveRecord.generated.h"

/**
 * One owner's part of the game's saved state (Systems.md, section 1): its save struct as bytes, the version of that
 * struct's layout, and the engine serialization versions the bytes were written with. An owner makes one with Write and
 * reads it back with Read; UVestigeSaveRegistry keeps one per key and UVestigeSaveGame stores them. A record whose owner
 * isn't there (a mod that isn't installed) is kept as it is, so its bytes can be older than the build that writes them
 * out again, which is why the engine versions travel with them.
 * Serialized natively (Serialize), so the bytes are copied in one block rather than one tagged byte at a time.
 */
USTRUCT()
struct FVestigeSaveRecord
{
	GENERATED_BODY()

public:
	/** A record of Data, written as its struct's current layout version (TStruct::Version). */
	template <typename TStruct> static FVestigeSaveRecord Write(const TStruct& Data)
	{
		return WriteStruct(*TStruct::StaticStruct(), &Data, TStruct::Version);
	}

	/**
	 * Reads the bytes into OutData, whose fields keep their values where the record has none. Returns false if the bytes
	 * can't be read as that struct: they are damaged, or a newer engine wrote them (IsFromNewerEngine).
	 */
	template <typename TStruct> bool Read(TStruct& OutData) const { return ReadStruct(*TStruct::StaticStruct(), &OutData); }

	/** The layout version of the struct in the bytes, as its owner numbers them. */
	int32 GetVersion() const { return Version; }

	/** True if a newer engine than this build's wrote the bytes, so this build can't read them. */
	bool IsFromNewerEngine() const;

	/** Writes or reads the record as one block, for the save file and the registry. */
	bool Serialize(FArchive& Ar);

private:
	static FVestigeSaveRecord WriteStruct(UScriptStruct& Type, const void* Data, int32 InVersion);
	bool ReadStruct(UScriptStruct& Type, void* OutData) const;

	int32 Version = 0;

	/** The engine serialization versions (FPackageFileVersion and the custom versions) the bytes were written with. */
	int32 PackageVersionUE4 = 0;
	int32 PackageVersionUE5 = 0;
	FCustomVersionContainer CustomVersions;

	/** The struct as tagged properties, which tolerate fields added or removed since it was written. */
	TArray<uint8> Data;
};

template <> struct TStructOpsTypeTraits<FVestigeSaveRecord> : public TStructOpsTypeTraitsBase2<FVestigeSaveRecord>
{
	enum
	{
		WithSerializer = true
	};
};
