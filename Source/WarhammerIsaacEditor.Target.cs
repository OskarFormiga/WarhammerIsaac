using UnrealBuildTool;

public class WarhammerIsaacEditorTarget : TargetRules
{
    public WarhammerIsaacEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("WarhammerIsaac");
    }
}
