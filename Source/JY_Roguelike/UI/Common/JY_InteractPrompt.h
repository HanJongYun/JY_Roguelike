#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "JY_InteractPrompt.generated.h"

class UCommonTextBlock;

/* 상호작용 대상 머리 위 "[E] 이름" 안내 */
UCLASS(Abstract, Blueprintable)
class UJY_InteractPrompt : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	/* 대상 이름 표시 */
	void SetPromptText(const FText& InText);

protected:
	/* WBP_JY_InteractPrompt 안의 같은 이름 위젯과 연결 (BindWidget) */

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> PromptText;
};
