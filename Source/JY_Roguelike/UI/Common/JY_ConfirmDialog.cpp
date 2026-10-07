#include "UI/Common/JY_ConfirmDialog.h"
#include "UI/Foundation/JY_ButtonBase.h"
#include "CommonTextBlock.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_ConfirmDialog)

/*
 * 떠 있는 동안 정지 — 정지 메뉴 위에 떠도 정지 화면 목록이 2개라 팝업만 닫아선 풀리지 않는다. 뒤로가기로 닫힘·메뉴 입력은 부모 기본값.
 * 팝업은 누가 열었는지 모른다 — 결과만 OnResult로 돌려준다 (Lyra UCommonMessagingSubsystem::ShowConfirmation과 같은 방식).
 */
UJY_ConfirmDialog::UJY_ConfirmDialog(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bPauseGame = true;
}

/* 위젯이 처음 만들어질 때 1번만 불림 — 스택이 풀에서 재사용해도 구독이 두 겹이 되지 않는다 */
// [OwnClient]
void UJY_ConfirmDialog::NativeOnInitialized() //override
{
	Super::NativeOnInitialized();

	ConfirmButton->OnClicked().AddUObject(this, &ThisClass::HandleConfirmClicked);
	CancelButton->OnClicked().AddUObject(this, &ThisClass::HandleCancelClicked);
}

/* 뒤로가기(Tab) = 취소 — 엔진 기본은 닫기만 해서(CommonActivatableWidget.cpp:355) 연 쪽이 결과를 못 받는다 */
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
 * 닫기를 먼저 한다 — 결과 함수가 같은 층에 새 팝업을 올리면, 이 팝업은 스택이 가린 채 꺼 두고(CommonActivatableWidgetContainer.cpp:163)
 * 이후 DeactivateWidget은 이미 꺼져 있어 무시된다(CommonActivatableWidget.cpp:144) → 새 팝업을 닫으면 이 팝업이 다시 켜진다(:302).
 */
// [OwnClient]
void UJY_ConfirmDialog::FinishDialog(bool bConfirmed)
{
	DeactivateWidget();
	OnResult.ExecuteIfBound(bConfirmed);
}
