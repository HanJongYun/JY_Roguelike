#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "JY_ScreenWidget.generated.h"

/* 화면이 켜져 있는 동안 입력을 어디로 보낼지 */
UENUM(BlueprintType)
enum class EJY_ScreenInputMode : uint8
{
	/* 바꾸지 않음 — 아래 화면의 입력 모드를 그대로 (화면 안의 하위 페이지·탭) */
	Default,

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

public:
	/* 이 화면을 연 월드 물체 지정 — 켜지기 전에 PushScreen의 InitFunc에서 부른다 */
	void SetSourceActor(AActor* InSourceActor) { SourceActor = InSourceActor; }

	/* 이 화면을 연 월드 물체 (예: 제단). 메뉴 키 등으로 열렸으면 nullptr */
	AActor* GetSourceActor() const { return SourceActor.Get(); }

protected:
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;

private:
	/* 정지 화면이면 켜짐·꺼짐을 UI 관리자 서브시스템에 알림 */
	void NotifyPauseScreen(bool bActive);

protected:
	/* 설정값 — 이 클래스를 부모로 한 WBP의 Class Defaults에서 수정 */

	/* 이 화면이 켜져 있는 동안의 입력 모드 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Input")
	EJY_ScreenInputMode InputMode = EJY_ScreenInputMode::Menu;

	/* 게임에 입력이 갈 때 마우스 잡는 방식 (쿼터뷰는 누르는 동안만 잡음) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Input")
	EMouseCaptureMode GameMouseCaptureMode = EMouseCaptureMode::CaptureDuringMouseDown;

	/* 이 화면이 켜져 있는 동안 게임을 멈출지 (정지 메뉴·확인 팝업) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Pause")
	bool bPauseGame = false;

private:
	/* 이 화면을 연 월드 물체 — 화면이 떠 있는 동안 물체가 사라질 수 있어 약한 참조 */
	TWeakObjectPtr<AActor> SourceActor;
};
