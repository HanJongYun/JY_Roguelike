#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/JY_ScreenWidget.h"
#include "JY_TitleScreen.generated.h"

class UJY_ButtonBase;

/* 타이틀 화면 — 시작·설정·종료 */
UCLASS(Abstract, Blueprintable)
class UJY_TitleScreen : public UJY_ScreenWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;

private:
	/* 시작 클릭 → 서버에 로비 이동 요청 */
	void HandleStartClicked();

	/* 종료 클릭 → 확인 팝업 표시 */
	void HandleQuitClicked();

	/* 종료 확인 팝업 결과 — 확인이면 게임 종료 */
	void HandleQuitConfirmResult(bool bConfirmed);

protected:
	/* WBP_JY_Title 안의 같은 이름 위젯과 연결 (BindWidget) */

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UJY_ButtonBase> StartButton;

	/* 설정 화면은 범위 밖 — 자리만 두고 비활성 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UJY_ButtonBase> SettingsButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UJY_ButtonBase> QuitButton;

private:
	/* 설정값 — WBP_JY_Title의 Class Defaults에서 수정 */

	/* 종료 확인 팝업 제목 (예: "게임 종료") */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Quit")
	FText QuitConfirmTitle;

	/* 종료 확인 팝업 본문 (예: "정말 게임을 종료할까요?") */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Quit")
	FText QuitConfirmBody;
};
