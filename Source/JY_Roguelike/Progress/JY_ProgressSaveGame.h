#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Progress/JY_ProgressTypes.h"
#include "JY_ProgressSaveGame.generated.h"

/* 서버가 디스크에 저장하는 진행 파일 (Saved/SaveGames/<슬롯>.sav) */
UCLASS()
class UJY_ProgressSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/* 저장된 진행 */
	UPROPERTY(SaveGame)
	FJY_PlayerProgress Progress;
};
