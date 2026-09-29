#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/JY_ScreenWidget.h"
#include "JY_HUDLayout.generated.h"

/* 게임 중 항상 떠 있는 HUD 화면 — Game 층 맨 아래에 1개 */
UCLASS(Abstract, Blueprintable)
class UJY_HUDLayout : public UJY_ScreenWidget
{
	GENERATED_BODY()

public:
	UJY_HUDLayout(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeOnInitialized() override;

private:
	/* 뒤로가기 키(Tab) → ESC 메뉴를 Menu 층에 올림 */
	void HandleEscapeAction();

	/* 이 맵 GameMode 설정에 따라 전투 표시(체력바) 보이기 — GameState가 아직 없으면 도착할 때 */
	void RefreshCombatPanel(class AGameStateBase* GameState);

protected:
	/* WBP_JY_HUD 안의 같은 이름 위젯과 연결 (BindWidget) */

	/* 체력바 묶음 — 허브에선 숨김 (AJY_GameMode::bShowCombatHUD) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> CombatPanel;

private:
	/* 설정값 — WBP_JY_HUD의 Class Defaults에서 수정 */

	/* 다른 화면이 없을 때 뒤로가기 키로 Menu 층에 올릴 화면 (WBP_JY_EscapeMenu) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|HUD")
	TSoftClassPtr<UJY_ScreenWidget> EscapeMenuClass;

private:
	TSharedPtr<struct FStreamableHandle> EscapeMenuPreloadHandle;
};
