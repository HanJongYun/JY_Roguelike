#include "UI/Foundation/JY_ScreenWidget.h"
#include "Input/UIActionBindingHandle.h"
#include "UI/JY_UIManagerSubsystem.h"
#include "Engine/LocalPlayer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_ScreenWidget)

/*
 * 스택 맨 위 화면이 켜질 때 CommonUI 라우터가 이 값을 읽어 입력 모드·커서를 바꾼다 (CommonUIActionRouterBase.cpp:1866 ApplyUIInputConfig).
 * 우리가 SetInputMode를 직접 부르지 않는다 — 화면이 "나는 어떤 입력을 원한다"고 선언만 한다.
 * 쿼터뷰라 커서는 항상 보여야 함 → 마지막 인자 bHideCursorDuringViewportCapture = false.
 */
// [OwnClient]
TOptional<FUIInputConfig> UJY_ScreenWidget::GetDesiredInputConfig() const //override
{
	switch (InputMode)
	{
	case EJY_ScreenInputMode::Game:
		return FUIInputConfig(ECommonInputMode::Game, GameMouseCaptureMode, false);
	case EJY_ScreenInputMode::GameAndMenu:
		return FUIInputConfig(ECommonInputMode::All, GameMouseCaptureMode, false);
	case EJY_ScreenInputMode::Menu:
	default:
		/* 메뉴는 게임이 마우스를 잡을 일이 없음 */
		return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture, false);
	}
}

// [OwnClient]
void UJY_ScreenWidget::NativeOnActivated() //override
{
	Super::NativeOnActivated();

	NotifyPauseScreen(true);
}

// [OwnClient]
void UJY_ScreenWidget::NativeOnDeactivated() //override
{
	NotifyPauseScreen(false);

	Super::NativeOnDeactivated();
}

/* 화면은 "정지가 필요하다"만 알린다 — 여러 정지 화면이 겹칠 때 실제 정지·해제 판단은 UIManager가 목록으로 한다 */
// [OwnClient]
void UJY_ScreenWidget::NotifyPauseScreen(bool bActive)
{
	if (bPauseGame == false)
		return;

	UJY_UIManagerSubsystem* UIManager = ULocalPlayer::GetSubsystem<UJY_UIManagerSubsystem>(GetOwningLocalPlayer());
	if (UIManager == nullptr)
		return;

	UIManager->SetPauseScreenActive(this, bActive);
}
