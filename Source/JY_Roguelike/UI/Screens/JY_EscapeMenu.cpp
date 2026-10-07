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

/* 위젯이 처음 만들어질 때 1번만 불림 — 스택이 풀에서 재사용해도 다시 불리지 않아 구독이 두 겹이 되지 않는다 */
// [OwnClient]
void UJY_EscapeMenu::NativeOnInitialized() //override
{
	Super::NativeOnInitialized();

	ContinueButton->OnClicked().AddUObject(this, &ThisClass::HandleContinueClicked);
	QuitButton->OnClicked().AddUObject(this, &ThisClass::HandleQuitClicked);

	/* 비활성이면 클릭이 막히고 버튼 스타일의 Disabled 글자 스타일로 그려진다 */
	SettingsButton->SetIsEnabled(false);
}

/* 스스로 꺼지면 스택이 이 화면을 빼고, 아래 화면(HUD)의 입력 모드(Game)로 돌아간다 */
// [OwnClient]
void UJY_EscapeMenu::HandleContinueClicked()
{
	DeactivateWidget();
}

/* 팝업은 누가 열었는지 모른다 — 결과를 받을 내 함수를 맡겨 두면 버튼을 누를 때 불러 준다 */
// [OwnClient]
void UJY_EscapeMenu::HandleQuitClicked()
{
	UJY_UIManagerSubsystem* UIManagerSubsystem = ULocalPlayer::GetSubsystem<UJY_UIManagerSubsystem>(GetOwningLocalPlayer());
	if (UIManagerSubsystem == nullptr)
		return;

	UIManagerSubsystem->ShowConfirm(QuitConfirmTitle, QuitConfirmBody, FJY_OnConfirmResult::CreateUObject(this, &ThisClass::HandleQuitConfirmResult));
}

/* 취소면 아무것도 안 함 — 팝업만 닫히고 메뉴가 남는다. PIE에서 종료는 PIE 세션이 끝난다 */
// [OwnClient]
void UJY_EscapeMenu::HandleQuitConfirmResult(bool bConfirmed)
{
	if (bConfirmed == false)
		return;

	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}
