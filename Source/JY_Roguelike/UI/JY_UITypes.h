#pragma once

#include "CoreMinimal.h"
#include "GenericPlatform/ICursor.h"
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

/* 마우스 커서 그림 한 벌 — DA_JY_GameData에서 지정 */
USTRUCT()
struct FJY_CursorData
{
	GENERATED_BODY()

	/* 그림을 씌울 커서 모양 (Default = 평소 화살표, Hand = 버튼 위 손가락) */
	UPROPERTY(EditDefaultsOnly, Category = "Cursor")
	TArray<TEnumAsByte<EMouseCursor::Type>> Shapes;

	/* 평소 그림 — Content 기준 PNG 경로, 확장자 없이 (예: Slate/Cursors/Sword) */
	UPROPERTY(EditDefaultsOnly, Category = "Cursor")
	FName NormalImagePath;

	/* 마우스 왼쪽 버튼을 누르고 있는 동안 그림 (예: Slate/Cursors/SwordGold) */
	UPROPERTY(EditDefaultsOnly, Category = "Cursor")
	FName PressedImagePath;

	/* 클릭이 들어가는 점 — 그림 안 비율 0~1 (0,0 = 왼쪽 위 칼끝) */
	UPROPERTY(EditDefaultsOnly, Category = "Cursor", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	FVector2D ClickPoint = FVector2D::ZeroVector;
};
