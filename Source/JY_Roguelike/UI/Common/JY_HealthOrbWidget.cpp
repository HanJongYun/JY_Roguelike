#include "UI/Common/JY_HealthOrbWidget.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_HealthOrbWidget)

/*
 * MVVM 바인딩이 FillPercent에 값을 쓸 때 BlueprintSetter라 이 함수로 들어온다.
 * 액체 높이는 머티리얼이 그린다 — 여기선 비율만 넘긴다. 첫 호출 때 동적 머티리얼이 만들어져 이 위젯 전용이 된다.
 */
// [OwnClient]
void UJY_HealthOrbWidget::SetFillPercent(float InFillPercent)
{
	FillPercent = InFillPercent;

	UMaterialInstanceDynamic* LiquidMaterial = LiquidImage->GetDynamicMaterial();
	LiquidMaterial->SetScalarParameterValue(TEXT("Percentage"), InFillPercent);
}
