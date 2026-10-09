#include "UI/Common/JY_NotificationWidget.h"
#include "CommonTextBlock.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_NotificationWidget)

/*
 * 틀(WBP_JY_PrimaryLayout)이 만들어질 때 함께 만들어진다 — 알림마다 위젯을 새로 만들지 않는다.
 * 틀의 NativeOnInitialized보다 먼저 불린다 (UserWidget.cpp:252 자식 Initialize → :175 부모 NativeOnInitialized).
 */
// [OwnClient]
void UJY_NotificationWidget::NativeOnInitialized() //override
{
	Super::NativeOnInitialized();

	SetVisibility(ESlateVisibility::Collapsed);
}

/*
 * 맵 이동으로 틀이 화면에서 떼어질 때 — 표시 중이면 예약을 지운다.
 * 안 지우면 떼어진 옛 위젯이 GC 전까지 남아 2.5초마다 HandleDisplayFinished가 불려 화면에 없는 위젯에서 대기열을 계속 돌린다.
 */
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

/*
 * 표시 시간은 코어 티커(FTSTicker)로 잰다 — 월드 타이머는 정지 중 멈춰서(LevelTick.cpp:1812 !bIsPaused) 정지 메뉴를 열면 배너가 안 사라진다.
 * 코어 티커는 게임 정지와 무관하게 실제 시간으로 돈다. 매 프레임 검사하지 않고 "N초 뒤 1번"만 예약한다.
 */
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
