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

private:
	/* 설정값 — WBP_JY_HUD의 Class Defaults에서 수정 */

	/* 다른 화면이 없을 때 뒤로가기 키로 Menu 층에 올릴 화면 (WBP_JY_EscapeMenu) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|HUD")
	TSoftClassPtr<UJY_ScreenWidget> EscapeMenuClass;
};
