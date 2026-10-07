#include "UI/Screens/JY_EscapeMenu.h"
#include "UI/Foundation/JY_ButtonBase.h"
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

/* [UI 과외 임시 3-12] 확인 없이 바로 종료 — 4장에서 확인 팝업을 거치도록 바꾼다. PIE에서는 PIE 세션이 끝난다 */
// [OwnClient]
void UJY_EscapeMenu::HandleQuitClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}
