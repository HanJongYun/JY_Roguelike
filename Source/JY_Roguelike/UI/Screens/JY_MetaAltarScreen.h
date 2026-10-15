#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/JY_ScreenWidget.h"
#include "JY_MetaAltarScreen.generated.h"

class UDynamicEntryBox;
class UJY_ButtonBase;

/* 영혼의 제단 화면 — 영혼석으로 영구 강화를 산다 */
UCLASS(Abstract, Blueprintable)
class UJY_MetaAltarScreen : public UJY_ScreenWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnActivated() override;

private:
	/* 전액 환불 클릭 → 확인 팝업 */
	void HandleRefundClicked();

	/* 확인 팝업 결과 → 확인이면 서버에 환불 요청 */
	void HandleRefundConfirmResult(bool bConfirmed);

	/* 돌아가기 클릭 → 화면 닫기 (Tab 뒤로가기와 같음) */
	void HandleCloseClicked();

protected:
	/* WBP_JY_MetaAltar 안의 같은 이름 위젯과 연결 (BindWidget) */

	/* 강화 줄(WBP_JY_MetaEntry)을 강화 수만큼 담는 칸 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UDynamicEntryBox> EntryBox;

	/* 전액 환불 (작은 버튼, 오른쪽 아래) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UJY_ButtonBase> RefundButton;

	/* 돌아가기 (아래 가운데) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UJY_ButtonBase> CloseButton;

private:
	/* 설정값 — WBP_JY_MetaAltar의 Class Defaults에서 수정 */

	/* 환불 확인 팝업 제목 (예: 전액 환불) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Meta")
	FText RefundConfirmTitle;

	/* 환불 확인 팝업 본문 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Meta")
	FText RefundConfirmBody;
};
