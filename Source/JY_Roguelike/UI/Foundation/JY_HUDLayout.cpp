#include "UI/Foundation/JY_HUDLayout.h"
#include "UI/JY_UIManagerSubsystem.h"
#include "System/JY_GameplayTags.h"
#include "Engine/LocalPlayer.h"
#include "ICommonInputModule.h"
#include "CommonInputSettings.h"
#include "Input/CommonUIInputTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_HUDLayout)

/*
 * HUD도 스택에 올라가는 화면 하나다(Lyra ULyraHUDLayout도 ULyraActivatableWidget 자식).
 * 켜져 있는 동안 입력은 게임으로 — 이동·공격이 막히지 않는다.
 */
UJY_HUDLayout::UJY_HUDLayout(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InputMode = EJY_ScreenInputMode::Game;
}

/*
 * bIsBackHandler를 쓰지 않는 이유: 엔진이 등록하는 뒤로가기 바인딩은 Menu 모드 전용(CommonUIInputTypes.h:62 기본값)이고,
 * 라우터는 현재 입력 모드와 바인딩 모드가 같을 때만 실행한다(UIActionRouterTypes.cpp:852) → Game 모드인 HUD에선 무시된다.
 * 그래서 같은 뒤로가기 액션을 Game 모드로 직접 등록한다.
 * 메뉴·팝업이 열려 있으면 입력 모드가 Menu라 이 바인딩은 무시되고, 그 화면의 뒤로가기(닫기)가 받는다.
 */
// [OwnClient]
void UJY_HUDLayout::NativeOnInitialized() //override
{
	Super::NativeOnInitialized();

	const UInputAction* BackAction = ICommonInputModule::GetSettings().GetEnhancedInputBackAction();
	if (BackAction == nullptr)
		return;

	FBindUIActionArgs BindArgs(BackAction, false, FSimpleDelegate::CreateUObject(this, &UJY_HUDLayout::HandleEscapeAction));
	BindArgs.InputMode = ECommonInputMode::Game;
	RegisterUIActionBinding(BindArgs);
}

// [OwnClient]
void UJY_HUDLayout::HandleEscapeAction()
{
	UJY_UIManagerSubsystem* UIManager = ULocalPlayer::GetSubsystem<UJY_UIManagerSubsystem>(GetOwningLocalPlayer());
	if (UIManager == nullptr)
		return;

	/* 한 번 로드되면 메모리에 남아 두 번째부터는 즉시 반환 */
	const TSubclassOf<UJY_ScreenWidget> MenuClass = EscapeMenuClass.LoadSynchronous();
	if (MenuClass == nullptr)
		return;

	UIManager->PushScreen(JYGameplayTags::UI_Layer_Menu, MenuClass);
}

