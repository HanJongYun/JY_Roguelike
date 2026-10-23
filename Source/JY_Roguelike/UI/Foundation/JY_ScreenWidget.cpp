#include "UI/Foundation/JY_ScreenWidget.h"
#include "Input/UIActionBindingHandle.h"
#include "UI/JY_UIManagerSubsystem.h"
#include "Engine/LocalPlayer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_ScreenWidget)

/* 화면 대부분은 뒤로가기 키로 닫힌다 — 기본 true, 닫히면 안 되는 HUD만 false */
UJY_ScreenWidget::UJY_ScreenWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsBackHandler = true;
}

/* 화면이 켜질 때 라우터가 이 값으로 입력 모드를 바꾼다 (CommonUIActionRouterBase.cpp:1866) — SetInputMode 직접 호출 안 함 */
/* 쿼터뷰라 커서는 항상 보여야 함 → 마지막 인자 false */
// [OwnClient]
TOptional<FUIInputConfig> UJY_ScreenWidget::GetDesiredInputConfig() const //override
{
	switch (InputMode)
	{
	case EJY_ScreenInputMode::Game:
		return FUIInputConfig(ECommonInputMode::Game, GameMouseCaptureMode, false);
	case EJY_ScreenInputMode::Menu:
		/* 메뉴는 게임이 마우스를 잡을 일이 없음 */
		return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture, false);
	case EJY_ScreenInputMode::Default:
	default:
		/* 빈 값 = 라우터가 부모 화면의 설정을 쓴다 (UIActionRouterTypes.cpp:1185) */
		return TOptional<FUIInputConfig>();
	}
}

// [OwnClient]
void UJY_ScreenWidget::NativeOnActivated() //override
{
	Super::NativeOnActivated();

	NotifyPauseScreen(true);
}

/* 닫힐 때뿐 아니라 같은 층에 다른 화면이 올라와 가려질 때도 불린다 */
// [OwnClient]
void UJY_ScreenWidget::NativeOnDeactivated() //override
{
	NotifyPauseScreen(false);

	Super::NativeOnDeactivated();
}

/* 정지 화면이 겹칠 수 있어 실제 정지·해제 판단은 UI 관리자가 목록으로 한다 */
// [OwnClient]
void UJY_ScreenWidget::NotifyPauseScreen(bool bActive)
{
	if (bPauseGame == false)
		return;

	UJY_UIManagerSubsystem* UIManagerSubsystem = ULocalPlayer::GetSubsystem<UJY_UIManagerSubsystem>(GetOwningLocalPlayer());
	if (UIManagerSubsystem == nullptr)
		return;

	UIManagerSubsystem->SetPauseScreenActive(this, bActive);
}
