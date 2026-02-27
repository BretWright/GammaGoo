// Copyright 2026 Bret Wright. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class GammaGooTarget : TargetRules
{
	public GammaGooTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("GammaGoo");
	}
}
