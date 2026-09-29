#include "UI/Common/JY_BannerWidget.h"
#include "CommonTextBlock.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_BannerWidget)

/* 레이아웃에 1개씩 미리 만들어 두고 계속 재사용 — 알림마다 위젯을 새로 만들지 않는다 */
// [OwnClient]
void UJY_BannerWidget::NativeOnInitialized() //override
{
	Super::NativeOnInitialized();

	SetVisibility(ESlateVisibility::Collapsed);
}

// [OwnClient]
void UJY_BannerWidget::NativeDestruct() //override
{
	FTSTicker::RemoveTicker(DisplayTimerHandle);
	DisplayTimerHandle.Reset();

	Super::NativeDestruct();
}

// [OwnClient]
void UJY_BannerWidget::EnqueueMessage(const FText& Message)
{
	if (Message.IsEmpty() == true)
		return;

	if (PendingMessages.Num() >= MaxPendingMessages)
	{
		PendingMessages.RemoveAt(0);
	}

	PendingMessages.Add(Message);

	/* 표시 중이면 끝난 뒤 차례대로 */
	if (DisplayTimerHandle.IsValid() == true)
		return;

	ShowNextMessage();
}

/*
 * 표시 시간은 코어 티커(FTSTicker)로 잰다 — 월드 타이머는 ESC 정지 중 멈춰서(LevelTick.cpp:1812) 배너가 안 사라진다.
 * 매 프레임 검사하지 않고 "N초 뒤 1회" 예약만 한다.
 */
// [OwnClient]
void UJY_BannerWidget::ShowNextMessage()
{
	if (PendingMessages.Num() == 0)
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	MessageText->SetText(PendingMessages[0]);
	PendingMessages.RemoveAt(0);

	/* 보이되 클릭은 통과 — 알림이 떠 있어도 뒤 화면·게임 입력을 막지 않는다 */
	SetVisibility(ESlateVisibility::HitTestInvisible);

	DisplayTimerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UJY_BannerWidget::HandleDisplayFinished), DisplaySeconds);
}

// [OwnClient]
bool UJY_BannerWidget::HandleDisplayFinished(float DeltaTime)
{
	DisplayTimerHandle.Reset();
	ShowNextMessage();

	/* false = 이 티커는 1회로 끝 */
	return false;
}
