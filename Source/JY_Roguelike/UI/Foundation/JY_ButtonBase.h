#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "JY_ButtonBase.generated.h"

class UCommonTextBlock;

/* 모든 버튼의 부모 — 라벨 글자를 갖고, 모양·소리는 버튼 스타일 에셋이 정한다 */
UCLASS(Abstract, Blueprintable)
class UJY_ButtonBase : public UCommonButtonBase
{
	GENERATED_BODY()

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeOnCurrentTextStyleChanged() override;

public:
	/* 라벨 글자를 바꿈 */
	void SetButtonText(const FText& InText);

protected:
	/* WBP_JY_Button 안의 같은 이름 위젯과 연결 (BindWidget) */

	/* 라벨 글자 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> ButtonTextBlock;

private:
	/* 설정값 — 이 버튼을 배치한 WBP(ESC 메뉴 등)의 Details에서 버튼마다 지정 */

	/* 버튼에 표시할 글자 (예: 계속하기) */
	UPROPERTY(EditAnywhere, Category = "JY|Button")
	FText ButtonText;
};
