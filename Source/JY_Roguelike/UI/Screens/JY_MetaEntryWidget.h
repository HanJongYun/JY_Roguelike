#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "JY_MetaEntryWidget.generated.h"

class UJY_ButtonBase;
class UImage;
class UJY_MetaEntryViewModel;
class AJY_InteractableActor;

/* 제단 목록의 강화 한 줄 — 아이콘·이름·설명·단계·가격·강화 버튼 */
UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class UJY_MetaEntryWidget : public UCommonUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;

public:
	/* 이 줄이 보여 줄 ViewModel(WBP의 ViewModel "MetaEntry", Creation Type = Manual)과 구매 요청에 실을 제단 지정 + 아이콘 */
	void SetEntry(UJY_MetaEntryViewModel* InEntryViewModel, AJY_InteractableActor* InAltar);

private:
	/* 강화 클릭 → 서버에 구매 요청 */
	void HandleBuyClicked();

protected:
	/* WBP_JY_MetaEntry 안의 같은 이름 위젯과 연결 (BindWidget) */

	/* 강화 버튼 — 활성 여부를 WBP 바인딩(MetaEntry.bCanBuy → bIsEnabled)이 바꿔서 BlueprintReadOnly 필요 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UJY_ButtonBase> BuyButton;

	/* 강화 아이콘 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> IconImage;

private:
	/* 이 줄의 ViewModel — 구매 요청에 강화 종류(UpgradeTag)를 꺼낸다 */
	UPROPERTY(Transient)
	TObjectPtr<UJY_MetaEntryViewModel> EntryViewModel;

	/* 이 화면을 연 제단 — 화면이 떠 있는 동안 사라질 수 있어 약한 참조 */
	TWeakObjectPtr<AJY_InteractableActor> Altar;
};
