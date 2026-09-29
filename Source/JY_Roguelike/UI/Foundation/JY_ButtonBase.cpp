#include "UI/Foundation/JY_ButtonBase.h"
#include "CommonTextBlock.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_ButtonBase)

/* 디자이너 미리보기에서도 라벨이 보이도록 PreConstruct에서 적용 (ESC 메뉴 WBP에서 버튼마다 다른 글자 확인) */
// [OwnClient]
void UJY_ButtonBase::NativePreConstruct() //override
{
	Super::NativePreConstruct();

	SetButtonText(ButtonText);
	NativeOnCurrentTextStyleChanged();
}

/*
 * 기본·호버·눌림·비활성 상태가 바뀔 때마다 엔진이 부른다(CommonButtonBase.h:745).
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

// [OwnClient]
void UJY_ButtonBase::SetButtonText(const FText& InText)
{
	ButtonText = InText;

	if (ButtonTextBlock == nullptr)
		return;

	ButtonTextBlock->SetText(ButtonText);
}
