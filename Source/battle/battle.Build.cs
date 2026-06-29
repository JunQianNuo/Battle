// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class battle : ModuleRules
{
	public battle(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			"SlateCore",
			"OnlineSubsystem",
			"OnlineSubsystemNull",
			"NavigationSystem"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"battle",
			"battle/Variant_Horror",
			"battle/Variant_Horror/UI",
			"battle/Variant_Shooter",
			"battle/Variant_Shooter/AI",
			"battle/Variant_Shooter/UI",
			"battle/Variant_Shooter/Weapons"
		});
	}
}
