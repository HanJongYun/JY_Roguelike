#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "JY_AssetManager.generated.h"

class UJY_GameData;

/* 프로젝트 에셋 관리자 — 공용 데이터(GameData)를 한 곳에서 꺼내 준다 */
UCLASS(Config = Game)
class UJY_AssetManager : public UAssetManager
{
	GENERATED_BODY()

public:
	/* 엔진이 만든 에셋 관리자 */
	static UJY_AssetManager& Get();

	/* 공용 데이터 (처음 부를 때 로드, 실패하면 nullptr) */
	const UJY_GameData* GetGameData();

protected:
	/* 설정값 — Config/DefaultGame.ini [/Script/JY_Roguelike.JY_AssetManager]에서 지정 */

	/* 공용 데이터 에셋 경로 (DA_JY_GameData) */
	UPROPERTY(Config)
	TSoftObjectPtr<UJY_GameData> GameDataPath;

private:
	UPROPERTY(Transient)
	TObjectPtr<const UJY_GameData> GameData;
};
