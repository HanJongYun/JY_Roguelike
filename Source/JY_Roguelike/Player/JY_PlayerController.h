#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "JY_PlayerController.generated.h"

class UInputMappingContext;
class UJY_ScreenWidget;

/* 입력 매핑 등록, 커서 표시, 디버그 체력 명령을 맡는 컨트롤러 */
UCLASS()
class AJY_PlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AJY_PlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void OnRep_PlayerState() override;

public:
	/* 콘솔 명령: 서버에서 내 체력 깎기 (예: JYDamage 10) */
	UFUNCTION(Exec)
	void JYDamage(float Amount);

	/* 콘솔 명령: 서버에서 내 체력 채우기 (예: JYHeal 10) */
	UFUNCTION(Exec)
	void JYHeal(float Amount);

protected:
	/* 로컬이면 HUD를 띄우고 PlayerState의 ASC를 UI에 연결 */
	void InitializeLocalUI();

	/* 디버그 체력 변경 요청 (음수=데미지, 양수=회복) */
	UFUNCTION(Server, Reliable)
	void Server_DebugApplyHealthDelta(float Delta);

private:
	/* 설정값 — BP_JY_PlayerController의 Class Defaults에서 수정 */

	/* 로컬 플레이어에 등록할 입력 매핑 */
	UPROPERTY(EditAnywhere, Category = "JY|Input")
	TArray<TObjectPtr<UInputMappingContext>> DefaultMappingContexts;

	/* 게임 중 띄울 HUD 위젯 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|UI")
	TSubclassOf<UJY_ScreenWidget> HUDWidgetClass;
};
