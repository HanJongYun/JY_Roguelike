#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "JY_GameMode.generated.h"

/* 플레이어가 쓸 PlayerState·컨트롤러·캐릭터 클래스를 정하는 게임모드 */
UCLASS()
class AJY_GameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AJY_GameMode();
};
