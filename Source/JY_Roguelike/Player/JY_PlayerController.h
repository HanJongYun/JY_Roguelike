#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "JY_PlayerController.generated.h"

class UInputMappingContext;
class AJY_InteractableActor;

/* 클라 → 서버 요청 창구(구매·디버그) + 입력 매핑 등록. 키 입력 처리 자체는 폰(AJY_Character)이 한다 */
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

	/* 메타 강화 1단계 구매 요청 — Seller = 제단 화면을 연 제단. 서버 GameMode가 검증 */
	UFUNCTION(Server, Reliable)
	void Server_BuyMetaUpgrade(FGameplayTag UpgradeTag, AJY_InteractableActor* Seller);

	/* 메타 강화 전액 환불 요청 */
	UFUNCTION(Server, Reliable)
	void Server_RefundAllMetaUpgrades(AJY_InteractableActor* Seller);

	/* 정지 화면이 켜짐(true)·모두 꺼짐(false)을 서버에 알려 게임 정지·해제 요청 */
	UFUNCTION(Server, Reliable)
	void Server_SetUIPaused(bool bPaused);

private:
	/* 복제 상태(체력·영혼석) 원본을 화면용 ViewModel에 연결 — 로컬 PC만. 여러 번 불려도 안전 */
	void InitializeLocalUI();

protected:
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
