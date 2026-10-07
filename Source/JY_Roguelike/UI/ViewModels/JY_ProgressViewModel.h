#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "JY_ProgressViewModel.generated.h"

class AJY_PlayerState;

/* 런이 끝나도 남는 진행(영혼석·기록)을 화면용 값으로 가공 */
UCLASS(BlueprintType, Blueprintable)
class UJY_ProgressViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	/* PlayerState 진행 변화 구독 + 현재 값으로 한 번 갱신 (맵 이동 후 새 PlayerState면 갈아탐) */
	void BindPlayerState(AJY_PlayerState* InPlayerState);

	/* PlayerState 구독 해제 */
	void UnbindPlayerState();

	FText GetSoulStonesText() const { return SoulStonesText; }
	FText GetRunCountText() const { return RunCountText; }
	FText GetClearCountText() const { return ClearCountText; }

private:
	/* PlayerState 진행 값으로 화면용 값 3개를 다시 계산 */
	void RefreshProgress();

private:
	/* 델리게이트 */

	FDelegateHandle ProgressChangedHandle;

private:
	/*
	 * 위젯이 바인딩하는 값 (FieldNotify)
	 * WBP_JY_HUD SoulText가 SoulStonesText 사용. 도전·클리어 횟수는 기록판 화면(이후 장)용
	 */

	/* 영혼석 수 */
	UPROPERTY(BlueprintReadOnly, Transient, FieldNotify, Getter, Category = "JY|Progress", meta = (AllowPrivateAccess))
	FText SoulStonesText;

	/* 도전 횟수 */
	UPROPERTY(BlueprintReadOnly, Transient, FieldNotify, Getter, Category = "JY|Progress", meta = (AllowPrivateAccess))
	FText RunCountText;

	/* 클리어 횟수 */
	UPROPERTY(BlueprintReadOnly, Transient, FieldNotify, Getter, Category = "JY|Progress", meta = (AllowPrivateAccess))
	FText ClearCountText;

private:
	/* 구독 중인 PlayerState — 맵 이동 때 바뀌므로 약한 참조 */
	TWeakObjectPtr<AJY_PlayerState> PlayerState;

	/* 마지막으로 글자를 만든 값 (-1 = 아직 없음) — 숫자가 바뀔 때만 글자를 새로 만든다 (5-7과 같은 이유) */
	int32 DisplayedSoulStones = INDEX_NONE;

	int32 DisplayedRunCount = INDEX_NONE;

	int32 DisplayedClearCount = INDEX_NONE;
};
