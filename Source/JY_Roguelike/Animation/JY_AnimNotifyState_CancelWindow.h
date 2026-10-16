#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "JY_AnimNotifyState_CancelWindow.generated.h"

/* 스킬을 다른 동작으로 끊을 수 있는 구간 표시 (동작 없음 — 스킬 GA가 재생 위치로 직접 확인) */
/* 태그로 바꾸지 말 것 — 블렌드 아웃 중인 이전 몽타주가 다음 스킬에 영향을 준다 */
UCLASS(Const, HideCategories = Object, CollapseCategories, Meta = (DisplayName = "JY Cancel Window"))
class UJY_AnimNotifyState_CancelWindow : public UAnimNotifyState
{
	GENERATED_BODY()
};
