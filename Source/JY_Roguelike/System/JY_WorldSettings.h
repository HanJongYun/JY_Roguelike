#pragma once

#include "CoreMinimal.h"
#include "GameFramework/WorldSettings.h"
#include "JY_WorldSettings.generated.h"

/* 맵마다 다른 설정을 담는 곳 — 에디터 World Settings 패널에서 맵별로 수정 */
UCLASS()
class AJY_WorldSettings : public AWorldSettings
{
	GENERATED_BODY()

public:
	AJY_WorldSettings(const FObjectInitializer& ObjectInitializer);

public:
	bool ShouldShowCombatHUD() const { return bShowCombatHUD; }

	const FText& GetAreaNameText() const { return AreaNameText; }

private:
	/*
	 * 설정값 — 맵을 연 뒤 World Settings 패널 > JY|UI에서 수정 (맵 파일에 저장)
	 * 퍼시스턴트 맵 것만 쓰인다 — 서브레벨의 World Settings는 엔진이 무시한다
	 */

	/* 체력 구슬 등 전투 표시를 보임 (허브 false — 싸우지 않는 곳) */
	UPROPERTY(EditAnywhere, Category = "JY|UI")
	bool bShowCombatHUD = true;

	/* 도착하면 위쪽 배너에 띄울 지역 이름 (예: 영혼의 성소). 비우면 안 띄움 */
	UPROPERTY(EditAnywhere, Category = "JY|UI")
	FText AreaNameText;
};
