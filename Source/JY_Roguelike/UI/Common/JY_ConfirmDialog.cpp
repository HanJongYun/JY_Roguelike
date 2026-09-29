#include "UI/Common/JY_ConfirmDialog.h"
#include "UI/Foundation/JY_ButtonBase.h"
#include "CommonTextBlock.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_ConfirmDialog)

/*
 * ESC = 취소(bIsBackHandler). 팝업이 떠 있는 동안 게임은 정지 — ESC 메뉴 위에 떠도 메뉴가 닫히기 전까지 정지 유지(정지 화면 목록).
 * 팝업은 누가 열었는지 모른다 — 결과만 콜백으로 돌려준다 (Lyra UCommonMessagingSubsystem::ShowConfirmation과 같은 방식).
 */
UJY_ConfirmDialog::UJY_ConfirmDialog(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsBackHandler = true;
	bPauseGame = true;
	InputMode = EJY_ScreenInputMode::Menu;
}

// [OwnClient]
void UJY_ConfirmDialog::NativeOnInitialized() //override
{
	Super::NativeOnInitialized();

	ConfirmButton->OnClicked().AddUObject(this, &UJY_ConfirmDialog::HandleConfirmClicked);
	CancelButton->OnClicked().AddUObject(this, &UJY_ConfirmDialog::HandleCancelClicked);
}

/* 엔진 기본 동작(바로 닫기) 대신 취소 결과를 전달하며 닫는다 */
// [OwnClient]
bool UJY_ConfirmDialog::NativeOnHandleBackAction() //override
{
	FinishDialog(false);
	return true;
}

// [OwnClient]
void UJY_ConfirmDialog::SetupDialog(const FText& Title, const FText& Body, FJY_OnConfirmResult InOnResult)
{
	TitleText->SetText(Title);
	BodyText->SetText(Body);
	OnResult = MoveTemp(InOnResult);
}

// [OwnClient]
void UJY_ConfirmDialog::HandleConfirmClicked()
{
	FinishDialog(true);
}

// [OwnClient]
void UJY_ConfirmDialog::HandleCancelClicked()
{
	FinishDialog(false);
}

/*
 * 먼저 닫고 나서 결과 전달 — 콜백이 새 화면을 열어도(예: 확인 → 다른 팝업) 스택 순서가 꼬이지 않는다.
 * 콜백을 꺼내 비워 두고 실행 → 연타해도 결과는 한 번만 간다.
 */
// [OwnClient]
void UJY_ConfirmDialog::FinishDialog(bool bConfirmed)
{
	FJY_OnConfirmResult ResultToSend = MoveTemp(OnResult);
	OnResult.Unbind();

	DeactivateWidget();
	ResultToSend.ExecuteIfBound(bConfirmed);
}
