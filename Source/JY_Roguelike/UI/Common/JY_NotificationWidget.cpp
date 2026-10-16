#include "UI/Common/JY_NotificationWidget.h"
#include "CommonTextBlock.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_NotificationWidget)

/* 틀(WBP_JY_PrimaryLayout)과 함께 한 번 만들어져 재사용 — 틀의 NativeOnInitialized보다 먼저 불린다 (UserWidget.cpp:252) */
// [OwnClient]
void UJY_NotificationWidget::NativeOnInitialized() //override
{
	Super::NativeOnInitialized();

	SetVisibility(ESlateVisibility::Collapsed);
}

/* 예약을 지울 것 — 안 지우면 떼어진 위젯이 GC 전까지 남아 화면 밖에서 대기열을 계속 돌린다 */
// [OwnClient]
void UJY_NotificationWidget::NativeDestruct() //override
{
	FTSTicker::RemoveTicker(DisplayTimerHandle);
	DisplayTimerHandle.Reset();

	Super::NativeDestruct();
}

// [OwnClient]
void UJY_NotificationWidget::EnqueueMessage(const FText& Message)
{
	if (PendingMessages.Num() >= MaxPendingMessages)
	{
		PendingMessages.PopFirst();
	}

	PendingMessages.PushLast(Message);

	/* 표시 중이면 끝난 뒤 HandleDisplayFinished가 차례대로 꺼낸다 */
	if (DisplayTimerHandle.IsValid() == true)
		return;

	ShowNextMessage();
}

/* 월드 타이머가 아닌 코어 티커 — 월드 타이머는 정지 중 멈춰(LevelTick.cpp:1812) 정지 메뉴를 열면 배너가 안 사라진다 */
// [OwnClient]
void UJY_NotificationWidget::ShowNextMessage()
{
	if (PendingMessages.Num() == 0)
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	MessageText->SetText(PendingMessages.First());
	PendingMessages.PopFirst();

	/* 보이되 클릭은 통과 — 알림이 떠 있어도 뒤 화면·게임 입력을 막지 않는다 */
	SetVisibility(ESlateVisibility::HitTestInvisible);

	DisplayTimerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UJY_NotificationWidget::HandleDisplayFinished), DisplaySeconds);
}

// [OwnClient]
bool UJY_NotificationWidget::HandleDisplayFinished(float DeltaTime)
{
	DisplayTimerHandle.Reset();
	ShowNextMessage();

	/* false = 이 예약은 1번으로 끝 (true면 DisplaySeconds마다 반복) */
	return false;
}
