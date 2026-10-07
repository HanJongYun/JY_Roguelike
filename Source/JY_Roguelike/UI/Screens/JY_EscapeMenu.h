#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/JY_ScreenWidget.h"
#include "JY_EscapeMenu.generated.h"

class UJY_ButtonBase;

/* 정지 메뉴 — HUD 위 Menu 층에 뜨고 뒤로가기 키로 닫힌다 */
UCLASS(Abstract, Blueprintable)
class UJY_EscapeMenu : public UJY_ScreenWidget
{
	GENERATED_BODY()

public:
	UJY_EscapeMenu(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeOnInitialized() override;

private:
	/* 계속하기 클릭 → 메뉴 닫기 */
	void HandleContinueClicked();

	/* 종료 클릭 → 게임 종료 */
	void HandleQuitClicked();

protected:
	/* WBP_JY_EscapeMenu 안의 같은 이름 위젯과 연결 (BindWidget) */

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UJY_ButtonBase> ContinueButton;

	/* 설정 화면은 범위 밖 — 자리만 두고 비활성 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UJY_ButtonBase> SettingsButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UJY_ButtonBase> QuitButton;
};
