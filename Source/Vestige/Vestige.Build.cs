// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Vestige : ModuleRules
{
	public Vestige(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"GameplayTags"
		});

		// Private: no other module uses our headers, so they don't need to expose these modules' types.
		PrivateDependencyModuleNames.AddRange(new string[] {
			"AssetRegistry",
			"CommonInput",
			"Slate",
			"DeveloperSettings",
			"EngineSettings",
			"GameplayAbilities",
			"GameplayTasks",
			"SlateCore",
			"UMG"
		});

		// CQTest is a developer module, so it is only available to builds with developer tools (never shipping).
		if (Target.bBuildDeveloperTools)
		{
			PrivateDependencyModuleNames.Add("CQTest");
		}

		PublicIncludePaths.AddRange(new string[] {
			"Vestige"
		});
	}
}
