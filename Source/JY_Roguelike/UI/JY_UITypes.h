#pragma once

#include "CoreMinimal.h"
#include "JY_UITypes.generated.h"

/* 알림 종류 — 어느 알림 칸에 띄울지 */
UENUM()
enum class EJY_NotificationType : uint8
{
	/* 화면 위쪽 전체 폭 큰 글자 (지역 이름 등) */
	Banner,

	/* 우상단 작은 글자 (강화 구매 등) */
	Toast
};

/* 확인 팝업 결과 — true = 확인, false = 취소 */
DECLARE_DELEGATE_OneParam(FJY_OnConfirmResult, bool /*bConfirmed*/);
