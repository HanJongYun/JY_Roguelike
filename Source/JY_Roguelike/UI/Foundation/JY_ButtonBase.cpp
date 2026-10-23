#include "UI/Foundation/JY_ButtonBase.h"
#include "CommonTextBlock.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_ButtonBase)

/* 디자이너 미리보기에서도 라벨이 보이도록 PreConstruct에서 적용 */
// [OwnClient]
void UJY_ButtonBase::NativePreConstruct() //override
{
	Super::NativePreConstruct();

	SetButtonText(ButtonText);

	/* 디자이너 미리보기에도 평소 상태 글자 스타일이 보이도록 한 번 적용 */
	NativeOnCurrentTextStyleChanged();
}

/* 버튼 상태가 바뀔 때마다 엔진이 부름 (CommonButtonBase.cpp:1781) — 상태별 글자 스타일을 스타일 에셋 한 곳에서 관리 */
// [OwnClient]
void UJY_ButtonBase::NativeOnCurrentTextStyleChanged() //override
{
	Super::NativeOnCurrentTextStyleChanged();

	if (ButtonTextBlock == nullptr)
		return;

	const TSubclassOf<UCommonTextStyle> TextStyleClass = GetCurrentTextStyleClass();
	if (TextStyleClass != nullptr)
	{
		ButtonTextBlock->SetStyle(TextStyleClass);
	}
}

/* 클릭과 함께 화면이 닫히면 효과 도중 여기로 온다 — 숨기지 않으면 풀에서 재사용될 때 멈춘 고리가 보인다 */
// [OwnClient]
void UJY_ButtonBase::NativeDestruct() //override
{
	FTSTicker::RemoveTicker(ClickBurstTickerHandle);
	ClickBurstTickerHandle.Reset();
	ClickBurstImage->SetVisibility(ESlateVisibility::Collapsed);

	Super::NativeDestruct();
}

/* 코어 티커 사용 — 월드 타이머는 정지 중 멈춰 정지 메뉴 버튼에서 효과가 안 난다 */
// [OwnClient]
void UJY_ButtonBase::NativeOnClicked() //override
{
	Super::NativeOnClicked();

	ClickBurstElapsed = 0.0f;
	ClickBurstImage->SetVisibility(ESlateVisibility::HitTestInvisible);

	if (ClickBurstTickerHandle.IsValid() == true)
		return;

	FTickerDelegate TickDelegate = FTickerDelegate::CreateUObject(this, &ThisClass::TickClickBurst);
	ClickBurstTickerHandle = FTSTicker::GetCoreTicker().AddTicker(MoveTemp(TickDelegate));
}

/* 반환값 false = 티커에서 빠짐 */
// [OwnClient]
bool UJY_ButtonBase::TickClickBurst(float DeltaTime)
{
	ClickBurstElapsed += DeltaTime;
	const float Progress = FMath::Clamp(ClickBurstElapsed / ClickBurstSeconds, 0.0f, 1.0f);

	UMaterialInstanceDynamic* BurstMaterial = ClickBurstImage->GetDynamicMaterial();
	BurstMaterial->SetScalarParameterValue(TEXT("Progress"), Progress);

	if (Progress < 1.0f)
		return true;

	ClickBurstImage->SetVisibility(ESlateVisibility::Collapsed);
	ClickBurstTickerHandle.Reset();
	return false;
}

// [OwnClient]
void UJY_ButtonBase::SetButtonText(const FText& InText)
{
	ButtonText = InText;

	if (ButtonTextBlock == nullptr)
		return;

	ButtonTextBlock->SetText(ButtonText);
}
