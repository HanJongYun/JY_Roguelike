// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class JY_Roguelike : ModuleRules
{
	public JY_Roguelike(ReadOnlyTargetRules Target) : base(Target)
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
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"ModelViewViewModel",
			"FieldNotification",
			"CommonUI",
			"CommonInput",
			"Niagara",
			"AnimGraphRuntime",
			"AnimationLocomotionLibraryRuntime",
			// Mass 웨이브 적 — 5.8에서 FMassFragment 등 기본 타입은 MassCore. MassReplication은 쓰지 않는다(복제는 직접 만든 Actor)
			"MassEntity",
			"MassCore",
			"MassCommon",
			"MassSpawner",
			"MassRepresentation",
			"MassMovement",
			"MassNavigation",
			"MassSignals",
			"MassLOD",
			"MassSimulation",
			"NavigationSystem",
			"NetCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"Chaos",
			"PhysicsCore",
			"GeometryCollectionEngine",
			"FieldSystemEngine"
		});

		PublicIncludePaths.AddRange(new string[] {
			"JY_Roguelike"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
