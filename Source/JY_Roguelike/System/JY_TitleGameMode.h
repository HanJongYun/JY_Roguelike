#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "JY_TitleGameMode.generated.h"

/* 타이틀 맵 규칙 — 캐릭터 없음, 시작하면 허브로 이동. 서버에만 있다 */
UCLASS()
class AJY_TitleGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AJY_TitleGameMode();

public:
	/* 허브로 서버 전체 이동 (GameData HubMap) */
	void StartGame();
};
