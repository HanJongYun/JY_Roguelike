#include "UI/Common/JY_InteractPrompt.h"
#include "CommonTextBlock.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_InteractPrompt)

// [OwnClient]
void UJY_InteractPrompt::SetPromptText(const FText& InText)
{
	PromptText->SetText(InText);
}
