#include "System/JY_TitleGameMode.h"
#include "System/JY_AssetManager.h"
#include "System/JY_GameData.h"
#include "Player/JY_TitlePlayerController.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_TitleGameMode)

/*
 * 캐릭터 클래스가 없으면 엔진이 스폰을 건너뛴다(GameModeBase.cpp:1291).
 * 진행(세이브) 로드는 하지 않는다 — 허브 도착 때 AJY_GameMode가 한다.
 */
AJY_TitleGameMode::AJY_TitleGameMode()
{
	PlayerControllerClass = AJY_TitlePlayerController::StaticClass();
	DefaultPawnClass = nullptr;

	/* 타이틀 → 허브도 맵이 바뀌는 이동이라 심리스 */
	bUseSeamlessTravel = true;
}

/* 이동 중에 또 불려도 엔진이 두 번째 ServerTravel을 무시한다(World.cpp:9540) */
// [Server]
void AJY_TitleGameMode::StartGame()
{
	const UJY_GameData* GameData = UJY_AssetManager::Get().GetGameData();
	if (GameData == nullptr)
		return;

	if (ensureMsgf(GameData->HubMap.IsNull() == false, TEXT("DA_JY_GameData의 HubMap이 비어 있음")) == false)
		return;

	GetWorld()->ServerTravel(GameData->HubMap.GetLongPackageName());
}
