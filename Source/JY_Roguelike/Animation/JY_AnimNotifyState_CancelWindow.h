#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "JY_AnimNotifyState_CancelWindow.generated.h"

/*
 * 스킬을 다른 동작(이동·다른 스킬·기본 공격)으로 끊을 수 있는 구간 표시 (동작 없음).
 * 스킬 GA가 자기 몽타주 재생 위치가 이 구간 안인지 직접 확인한다 — 태그를 붙이지 않아 블렌드 아웃 중인 이전 몽타주가 다음 스킬에 영향을 주지 않음
 */
UCLASS(Const, HideCategories = Object, CollapseCategories, Meta = (DisplayName = "JY Cancel Window"))
class UJY_AnimNotifyState_CancelWindow : public UAnimNotifyState
{
	GENERATED_BODY()
};
