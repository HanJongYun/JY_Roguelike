#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "UI/JY_UITypes.h"
#include "JY_PlayerController.generated.h"

class UInputMappingContext;

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

	/* 콘솔 명령: 서버에서 영혼석 더하기 (예: JYAddSoul 100) */
	UFUNCTION(Exec)
	void JYAddSoul(int32 Amount);

	/* 정지 화면(ESC 메뉴 등)이 켜지거나 모두 꺼졌을 때 서버에 월드 정지·해제 요청 */
	UFUNCTION(Server, Reliable)
	void Server_SetUIPaused(bool bPaused);

	/* 메타 강화 1단계 구매 요청 — 서버 GameMode가 검증 */
	UFUNCTION(Server, Reliable)
	void Server_BuyMetaUpgrade(FGameplayTag UpgradeTag);

	/* 메타 강화 전액 환불 요청 */
	UFUNCTION(Server, Reliable)
	void Server_RefundAllMetaUpgrades();

	/* 서버가 정한 알림(배너·토스트)을 내 화면에 */
	UFUNCTION(Client, Reliable)
	void Client_ShowNotification(EJY_NotificationType Type, const FText& Message);

protected:
	/* 로컬이면 PlayerState의 ASC를 UI에 연결 */
	void InitializeLocalUI();

	/* 디버그 체력 변경 요청 (음수=데미지, 양수=회복) */
	UFUNCTION(Server, Reliable)
	void Server_DebugApplyHealthDelta(float Delta);

	/* 디버그 영혼석 추가 요청 */
	UFUNCTION(Server, Reliable)
	void Server_DebugAddSoulStones(int32 Amount);

private:
	/* 설정값 — BP_JY_PlayerController의 Class Defaults에서 수정 */

	/* 로컬 플레이어에 등록할 입력 매핑 */
	UPROPERTY(EditAnywhere, Category = "JY|Input")
	TArray<TObjectPtr<UInputMappingContext>> DefaultMappingContexts;
};
