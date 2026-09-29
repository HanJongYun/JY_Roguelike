#include "UI/Debug/JY_DebugTestMenu.h"
#include "Components/Button.h"
#include "System/JY_LogChannels.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_DebugTestMenu)

// [OwnClient]
void UJY_DebugTestMenu::NativeOnInitialized() //override
{
	Super::NativeOnInitialized();

	CloseButton->OnClicked.AddDynamic(this, &ThisClass::HandleCloseClicked);
}

/* 스스로 끄면 스택이 빼 준다 → 아래 화면(없으면 기본 입력)으로 복귀 */
// [OwnClient]
void UJY_DebugTestMenu::HandleCloseClicked()
{
	UE_LOG(LogJY, Log, TEXT("JYUI Close %s"), *GetName());
	DeactivateWidget();
}
