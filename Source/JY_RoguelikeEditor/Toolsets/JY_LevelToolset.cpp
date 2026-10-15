#include "Toolsets/JY_LevelToolset.h"

#include "Editor.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "EditorLevelUtils.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Engine/World.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "NavMesh/RecastNavMesh.h"

FString UJY_LevelToolset::AddStreamingLevel(const FString& LevelPackageName, const FVector& Location)
{
	UWorld* World = GEditor->GetEditorWorldContext().World();

	/* 블루프린트 스트리밍(ULevelStreamingDynamic) — 항상 불림(AlwaysLoaded)이면 방 6개가 한꺼번에 떠서 "방 하나씩 불러오기"(Design_Travel §6)와 어긋남 */
	const FTransform LevelTransform(Location);
	ULevelStreaming* AddedLevel = UEditorLevelUtils::AddLevelToWorld(World, *LevelPackageName, ULevelStreamingDynamic::StaticClass(), LevelTransform);
	if (AddedLevel == nullptr)
		return FString();

	/* 엔진이 추가한 방을 현재 레벨로 바꾼다(EditorLevelUtils.cpp:412) — 이후 배치하는 액터가 방 맵에 들어가지 않게 지역 맵으로 되돌림 */
	World->SetCurrentLevel(World->PersistentLevel);

	return AddedLevel->GetWorldAssetPackageName();
}

TArray<FString> UJY_LevelToolset::GetStreamingLevels()
{
	UWorld* World = GEditor->GetEditorWorldContext().World();

	TArray<FString> Lines;
	for (const ULevelStreaming* StreamingLevel : World->GetStreamingLevels())
	{
		/* 엔진도 목록 안 null을 건너뜀 (LevelStreaming.cpp:287) */
		if (StreamingLevel == nullptr)
			continue;

		const FVector Location = StreamingLevel->LevelTransform.GetLocation();
		const FString ClassName = StreamingLevel->GetClass()->GetName();
		Lines.Add(FString::Printf(TEXT("%s %s %s"), *StreamingLevel->GetWorldAssetPackageName(), *Location.ToString(), *ClassName));
	}

	return Lines;
}

int32 UJY_LevelToolset::BuildNavigation()
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	UNavigationSystemV1* NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	NavigationSystem->Build();

	/* 네비 볼륨이 없는 맵이면 네비 데이터 자체가 안 생김 */
	const ARecastNavMesh* NavMesh = Cast<ARecastNavMesh>(NavigationSystem->GetDefaultNavDataInstance());
	if (NavMesh == nullptr)
		return 0;

	return NavMesh->GetNavMeshTilesCount();
}

bool UJY_LevelToolset::IsNavigationBuilding()
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	UNavigationSystemV1* NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	return NavigationSystem->IsNavigationBuildInProgress();
}

TArray<FString> UJY_LevelToolset::CheckNavPaths(const FVector& Start, const TArray<FVector>& Ends)
{
	UWorld* World = GEditor->GetEditorWorldContext().World();

	TArray<FString> Lines;
	for (const FVector& End : Ends)
	{
		const UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(World, Start, End);
		if (Path == nullptr || Path->IsValid() == false)
		{
			Lines.Add(TEXT("NONE"));
			continue;
		}

		const TCHAR* Result = Path->IsPartial() ? TEXT("PARTIAL") : TEXT("OK");
		Lines.Add(FString::Printf(TEXT("%s %.0f"), Result, Path->GetPathLength()));
	}

	return Lines;
}

FString UJY_LevelToolset::RunPIEConsoleCommand(int32 PIEInstance, const FString& Command)
{
	for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
	{
		if (WorldContext.WorldType != EWorldType::PIE || WorldContext.PIEInstance != PIEInstance)
			continue;

		/* 데디 서버 월드엔 로컬 플레이어가 없다 — 그땐 월드에 직접 (servertravel 등 서버 명령) */
		const TWeakObjectPtr<UWorld> WeakWorld = WorldContext.World();
		auto RunCommand = [WeakWorld, Command](float DeltaSeconds)
		{
			UWorld* TargetWorld = WeakWorld.Get();
			if (TargetWorld == nullptr)
				return false;

			APlayerController* PlayerController = TargetWorld->GetFirstPlayerController();
			if (PlayerController != nullptr)
			{
				PlayerController->ConsoleCommand(Command);
			}
			else
			{
				GEngine->Exec(TargetWorld, *Command);
			}

			/* 한 번만 */
			return false;
		};
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(RunCommand));
		return TEXT("Scheduled");
	}

	return TEXT("No PIE world with that instance");
}
