#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/JY_ScreenWidget.h"
#include "JY_HUDLayout.generated.h"

/* 게임 중 항상 떠 있는 HUD 화면 (Game 층) */
UCLASS(Abstract, Blueprintable)
class UJY_HUDLayout : public UJY_ScreenWidget
{
	GENERATED_BODY()

public:
	UJY_HUDLayout(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeOnInitialized() override;

private:
	/* 뒤로가기 키(지금 Tab) → 정지 메뉴를 Menu 층에 올림 */
	void HandleBackAction();

	/* 이 맵 World Settings에 따라 전투 표시(체력 구슬) 보이기·숨기기 */
	void RefreshCombatPanel();

protected:
	/* WBP_JY_HUD 안의 같은 이름 위젯과 연결 (BindWidget) */

	/* 좌하단 체력 구슬 묶음 칸 — 로비에선 숨김 (AJY_WorldSettings::bShowCombatHUD) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> CombatPanel;

private:
	/* 설정값 — WBP_JY_HUD의 Class Defaults에서 수정 */

	/* 뒤로가기 키로 Menu 층에 올릴 정지 메뉴 (WBP_JY_EscapeMenu) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|HUD")
	TSoftClassPtr<UJY_ScreenWidget> EscapeMenuClass;

private:
	/* 정지 메뉴 미리 로드 핸들 */
	TSharedPtr<struct FStreamableHandle> EscapeMenuPreloadHandle;
};
