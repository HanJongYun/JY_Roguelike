#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "JY_TitlePlayerController.generated.h"

/* 타이틀 맵 전용 PC — 타이틀 화면을 띄우고 시작 요청을 서버에 보낸다 */
UCLASS()
class AJY_TitlePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AJY_TitlePlayerController();

protected:
	virtual void BeginPlay() override;

public:
	/* 시작 버튼 → 서버가 허브로 이동 */
	UFUNCTION(Server, Reliable)
	void Server_StartGame();
};
