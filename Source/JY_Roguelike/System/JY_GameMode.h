#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameplayTagContainer.h"
#include "Progress/JY_ProgressTypes.h"
#include "JY_GameMode.generated.h"

class AJY_InteractableActor;
class AJY_PlayerState;

/* 진행을 바꿀 때 메타 효과(최대 체력 등)도 다시 걸지 */
enum class EJY_ProgressCommitScope : uint8
{
	/* 영혼석·기록만 바뀜 — 효과는 그대로 */
	ValuesOnly,

	/* 메타 강화 단계가 바뀜 — 효과를 다시 맞춘다 */
	MetaRanksChanged
};

/* 플레이어 클래스 지정 + 서버 진행(세이브·메타 구매) 규칙. 서버에만 있다 */
UCLASS()
class AJY_GameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AJY_GameMode();

public:
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

public:
	/* 메타 강화 1단계 구매 — 판매 대상(제단)·가격·최대 단계·보유 영혼석 검증 */
	void TryBuyMetaUpgrade(APlayerController* Buyer, FGameplayTag UpgradeTag, AJY_InteractableActor* Seller);

	/* 산 메타 강화를 전부 되돌리고 쓴 영혼석을 돌려줌 (판매 대상 검증) */
	void RefundAllMetaUpgrades(APlayerController* Requester, AJY_InteractableActor* Seller);

	/* 영혼석 더하기 (치트·보상) */
	void AddSoulStones(APlayerController* Receiver, int32 Amount);

private:
	/* 요청한 플레이어가 제단 옆에 있나 — 화면은 클라에서만 열려 서버가 모르므로 요청마다 확인 */
	bool IsValidMetaSeller(const APlayerController* Requester, const AJY_InteractableActor* Seller) const;

	/* 진행을 바꾸고 → 복제 → 즉시 저장 (단계가 바뀌었으면 메타 효과도 갱신) */
	void CommitProgress(AJY_PlayerState* PlayerState, const FJY_PlayerProgress& NewProgress, EJY_ProgressCommitScope Scope);

	/* 세이브 파일 → PlayerState */
	void LoadProgress(AJY_PlayerState* PlayerState);

private:
	/* 설정값 — BP_JY_GameMode의 Class Defaults에서 수정 */

	/* 세이브 슬롯 이름 (1인 게임이라 1개) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Progress")
	FString SaveSlotName = TEXT("JY_Progress");
};
