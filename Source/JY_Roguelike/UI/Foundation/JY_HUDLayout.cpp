#include "UI/Foundation/JY_HUDLayout.h"
#include "UI/JY_UIManagerSubsystem.h"
#include "System/JY_GameplayTags.h"
#include "System/JY_WorldSettings.h"
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

/* 뒤로가기를 Game 모드로 직접 등록 — bIsBackHandler 바인딩은 Menu 모드 전용이라 HUD에선 무시된다 (UIActionRouterTypes.cpp:851) */
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

	/* 정지 메뉴를 처음 열 때 끊기지 않게 미리 로드 — 핸들을 쥐고 있어야 메모리에 남는다 (StreamableManager.h:195) */
	if (EscapeMenuClass.IsNull() == false)
	{
		EscapeMenuPreloadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(EscapeMenuClass.ToSoftObjectPath());
	}

	RefreshCombatPanel();
}

/* World Settings는 맵 파일에 있어 클라도 바로 갖고 있다 — 복제를 기다릴 필요 없음 */
/* HUD는 맵마다 새로 만들어지므로 생성 때 한 번만 읽는다 */
// [OwnClient]
void UJY_HUDLayout::RefreshCombatPanel()
{
	const AJY_WorldSettings* WorldSettings = Cast<AJY_WorldSettings>(GetWorld()->GetWorldSettings());
	if (ensureMsgf(WorldSettings != nullptr, TEXT("World Settings가 AJY_WorldSettings가 아님 — DefaultEngine.ini WorldSettingsClassName 확인")) == false)
		return;

	const ESlateVisibility CombatVisibility = WorldSettings->ShouldShowCombatHUD() == true ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
	CombatPanel->SetVisibility(CombatVisibility);
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
