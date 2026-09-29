#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/JY_ScreenWidget.h"
#include "UI/JY_UITypes.h"
#include "JY_ConfirmDialog.generated.h"

class UCommonTextBlock;
class UJY_ButtonBase;

/* 제목·본문·확인/취소 팝업 — 결과를 연 쪽에 돌려준다 (Modal 층) */
UCLASS(Abstract, Blueprintable)
class UJY_ConfirmDialog : public UJY_ScreenWidget
{
	GENERATED_BODY()

public:
	UJY_ConfirmDialog(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeOnInitialized() override;
	virtual bool NativeOnHandleBackAction() override;

public:
	/* 글자와 결과 콜백을 채움 (UJY_UIManagerSubsystem::ShowConfirm이 호출) */
	void SetupDialog(const FText& Title, const FText& Body, FJY_OnConfirmResult InOnResult);

private:
	/* 확인 버튼 클릭 */
	void HandleConfirmClicked();

	/* 취소 버튼 클릭 */
	void HandleCancelClicked();

	/* 팝업을 닫고 결과를 한 번만 전달 */
	void FinishDialog(bool bConfirmed);

protected:
	/* WBP_JY_ConfirmDialog 안의 같은 이름 위젯과 연결 (BindWidget) */

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> TitleText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> BodyText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UJY_ButtonBase> ConfirmButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UJY_ButtonBase> CancelButton;

private:
	FJY_OnConfirmResult OnResult;
};
