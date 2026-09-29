#include "UI/ViewModels/JY_InteractionViewModel.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_InteractionViewModel)

/* 이름은 대상 액터의 FText를 복사 — 같은 대상이면 같은 데이터라 다시 알리지 않는다 */
// [OwnClient]
void UJY_InteractionViewModel::SetFocusedName(const FText& InName)
{
	UE_MVVM_SET_PROPERTY_VALUE(PromptText, InName);
	UE_MVVM_SET_PROPERTY_VALUE(PromptVisibility, InName.IsEmpty() == true ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}
