#include "UI/Foundation/JY_ScreenWidget.h"
#include "Input/UIActionBindingHandle.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_ScreenWidget)

/* 화면 대부분은 뒤로가기 키로 닫힌다 — 기본 true, 닫히면 안 되는 HUD만 false */
UJY_ScreenWidget::UJY_ScreenWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsBackHandler = true;
}

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
	case EJY_ScreenInputMode::Menu:
		/* 메뉴는 게임이 마우스를 잡을 일이 없음 */
		return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture, false);
	case EJY_ScreenInputMode::Default:
	default:
		/* 빈 값 = 라우터가 바로 위 부모 화면의 설정을 쓴다 (UIActionRouterTypes.cpp:1185 FindDesiredInputConfig, Lyra ELyraWidgetInputMode::Default와 같음) */
		return TOptional<FUIInputConfig>();
	}
}
