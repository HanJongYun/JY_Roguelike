#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameplayTagContainer.h"
#include "Progress/JY_ProgressTypes.h"
#include "JY_GameMode.generated.h"

class AJY_PlayerState;

/* 플레이어 클래스 지정 + 서버 진행(세이브·메타 구매) 규칙 */
UCLASS()
class AJY_GameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AJY_GameMode();

public:
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

public:
	/* 메타 강화 1단계 구매 — 가격·최대 단계·보유 영혼석 검증 */
	void TryBuyMetaUpgrade(APlayerController* Buyer, FGameplayTag UpgradeTag);

	/* 산 메타 강화를 전부 되돌리고 쓴 영혼석을 돌려줌 */
	void RefundAllMetaUpgrades(APlayerController* Requester);

	/* 영혼석 더하기 (치트·보상) */
	void AddSoulStones(APlayerController* Receiver, int32 Amount);

	/* 이 맵에서 체력바 등 전투 표시를 보이나 — 클라 HUD가 GameMode 기본값으로 읽는다 */
	bool ShouldShowCombatHUD() const { return bShowCombatHUD; }

private:
	/* 진행을 바꾸고 → 복제 → 즉시 저장 (단계가 바뀌었으면 메타 효과도 갱신) */
	void CommitProgress(AJY_PlayerState* PlayerState, const FJY_PlayerProgress& NewProgress, bool bMetaRanksChanged);

	/* 세이브 파일 → PlayerState */
	void LoadProgress(AJY_PlayerState* PlayerState);

private:
	/* 설정값 — BP_JY_GameMode의 Class Defaults에서 수정 */

	/* 세이브 슬롯 이름 (솔로 게임이라 1개) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Progress")
	FString SaveSlotName = TEXT("JY_Progress");

	/* 이 맵에서 메타 강화 구매·환불 허용 (허브 GameMode만 true — 던전 GameMode는 false) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Progress")
	bool bAllowMetaUpgrades = false;

	/* 이 맵에서 체력바를 보임 (허브 false — 싸우지 않는 곳, 던전 true) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|UI")
	bool bShowCombatHUD = true;

	/* 접속 때 띄울 배너 (예: 허브). 비우면 안 띄움 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|UI")
	FText WelcomeBannerText;
};
