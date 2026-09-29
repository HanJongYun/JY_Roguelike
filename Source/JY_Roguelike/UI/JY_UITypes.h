#pragma once

#include "CoreMinimal.h"
#include "JY_UITypes.generated.h"

/* 알림 종류 — 어느 알림 칸에 띄울지. 종류 추가 = 값 1줄 + 레이아웃 BindWidget 1개 + UJY_PrimaryLayout::GetNotificationWidget case 1줄 */
UENUM(BlueprintType)
enum class EJY_NotificationType : uint8
{
	/* 화면 위쪽 큰 글자 (지역 이름 등) */
	Banner,

	/* 화면 아래쪽 작은 글자 (구매 완료 등) */
	Toast
};

/* 확인 팝업 결과 — true = 확인, false = 취소·뒤로가기 */
DECLARE_DELEGATE_OneParam(FJY_OnConfirmResult, bool /*bConfirmed*/);
