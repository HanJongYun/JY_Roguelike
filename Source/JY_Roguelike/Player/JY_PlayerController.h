#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "JY_PlayerController.generated.h"

class UInputMappingContext;
class UJY_ScreenWidget;

/* 입력 매핑 등록, UI 연결, 디버그 명령을 맡는 컨트롤러 */
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

	/* 콘솔 명령(임시): 테스트 화면을 Menu 층에 올리기 (예: JYUIPush) */
	UFUNCTION(Exec)
	void JYUIPush();

protected:
	/* 로컬이면 PlayerState의 ASC를 UI에 연결 */
	void InitializeLocalUI();

	/* 디버그 체력 변경 요청 (음수=데미지, 양수=회복) */
	UFUNCTION(Server, Reliable)
	void Server_DebugApplyHealthDelta(float Delta);

private:
	/* 설정값 — BP_JY_PlayerController의 Class Defaults에서 수정 */

	/* 로컬 플레이어에 등록할 입력 매핑 */
	UPROPERTY(EditAnywhere, Category = "JY|Input")
	TArray<TObjectPtr<UInputMappingContext>> DefaultMappingContexts;

	/* JYUIPush로 띄울 테스트 화면 (임시, WBP_JY_TestMenu) — Soft라 서버는 로드 안 함 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Debug")
	TSoftClassPtr<UJY_ScreenWidget> DebugPushScreenClass;
};
