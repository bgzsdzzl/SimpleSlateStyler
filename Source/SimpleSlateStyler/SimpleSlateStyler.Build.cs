

using UnrealBuildTool;

public class SimpleSlateStyler : ModuleRules
{
    public SimpleSlateStyler(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "Slate",
            "SlateCore",
            "EditorSubsystem",
            "UnrealEd",
            "Projects"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "AppFramework",
            "EditorStyle",
            "DirectoryWatcher",
            "ToolMenus",
            "WorkspaceMenuStructure"
        });
    }
}