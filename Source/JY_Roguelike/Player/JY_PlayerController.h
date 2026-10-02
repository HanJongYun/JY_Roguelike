#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "JY_PlayerController.generated.h"

class UInputMappingContext;
class AJY_InteractableActor;
class UJY_HUDLayout;

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

private:
	/* [UI 과외 임시] HUD에 내 PlayerState의 ASC를 연결 — PlayerState가 아직 없으면 아무것도 안 함 */
	void BindHUDToPlayerState();

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

	/* [UI 과외 임시] 시작할 때 화면에 띄울 HUD (WBP_JY_HUD) — 2장에서 UI 틀로 이동 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|UI")
	TSubclassOf<UJY_HUDLayout> HUDClass;

private:
	/* [UI 과외 임시] BeginPlay에서 만든 HUD — PlayerState가 늦게 오면 OnRep_PlayerState에서 다시 연결 */
	UPROPERTY(Transient)
	TObjectPtr<UJY_HUDLayout> HUDWidget;
};
