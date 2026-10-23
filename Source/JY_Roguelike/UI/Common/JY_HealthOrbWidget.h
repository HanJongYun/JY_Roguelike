#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "JY_HealthOrbWidget.generated.h"

class UImage;

/* 액체가 출렁이는 체력 구슬 게이지 */
UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class UJY_HealthOrbWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	/* 채움 비율 지정 (0~1) → 액체 높이 */
	UFUNCTION(BlueprintSetter)
	void SetFillPercent(float InFillPercent);

protected:
	/* WBP_JY_HealthOrb 안의 같은 이름 위젯과 연결 (BindWidget) */

	/* 액체 — 머티리얼 MI_JY_HealthOrb의 Percentage가 높이 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> LiquidImage;

private:
	/* WBP_JY_HUD에서 Stats.HealthPercent → FillPercent로 바인딩 */

	/* 채움 비율 (0~1) */
	UPROPERTY(BlueprintReadWrite, BlueprintSetter = SetFillPercent, Category = "JY|Orb", meta = (AllowPrivateAccess))
	float FillPercent = 1.0f;
};
