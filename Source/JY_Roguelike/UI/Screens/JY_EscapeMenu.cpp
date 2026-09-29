#include "UI/Screens/JY_EscapeMenu.h"
#include "UI/Foundation/JY_ButtonBase.h"
#include "UI/JY_UIManagerSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/KismetSystemLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_EscapeMenu)

/* ESC로 닫힘(bIsBackHandler) · 열려 있는 동안 정지 · 메뉴 입력만 */
UJY_EscapeMenu::UJY_EscapeMenu(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsBackHandler = true;
	bPauseGame = true;
	InputMode = EJY_ScreenInputMode::Menu;
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

/* ESC 메뉴는 열어 둔 채 Modal 층에 팝업 — 취소하면 메뉴로 돌아온다 */
// [OwnClient]
void UJY_EscapeMenu::HandleQuitClicked()
{
	UJY_UIManagerSubsystem* UIManager = ULocalPlayer::GetSubsystem<UJY_UIManagerSubsystem>(GetOwningLocalPlayer());
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
