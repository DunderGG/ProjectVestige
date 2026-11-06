// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Core/VestigeSaveRecord.h"

#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "UObject/Class.h"
#include "UObject/ObjectVersion.h"

namespace VestigeSaveRecords
{
	/** Records hold no object references, so a reference that can't be found is never loaded. */
	constexpr bool bLoadMissingObjects = false;

	/** The bytes live in a save file, like a package, so they are written and read as persistent data. */
	constexpr bool bPersistent = true;
}

bool FVestigeSaveRecord::IsFromNewerEngine() const
{
	return PackageVersionUE4 > GPackageFileUEVersion.FileVersionUE4 || PackageVersionUE5 > GPackageFileUEVersion.FileVersionUE5;
}

bool FVestigeSaveRecord::Serialize(FArchive& Ar)
{
	Ar << Version;
	Ar << PackageVersionUE4;
	Ar << PackageVersionUE5;
	CustomVersions.Serialize(Ar);
	Ar << Data;
	return true;
}

FVestigeSaveRecord FVestigeSaveRecord::WriteStruct(UScriptStruct& Type, const void* InData, int32 InVersion)
{
	FVestigeSaveRecord Record;
	Record.Version = InVersion;
	Record.PackageVersionUE4 = GPackageFileUEVersion.FileVersionUE4;
	Record.PackageVersionUE5 = GPackageFileUEVersion.FileVersionUE5;

	// The same archive a save game is written with: names and object paths as strings.
	FMemoryWriter Writer(Record.Data, VestigeSaveRecords::bPersistent);
	FObjectAndNameAsStringProxyArchive Archive(Writer, VestigeSaveRecords::bLoadMissingObjects);

	// SerializeItem takes one pointer for both directions; while saving it only reads the struct.
	Type.SerializeItem(Archive, const_cast<void*>(InData), nullptr);

	// The writer collected the custom versions the struct's properties used, and only those.
	Record.CustomVersions = Writer.GetCustomVersions();
	return Record;
}

bool FVestigeSaveRecord::ReadStruct(UScriptStruct& Type, void* OutData) const
{
	if (Data.IsEmpty() || IsFromNewerEngine())
	{
		return false;
	}

	FMemoryReader Reader(Data, VestigeSaveRecords::bPersistent);
	Reader.SetUEVer(FPackageFileVersion(PackageVersionUE4, static_cast<EUnrealEngineObjectUE5Version>(PackageVersionUE5)));
	Reader.SetCustomVersions(CustomVersions);

	// Made after setting the versions, because the proxy copies the reader's state when it is made.
	FObjectAndNameAsStringProxyArchive Archive(Reader, VestigeSaveRecords::bLoadMissingObjects);
	Type.SerializeItem(Archive, OutData, nullptr);
	return !Archive.IsError() && !Reader.IsError() && Reader.AtEnd();
}
