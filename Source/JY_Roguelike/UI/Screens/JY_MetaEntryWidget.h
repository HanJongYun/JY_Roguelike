#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "JY_MetaEntryWidget.generated.h"

class UJY_ButtonBase;
class UJY_MetaEntryViewModel;

/* 제단 목록 항목 1개 — 글자는 WBP 바인딩이 ViewModel에서 가져오고, 구매 버튼만 C++이 처리 */
UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class UJY_MetaEntryWidget : public UCommonUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;

public:
	/* 이 항목이 보여 줄 ViewModel 지정 (WBP의 ViewModel "MetaEntry", Creation Type = Manual) */
	void SetEntryViewModel(UJY_MetaEntryViewModel* InViewModel);

private:
	/* 구매 클릭 → 서버에 요청 */
	void HandleBuyClicked();

protected:
	/* WBP_JY_MetaEntry 안의 같은 이름 위젯과 연결 (BindWidget) */

	/* 활성 여부는 WBP 바인딩(MetaEntry.bCanBuy → bIsEnabled) — 바인딩 대상이라 BlueprintReadOnly */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UJY_ButtonBase> BuyButton;

private:
	TWeakObjectPtr<UJY_MetaEntryViewModel> EntryViewModel;
};
