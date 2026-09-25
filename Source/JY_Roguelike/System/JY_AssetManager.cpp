#include "System/JY_AssetManager.h"
#include "System/JY_GameData.h"
#include "System/JY_LogChannels.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_AssetManager)

/*
 * 엔진 시작 때 GEngine이 ini에 적힌 클래스로 딱 하나 만든다 (Lyra ULyraAssetManager::Get 답습).
 * 캐스트가 실패하면 ini 설정이 빠진 것 — 계속 돌리면 원인 모를 오류가 나므로 여기서 멈춘다.
 */
UJY_AssetManager& UJY_AssetManager::Get()
{
	check(GEngine != nullptr);

	UJY_AssetManager* AssetManager = Cast<UJY_AssetManager>(GEngine->AssetManager);
	if (AssetManager == nullptr)
	{
		UE_LOG(LogJY, Fatal, TEXT("JYUI AssetManagerClassName이 JY_AssetManager가 아님 — DefaultEngine.ini 확인"));
	}

	return *AssetManager;
}

/*
 * 처음 부를 때 한 번만 동기 로드하고 계속 들고 있는다 — 에셋이 작고(클래스 참조만) 로드 1회뿐이라 비동기 불필요.
 * 안의 UI 클래스는 Soft 참조라 여기서 같이 로드되지 않는다 → 데디 서버는 UI 클래스를 메모리에 올리지 않음.
 */
const UJY_GameData* UJY_AssetManager::GetGameData()
{
	if (GameData != nullptr)
		return GameData;

	if (GameDataPath.IsNull() == true)
	{
		UE_LOG(LogJY, Error, TEXT("JYUI GameDataPath 비어 있음 — DefaultGame.ini [/Script/JY_Roguelike.JY_AssetManager] 확인"));
		return nullptr;
	}

	GameData = GameDataPath.LoadSynchronous();
	if (GameData == nullptr)
	{
		UE_LOG(LogJY, Error, TEXT("JYUI GameData 로드 실패 %s"), *GameDataPath.ToString());
		return nullptr;
	}

	UE_LOG(LogJY, Log, TEXT("JYUI GameData loaded %s"), *GetNameSafe(GameData));
	return GameData;
}
