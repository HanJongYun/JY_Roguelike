#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "UI/JY_UITypes.h"
#include "UI/JY_LoadingProcessInterface.h"
#include "JY_PlayerController.generated.h"

class UInputMappingContext;
class UJY_CameraOcclusionFadeComponent;

/* 클라 → 서버 요청 창구(구매·카드)와 입력 매핑 등록 */
UCLASS()
class AJY_PlayerController : public APlayerController, public IJY_LoadingProcessInterface
{
	GENERATED_BODY()

public:
	AJY_PlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void OnRep_PlayerState() override;
	virtual void PostProcessInput(const float DeltaTime, const bool bGamePaused) override;

public:
	virtual void AddCheats(bool bForce) override;
	virtual bool ShouldShowLoadingScreen() const override;

public:
	/* 콘솔 명령: 강화 카드 고르기 (예: JYChooseSkillCard 0) */
	UFUNCTION(Exec)
	void JYChooseSkillCard(int32 ChoiceIndex);

	/* 강화 카드 선택 요청 (카드 UI도 이걸 부름) */
	UFUNCTION(Server, Reliable)
	void Server_ChooseSkillCard(int32 ChoiceIndex);

	/* 메타 강화 1단계 구매 요청 — Altar = 제단 화면을 연 제단 */
	UFUNCTION(Server, Reliable)
	void Server_BuyMetaUpgrade(FGameplayTag UpgradeTag, AActor* Altar);

	/* 메타 강화 전액 환불 요청 */
	UFUNCTION(Server, Reliable)
	void Server_RefundAllMetaUpgrades(AActor* Altar);

	/* 정지 화면이 켜짐(true)·모두 꺼짐(false)을 서버에 알려 게임 정지·해제 요청 */
	UFUNCTION(Server, Reliable)
	void Server_SetUIPaused(bool bPaused);

	/* 서버가 정한 일회성 알림(배너·토스트)을 내 화면에 띄움 */
	UFUNCTION(Client, Reliable)
	void Client_ShowNotification(EJY_NotificationType Type, const FText& Message);

private:
	/* 복제 상태(체력·영혼석) 원본을 화면용 ViewModel에 연결 (로컬 PC만) */
	void InitializeLocalUI();

private:
	/* 카메라를 가린 벽 흐리게 (내 화면만) */
	UPROPERTY(VisibleAnywhere, Category = "JY|Camera")
	TObjectPtr<UJY_CameraOcclusionFadeComponent> CameraOcclusionFadeComponent;

private:
	/* 설정값 — BP_JY_PlayerController의 Class Defaults에서 수정 */

	/* 로컬 플레이어에 등록할 입력 매핑 */
	UPROPERTY(EditAnywhere, Category = "JY|Input")
	TArray<TObjectPtr<UInputMappingContext>> DefaultMappingContexts;
};
