#include "UI/Screens/JY_EscapeMenu.h"
#include "UI/Foundation/JY_ButtonBase.h"
#include "UI/JY_UIManagerSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/KismetSystemLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_EscapeMenu)

/* 열려 있는 동안 정지 (뒤로가기로 닫힘·메뉴 입력은 부모 기본값) */
UJY_EscapeMenu::UJY_EscapeMenu(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bPauseGame = true;
}

// [OwnClient]
void UJY_EscapeMenu::NativeOnInitialized() //override
{
	Super::NativeOnInitialized();

	ContinueButton->OnClicked().AddUObject(this, &UJY_EscapeMenu::HandleContinueClicked);
	QuitButton->OnClicked().AddUObject(this, &UJY_EscapeMenu::HandleQuitClicked);
	SettingsButton->SetIsEnabled(false);
}

// [OwnClient]
void UJY_EscapeMenu::HandleContinueClicked()
{
	DeactivateWidget();
}

/* 정지 메뉴는 열어 둔 채 Modal 층에 팝업 — 취소하면 메뉴로 돌아온다 */
// [OwnClient]
void UJY_EscapeMenu::HandleQuitClicked()
{
	UJY_UIManagerSubsystem* UIManager = GetUIManager();
	if (UIManager == nullptr)
		return;

	UIManager->ShowConfirm(QuitConfirmTitle, QuitConfirmBody, FJY_OnConfirmResult::CreateUObject(this, &UJY_EscapeMenu::HandleQuitConfirmResult));
}

/* PIE에서는 PIE 세션만 끝난다 */
// [OwnClient]
void UJY_EscapeMenu::HandleQuitConfirmResult(bool bConfirmed)
{
	if (bConfirmed == false)
		return;

	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}
