#include "UI/Screens/JY_TitleScreen.h"
#include "UI/Foundation/JY_ButtonBase.h"
#include "UI/JY_UIManagerSubsystem.h"
#include "Player/JY_TitlePlayerController.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/KismetSystemLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_TitleScreen)

/* 위젯이 처음 만들어질 때 1번만 — 풀 재사용 시 구독이 두 겹이 되지 않는다 */
// [OwnClient]
void UJY_TitleScreen::NativeOnInitialized() //override
{
	Super::NativeOnInitialized();

	StartButton->OnClicked().AddUObject(this, &ThisClass::HandleStartClicked);
	QuitButton->OnClicked().AddUObject(this, &ThisClass::HandleQuitClicked);

	SettingsButton->SetIsEnabled(false);
}

/* 이동은 서버가 정한다 — 화면은 요청만 보낸다. 연타는 엔진이 무시(World.cpp:9540) */
// [OwnClient]
void UJY_TitleScreen::HandleStartClicked()
{
	AJY_TitlePlayerController* TitlePlayerController = GetOwningPlayer<AJY_TitlePlayerController>();
	TitlePlayerController->Server_StartGame();
}

// [OwnClient]
void UJY_TitleScreen::HandleQuitClicked()
{
	UJY_UIManagerSubsystem* UIManagerSubsystem = ULocalPlayer::GetSubsystem<UJY_UIManagerSubsystem>(GetOwningLocalPlayer());
	if (UIManagerSubsystem == nullptr)
		return;

	UIManagerSubsystem->ShowConfirm(QuitConfirmTitle, QuitConfirmBody, FJY_OnConfirmResult::CreateUObject(this, &ThisClass::HandleQuitConfirmResult));
}

/* PIE에서 종료는 PIE 세션이 끝난다 */
// [OwnClient]
void UJY_TitleScreen::HandleQuitConfirmResult(bool bConfirmed)
{
	if (bConfirmed == false)
		return;

	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}
