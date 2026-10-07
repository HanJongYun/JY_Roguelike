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

	/* 종료 클릭 → 확인 팝업 표시 */
	void HandleQuitClicked();

	/* 종료 확인 팝업 결과 — 확인이면 게임 종료 */
	void HandleQuitConfirmResult(bool bConfirmed);

protected:
	/* WBP_JY_EscapeMenu 안의 같은 이름 위젯과 연결 (BindWidget) */

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UJY_ButtonBase> ContinueButton;

	/* 설정 화면은 범위 밖 — 자리만 두고 비활성 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UJY_ButtonBase> SettingsButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UJY_ButtonBase> QuitButton;

private:
	/* 설정값 — WBP_JY_EscapeMenu의 Class Defaults에서 수정 */

	/* 종료 확인 팝업 제목 (예: "게임 종료") */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Quit")
	FText QuitConfirmTitle;

	/* 종료 확인 팝업 본문 (예: "정말 게임을 종료할까요?") */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Quit")
	FText QuitConfirmBody;
};
