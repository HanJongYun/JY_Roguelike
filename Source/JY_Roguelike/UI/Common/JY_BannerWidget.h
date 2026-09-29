#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "Containers/Ticker.h"
#include "JY_BannerWidget.generated.h"

class UCommonTextBlock;

/* 글자를 잠깐 보여 주고 사라지는 알림 칸 — 여러 개가 오면 차례대로 표시 */
UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class UJY_BannerWidget : public UCommonUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

public:
	/* 알림 1개를 대기열에 넣음 (비어 있으면 바로 표시) */
	void EnqueueMessage(const FText& Message);

private:
	/* 대기열 맨 앞을 표시하거나, 비었으면 숨김 */
	void ShowNextMessage();

	/* 표시 시간이 끝났을 때 (코어 티커가 1회 호출) */
	bool HandleDisplayFinished(float DeltaTime);

protected:
	/* WBP 안의 같은 이름 위젯과 연결 (BindWidget) */

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> MessageText;

private:
	/* 설정값 — 이 위젯을 배치한 WBP_JY_PrimaryLayout의 Details에서 수정 */

	/* 알림 1개를 보여 주는 시간(초, 실제 시간 — 게임 정지와 무관) */
	UPROPERTY(EditAnywhere, Category = "JY|Notification", meta = (ClampMin = "0.5"))
	float DisplaySeconds = 2.5f;

	/* 대기열 최대 개수 — 넘치면 가장 오래된 것부터 버림 */
	UPROPERTY(EditAnywhere, Category = "JY|Notification", meta = (ClampMin = "1"))
	int32 MaxPendingMessages = 4;

private:
	TArray<FText> PendingMessages;

	FTSTicker::FDelegateHandle DisplayTimerHandle;
};
