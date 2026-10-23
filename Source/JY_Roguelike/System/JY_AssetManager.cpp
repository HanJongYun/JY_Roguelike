#include "System/JY_AssetManager.h"
#include "System/JY_GameData.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_AssetManager)

/* 캐스트 실패 = DefaultEngine.ini AssetManagerClassName 누락 — 원인 모를 오류 대신 여기서 멈춘다 */
UJY_AssetManager& UJY_AssetManager::Get()
{
	check(GEngine != nullptr);

	UJY_AssetManager* AssetManager = Cast<UJY_AssetManager>(GEngine->AssetManager);
	check(AssetManager != nullptr);

	return *AssetManager;
}

/* 작은 에셋을 1회만 로드하므로 동기 로드. 안의 UI 클래스는 Soft라 데디 서버엔 안 올라간다 */
const UJY_GameData* UJY_AssetManager::GetGameData()
{
	if (GameData != nullptr)
		return GameData;

	if (GameDataPath.IsNull() == true)
		return nullptr;

	GameData = GameDataPath.LoadSynchronous();
	if (GameData == nullptr)
		return nullptr;

	return GameData;
}
