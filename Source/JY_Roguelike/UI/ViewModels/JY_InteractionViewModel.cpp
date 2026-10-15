#include "UI/ViewModels/JY_InteractionViewModel.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_InteractionViewModel)

/* 보일 때 Visible 대신 HitTestInvisible — 안내는 읽기만 하는 표시라 클릭을 받지 않는다 */
// [OwnClient]
void UJY_InteractionViewModel::SetFocusedName(const FText& InName)
{
	UE_MVVM_SET_PROPERTY_VALUE(PromptText, InName);
	UE_MVVM_SET_PROPERTY_VALUE(PromptVisibility, InName.IsEmpty() == true ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}
