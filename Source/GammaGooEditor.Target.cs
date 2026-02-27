// Copyright 2026 Bret Wright. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class GammaGooEditorTarget : TargetRules
{
	public GammaGooEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("GammaGoo");
	}
}
