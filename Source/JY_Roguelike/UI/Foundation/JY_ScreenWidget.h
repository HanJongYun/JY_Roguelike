#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "JY_ScreenWidget.generated.h"

class UJY_UIManagerSubsystem;

/* 화면이 켜져 있는 동안 입력을 어디로 보낼지 */
UENUM(BlueprintType)
enum class EJY_ScreenInputMode : uint8
{
	/* 게임만 (HUD) */
	Game,

	/* 메뉴만 — 이동·공격 막힘 */
	Menu
};

/* 모든 화면 위젯의 공통 부모 — 켜지면 자기 입력 모드를 CommonUI에 알린다 */
UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class UJY_ScreenWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UJY_ScreenWidget(const FObjectInitializer& ObjectInitializer);

public:
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

protected:
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;

protected:
	/* 이 화면 주인(로컬 플레이어)의 UI 관리자 */
	UJY_UIManagerSubsystem* GetUIManager() const;

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
