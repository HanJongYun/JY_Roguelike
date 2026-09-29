#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/JY_ScreenWidget.h"
#include "JY_DebugTestMenu.generated.h"

class UButton;

/* 임시 테스트 화면 — 닫기 버튼만 있음 (L5 ESC 메뉴가 생기면 삭제) */
UCLASS(Abstract)
class UJY_DebugTestMenu : public UJY_ScreenWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;

private:
	UFUNCTION()
	void HandleCloseClicked();

private:
	/* 바인딩 — WBP_JY_TestMenu 안 같은 이름의 Button */

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CloseButton;
};
