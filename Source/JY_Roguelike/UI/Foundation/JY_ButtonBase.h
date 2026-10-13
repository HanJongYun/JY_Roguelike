#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "Containers/Ticker.h"
#include "JY_ButtonBase.generated.h"

class UCommonTextBlock;
class UImage;

/* 모든 버튼의 부모 — 라벨 글자를 갖고, 모양·소리는 버튼 스타일 에셋이 정한다. 클릭하면 금빛 퍼짐 효과 */
UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class UJY_ButtonBase : public UCommonButtonBase
{
	GENERATED_BODY()

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnCurrentTextStyleChanged() override;
	virtual void NativeOnClicked() override;

private:
	/* 라벨 글자를 바꿈 */
	void SetButtonText(const FText& InText);

	/* 클릭 효과 한 프레임 — 진행도(0→1)를 머티리얼에 넣고, 끝나면 숨김 */
	bool TickClickBurst(float DeltaTime);

protected:
	/* WBP_JY_Button 안의 같은 이름 위젯과 연결 (BindWidget) */

	/* 라벨 글자 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> ButtonTextBlock;

	/* 클릭 효과 이미지 (머티리얼 M_JY_UI_ClickBurst, 평소 숨김) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ClickBurstImage;

private:
	/* 설정값 — 이 버튼을 배치한 WBP(정지 메뉴 등)의 Details에서 버튼마다 지정 */

	/* 버튼에 표시할 글자 (예: 계속하기) */
	UPROPERTY(EditAnywhere, Category = "JY|Button")
	FText ButtonText;

	/* 설정값 — WBP_JY_Button의 Class Defaults에서 수정 */

	/* 클릭 효과가 퍼졌다 사라지는 시간(초) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Button", meta = (ClampMin = "0.05"))
	float ClickBurstSeconds = 0.4f;

private:
	FTSTicker::FDelegateHandle ClickBurstTickerHandle;

	/* 클릭 후 지난 시간(초) */
	float ClickBurstElapsed = 0.0f;
};
