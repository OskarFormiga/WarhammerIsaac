using UnrealBuildTool;

public class WarhammerIsaacTarget : TargetRules
{
    public WarhammerIsaacTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("WarhammerIsaac");
    }
}
