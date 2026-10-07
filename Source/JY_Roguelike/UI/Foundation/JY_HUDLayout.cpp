#include "UI/Foundation/JY_HUDLayout.h"
#include "UI/JY_UIManagerSubsystem.h"
#include "System/JY_GameplayTags.h"
#include "Engine/LocalPlayer.h"
#include "Engine/AssetManager.h"
#include "ICommonInputModule.h"
#include "CommonInputSettings.h"
#include "Input/CommonUIInputTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_HUDLayout)

/* HUD는 켜져 있는 동안 캐릭터 이동·공격이 돼야 하고(Game), Tab으로 닫히면 안 된다 */
UJY_HUDLayout::UJY_HUDLayout(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InputMode = EJY_ScreenInputMode::Game;
	bIsBackHandler = false;
}

/*
 * bIsBackHandler를 쓰지 않는 이유: 엔진이 등록하는 뒤로가기 바인딩은 Menu 모드 전용(CommonUIInputTypes.h:62 기본값)이고,
 * 라우터는 현재 입력 모드와 바인딩 모드가 같을 때만 실행한다(UIActionRouterTypes.cpp:851) → Game 모드인 HUD에선 무시된다.
 * 그래서 같은 뒤로가기 액션을 Game 모드로 직접 등록한다.
 * 정지 메뉴가 열려 있으면 입력 모드가 Menu라 이 바인딩은 무시되고, 메뉴의 뒤로가기(닫기)가 받는다.
 */
// [OwnClient]
void UJY_HUDLayout::NativeOnInitialized() //override
{
	Super::NativeOnInitialized();

	const UInputAction* BackAction = ICommonInputModule::GetSettings().GetEnhancedInputBackAction();
	if (BackAction != nullptr)
	{
		FBindUIActionArgs BindArgs(BackAction, false, FSimpleDelegate::CreateUObject(this, &ThisClass::HandleBackAction));
		BindArgs.InputMode = ECommonInputMode::Game;
		RegisterUIActionBinding(BindArgs);
	}

	/* 정지 메뉴를 처음 열 때 끊기지 않게 지금 비동기로 미리 올려 둔다 — 핸들을 쥐고 있는 동안 로드한 클래스가 메모리에 남는다(StreamableManager.h:195) */
	if (EscapeMenuClass.IsNull() == false)
	{
		EscapeMenuPreloadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(EscapeMenuClass.ToSoftObjectPath());
	}
}

// [OwnClient]
void UJY_HUDLayout::HandleBackAction()
{
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	if (LocalPlayer == nullptr)
		return;

	UJY_UIManagerSubsystem* UIManagerSubsystem = LocalPlayer->GetSubsystem<UJY_UIManagerSubsystem>();
	if (UIManagerSubsystem == nullptr)
		return;

	/* 미리 로드가 끝났으면 즉시 반환, 아직이면 그 자리에서 마저 로드 */
	const TSubclassOf<UJY_ScreenWidget> MenuClass = EscapeMenuClass.LoadSynchronous();
	if (MenuClass == nullptr)
		return;

	UIManagerSubsystem->PushScreen(JYGameplayTags::UI_Layer_Menu, MenuClass);
}
