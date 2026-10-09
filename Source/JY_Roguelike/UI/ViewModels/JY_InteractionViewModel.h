#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "Components/SlateWrapperTypes.h"
#include "JY_InteractionViewModel.generated.h"

/* 화면 아래 가운데 "[E] 이름" 안내 표시 값 — 내 컴퓨터에서만 정해지는 상태라 복제 없이 InteractionComponent가 직접 채운다 */
UCLASS(BlueprintType, Blueprintable)
class UJY_InteractionViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	/* 초점 대상 이름을 보여 줌 (빈 글자면 숨김) */
	void SetFocusedName(const FText& InName);

	FText GetPromptText() const { return PromptText; }
	ESlateVisibility GetPromptVisibility() const { return PromptVisibility; }

private:
	/*
	 * 위젯이 바인딩하는 값 (FieldNotify — 값이 바뀌면 위젯이 자동 갱신)
	 * WBP_JY_HUD InteractPrompt가 PromptText·PromptVisibility 사용 (7-2에서 연결)
	 */

	/* 대상 이름 (예: 제단) */
	UPROPERTY(BlueprintReadOnly, Transient, FieldNotify, Getter, Category = "JY|Interaction", meta = (AllowPrivateAccess))
	FText PromptText;

	/* 안내 전체 보이기 — 초점 대상이 없으면 Collapsed */
	UPROPERTY(BlueprintReadOnly, Transient, FieldNotify, Getter, Category = "JY|Interaction", meta = (AllowPrivateAccess))
	ESlateVisibility PromptVisibility = ESlateVisibility::Collapsed;
};
