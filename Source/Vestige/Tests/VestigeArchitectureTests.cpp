// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "CQTest.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace VestigeArchitectureTests
{
	/**
	 * The source folders as architecture layers, lowest first, matching Architecture.md section 14. A file may include
	 * headers from its own folder and from folders earlier in this list, never from a later one. A new folder must be
	 * added here, in its layer, before files go in it.
	 */
	const TCHAR* const LayerOrder[] = {TEXT("Core"), TEXT("AbilitySystem"), TEXT("Content"), TEXT("Time"), TEXT("Inventory"),
		TEXT("Farming"), TEXT("Save"), TEXT("Player"), TEXT("UI")};

	/** Tests may include anything, and nothing may include a test. Their files aren't checked. */
	const TCHAR* const TestsFolder = TEXT("Tests");

	/**
	 * Epic's template code, which isn't maintained to the guide (CodingGuidelines.md, section 2.2). It sits outside the
	 * layers as app-level wiring: its files may include any layer (the game mode names the HUD class), so only the layer
	 * that derives from it and the layers above may include it.
	 */
	const TCHAR* const TemplateFolder = TEXT("Template");
	const TCHAR* const LowestLayerThatMayIncludeTemplate = TEXT("Player");

	/** The layer index of Folder, or INDEX_NONE if it isn't a layer. */
	int32 GetLayer(const FString& Folder)
	{
		for (int32 Layer = 0; Layer < UE_ARRAY_COUNT(LayerOrder); ++Layer)
		{
			if (Folder == LayerOrder[Layer])
			{
				return Layer;
			}
		}
		return INDEX_NONE;
	}

	/** Why a file in Folder may not include one of our headers from IncludedFolder, or an empty string if it may. */
	FString GetIncludeProblem(const FString& Folder, const FString& IncludedFolder)
	{
		if (IncludedFolder == TestsFolder)
		{
			return TEXT("Only tests may include test files.");
		}
		if (Folder == TemplateFolder)
		{
			return FString();
		}
		if (IncludedFolder == TemplateFolder && GetLayer(Folder) < GetLayer(LowestLayerThatMayIncludeTemplate))
		{
			return FString::Printf(
				TEXT("Only %s/ and the layers above it may include the template code."), LowestLayerThatMayIncludeTemplate);
		}
		// The template folder isn't a layer, so it never counts as higher.
		if (GetLayer(IncludedFolder) > GetLayer(Folder))
		{
			return FString::Printf(TEXT("%s/ may only include %s/ and the layers below it."), *Folder, *Folder);
		}
		return FString();
	}

	/** The path inside the quotes of an #include "..." line, or an empty string for any other line. */
	FString GetQuotedInclude(const FString& Line)
	{
		const FString Trimmed = Line.TrimStartAndEnd();
		if (!Trimmed.StartsWith(TEXT("#include \"")))
		{
			return FString();
		}
		const int32 Start = Trimmed.Find(TEXT("\"")) + 1;
		const int32 End = Trimmed.Find(TEXT("\""), ESearchCase::CaseSensitive, ESearchDir::FromStart, Start);
		return End > Start ? Trimmed.Mid(Start, End - Start) : FString();
	}

	TEST_CLASS(Architecture, "Vestige")
	{
		TEST_METHOD(IncludeDirection)
		{
			const FString SourceDir = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::GameSourceDir(), TEXT("Vestige")));
			if (!FPaths::DirectoryExists(SourceDir))
			{
				AddInfo(FString::Printf(TEXT("Skipped: there is no source folder at %s."), *SourceDir));
				return;
			}

			TArray<FString> Files;
			IFileManager::Get().FindFilesRecursive(Files, *SourceDir, TEXT("*.*"), true, false);
			TSet<FString> OurHeaderNames;
			for (const FString& File : Files)
			{
				if (FPaths::GetExtension(File) == TEXT("h"))
				{
					OurHeaderNames.Add(FPaths::GetCleanFilename(File));
				}
			}

			int32 FilesChecked = 0;
			for (const FString& File : Files)
			{
				const FString Extension = FPaths::GetExtension(File);
				if (Extension != TEXT("h") && Extension != TEXT("cpp"))
				{
					continue;
				}

				FString RelativePath = File;
				FPaths::MakePathRelativeTo(RelativePath, *(SourceDir + TEXT("/")));
				FString Folder;
				FString Rest;
				if (!RelativePath.Split(TEXT("/"), &Folder, &Rest) || Folder == TestsFolder)
				{
					continue;
				}

				if (Folder != TemplateFolder && GetLayer(Folder) == INDEX_NONE)
				{
					AddError(FString::Printf(TEXT("%s is in %s/, which isn't a layer. Add the folder to LayerOrder in this test and "
												  "to Architecture.md section 14."),
						*RelativePath, *Folder));
					continue;
				}

				++FilesChecked;
				TArray<FString> Lines;
				FFileHelper::LoadFileToStringArray(Lines, *File);
				for (int32 LineIndex = 0; LineIndex < Lines.Num(); ++LineIndex)
				{
					const FString Include = GetQuotedInclude(Lines[LineIndex]);
					FString IncludedFolder;
					FString IncludedFile;
					if (Include.IsEmpty() || Include.EndsWith(TEXT(".generated.h")))
					{
						continue;
					}
					if (!Include.Split(TEXT("/"), &IncludedFolder, &IncludedFile))
					{
						// A bare name is fine for engine headers ("CoreMinimal.h"), not for ours.
						if (OurHeaderNames.Contains(Include))
						{
							AddError(FString::Printf(TEXT("%s(%d) includes \"%s\" without its folder. Include our headers by their "
														  "path from the module root."),
								*RelativePath, LineIndex + 1, *Include));
						}
						continue;
					}

					// Engine headers have folders too ("Engine/World.h"); only paths that exist in our source are ours.
					const bool bOurs = FPaths::FileExists(FPaths::Combine(SourceDir, Include));
					const FString Problem = bOurs ? GetIncludeProblem(Folder, IncludedFolder) : FString();
					if (!Problem.IsEmpty())
					{
						AddError(FString::Printf(
							TEXT("%s(%d) includes %s. %s (Architecture.md section 14)"), *RelativePath, LineIndex + 1, *Include, *Problem));
					}
				}
			}

			ASSERT_THAT(IsTrue(FilesChecked > 0));
		}
	};
}

#endif
