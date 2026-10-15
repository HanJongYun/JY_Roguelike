#include "UI/Foundation/JY_ButtonBase.h"
#include "CommonTextBlock.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_ButtonBase)

/* 디자이너 미리보기에서도 라벨이 보이도록 PreConstruct에서 적용 (정지 메뉴 WBP에서 버튼마다 다른 글자 확인) */
// [OwnClient]
void UJY_ButtonBase::NativePreConstruct() //override
{
	Super::NativePreConstruct();

	SetButtonText(ButtonText);

	/* 디자이너 미리보기에도 평소 상태 글자 스타일이 보이도록 한 번 적용 */
	NativeOnCurrentTextStyleChanged();
}

/*
 * 평소·호버·눌림·비활성 상태가 바뀔 때마다 엔진이 부른다
 * (CommonButtonBase.cpp:1781 NativeOnHovered, :1941 NativeOnDisabled, :2102 SetButtonStyle).
 * 버튼 스타일 에셋의 상태별 글자 스타일(NormalTextStyle 등)을 라벨에 입힌다 — 색·폰트는 스타일 에셋 한 곳에서 바뀐다.
 */
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

/*
 * 클릭과 동시에 화면이 닫히는 버튼(확인·돌아가기)은 효과 도중 여기로 온다 — 티커를 떼고 효과도 숨긴다.
 * 숨기지 않으면 화면이 보관함(풀)에서 다시 쓰일 때 멈춘 고리가 그대로 보인다.
 */
// [OwnClient]
void UJY_ButtonBase::NativeDestruct() //override
{
	FTSTicker::RemoveTicker(ClickBurstTickerHandle);
	ClickBurstTickerHandle.Reset();
	ClickBurstImage->SetVisibility(ESlateVisibility::Collapsed);

	Super::NativeDestruct();
}

/*
 * 눌렀다 뗐을 때(클릭 완료) 엔진이 부른다 — 호버·누르는 중에는 오지 않는다.
 * 효과는 머티리얼(M_JY_UI_ClickBurst)이 Progress 0→1로 고리·광선을 퍼뜨리고 흐려진다. 여기선 Progress만 넘긴다.
 * 코어 티커를 쓰는 이유: 정지 메뉴 버튼도 효과가 나야 하는데 월드 타이머는 정지 중 멈춘다(6장 알림과 같은 이유).
 * 연타하면 처음부터 다시 퍼진다.
 */
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
