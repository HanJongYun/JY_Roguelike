#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "JY_ScreenWidget.generated.h"

/* 화면이 켜져 있는 동안 입력을 어디로 보낼지 */
UENUM(BlueprintType)
enum class EJY_ScreenInputMode : uint8
{
	Game,			// 게임만 (HUD 등)
	Menu,			// 메뉴만 — 이동·공격 막힘
	GameAndMenu		// 둘 다
};

/* 모든 화면 위젯의 공통 부모 — 켜지면 자기 입력 모드를 CommonUI에 알린다 */
UCLASS(Abstract, Blueprintable)
class UJY_ScreenWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

protected:
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;

private:
	/* 정지 화면이면 UIManager에 켜짐·꺼짐을 알림 */
	void NotifyPauseScreen(bool bActive);

protected:
	/* 설정값 — 이 클래스를 부모로 한 WBP의 Class Defaults에서 수정 */

	/* 이 화면이 켜져 있는 동안의 입력 모드 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Input")
	EJY_ScreenInputMode InputMode = EJY_ScreenInputMode::Menu;

	/* 게임에 입력이 갈 때 마우스 잡는 방식 (쿼터뷰는 누르는 동안만 잡음) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Input")
	EMouseCaptureMode GameMouseCaptureMode = EMouseCaptureMode::CaptureDuringMouseDown;

	/* 켜져 있는 동안 게임(월드)을 멈춤 (ESC 메뉴·확인 팝업) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Pause")
	bool bPauseGame = false;
};
