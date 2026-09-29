#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/JY_ScreenWidget.h"
#include "JY_MetaAltarScreen.generated.h"

class UDynamicEntryBox;
class UJY_ButtonBase;

/* 메타 제단 화면 — 강화 목록 + 전액 환불 */
UCLASS(Abstract, Blueprintable)
class UJY_MetaAltarScreen : public UJY_ScreenWidget
{
	GENERATED_BODY()

public:
	UJY_MetaAltarScreen(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeOnInitialized() override;

private:
	/* 환불 클릭 → 확인 팝업 */
	void HandleRefundClicked();

	/* 확인 팝업 결과 → 서버에 환불 요청 */
	void HandleRefundConfirmResult(bool bConfirmed);

protected:
	/* WBP_JY_MetaAltar 안의 같은 이름 위젯과 연결 (BindWidget) */

	/* 항목 위젯(WBP_JY_MetaEntry)을 강화 수만큼 담는 칸 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UDynamicEntryBox> EntryBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UJY_ButtonBase> RefundButton;

private:
	/* 설정값 — WBP_JY_MetaAltar의 Class Defaults에서 수정 */

	/* 환불 확인 팝업 제목 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Meta")
	FText RefundConfirmTitle;

	/* 환불 확인 팝업 본문 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Meta")
	FText RefundConfirmBody;
};
