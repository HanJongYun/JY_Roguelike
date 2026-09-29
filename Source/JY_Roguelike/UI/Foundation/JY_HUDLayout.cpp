#include "UI/Foundation/JY_HUDLayout.h"
#include "UI/JY_UIManagerSubsystem.h"
#include "System/JY_GameplayTags.h"
#include "Engine/LocalPlayer.h"
#include "ICommonInputModule.h"
#include "CommonInputSettings.h"
#include "Input/CommonUIInputTypes.h"
#include "Engine/AssetManager.h"
#include "System/JY_GameMode.h"
#include "GameFramework/GameStateBase.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_HUDLayout)

/*
 * HUD도 스택에 올라가는 화면 하나다(Lyra ULyraHUDLayout도 ULyraActivatableWidget 자식).
 * 켜져 있는 동안 입력은 게임으로 — 이동·공격이 막히지 않는다.
 */
UJY_HUDLayout::UJY_HUDLayout(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InputMode = EJY_ScreenInputMode::Game;
	bIsBackHandler = false;
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

	/* ESC 메뉴를 처음 열 때 끊기지 않게 지금 비동기로 미리 올려 둔다 */
	if (EscapeMenuClass.IsNull() == false)
	{
		EscapeMenuPreloadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(EscapeMenuClass.ToSoftObjectPath());
	}

	/* 클라에는 GameMode가 없지만 GameState가 GameMode 클래스를 복제해 준다 → 그 기본값을 읽는다 */
	AGameStateBase* GameState = GetWorld()->GetGameState();
	if (GameState == nullptr)
	{
		GetWorld()->GameStateSetEvent.AddUObject(this, &UJY_HUDLayout::RefreshCombatPanel);
		CombatPanel->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	RefreshCombatPanel(GameState);
}

/* 예: 허브 GameMode bShowCombatHUD=false → 체력바 숨김, 던전 GameMode true → 표시 (하데스 허브엔 체력바가 없다) */
// [OwnClient]
void UJY_HUDLayout::RefreshCombatPanel(AGameStateBase* GameState)
{
	const AJY_GameMode* GameModeDefaults = GameState != nullptr ? GameState->GetDefaultGameMode<AJY_GameMode>() : nullptr;
	const bool bShowCombatHUD = GameModeDefaults == nullptr || GameModeDefaults->ShouldShowCombatHUD() == true;
	CombatPanel->SetVisibility(bShowCombatHUD == true ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

// [OwnClient]
void UJY_HUDLayout::HandleEscapeAction()
{
	UJY_UIManagerSubsystem* UIManager = GetUIManager();
	if (UIManager == nullptr)
		return;

	/* 미리 로드가 끝났으면 즉시 반환, 아직이면 그 자리에서 마저 로드 */
	const TSubclassOf<UJY_ScreenWidget> MenuClass = EscapeMenuClass.LoadSynchronous();
	if (MenuClass == nullptr)
		return;

	UIManager->PushScreen(JYGameplayTags::UI_Layer_Menu, MenuClass);
}

