#include "UI/Common/JY_HealthOrbWidget.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_HealthOrbWidget)

/* MVVM 바인딩이 FillPercent에 쓰면 BlueprintSetter라 여기로 들어온다 — 높이는 머티리얼이 그린다 */
// [OwnClient]
void UJY_HealthOrbWidget::SetFillPercent(float InFillPercent)
{
	FillPercent = InFillPercent;

	UMaterialInstanceDynamic* LiquidMaterial = LiquidImage->GetDynamicMaterial();
	LiquidMaterial->SetScalarParameterValue(TEXT("Percentage"), InFillPercent);
}
