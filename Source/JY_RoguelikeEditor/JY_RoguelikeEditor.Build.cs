using UnrealBuildTool;

public class JY_RoguelikeEditor : ModuleRules
{
	public JY_RoguelikeEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"UnrealEd",
			"ToolsetRegistry",
			"AnimGraph",
			"BlueprintGraph",
			"AnimationModifiers",
			"AnimationBlueprintLibrary",
			"AssetTools"
		});

		PublicIncludePaths.AddRange(new string[] {
			"JY_RoguelikeEditor"
		});
	}
}
