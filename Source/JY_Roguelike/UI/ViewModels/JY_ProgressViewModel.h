#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "JY_ProgressViewModel.generated.h"

class AJY_PlayerState;
class UJY_MetaEntryViewModel;

/* 런이 끝나도 남는 진행(영혼석·기록·메타 강화)을 화면용 값으로 가공 */
UCLASS(BlueprintType, Blueprintable)
class UJY_ProgressViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

protected:
	virtual void BeginDestroy() override;

public:
	/* PlayerState 진행 변화 구독 + 현재 값으로 한 번 갱신 (맵 이동 후 새 PlayerState면 갈아탐) */
	void BindPlayerState(AJY_PlayerState* InPlayerState);

	/* 제단 항목 ViewModel들 (메타 강화 표 순서, 처음 연결 때 1회 생성) */
	const TArray<TObjectPtr<UJY_MetaEntryViewModel>>& GetMetaEntries() const { return MetaEntries; }

	FText GetSoulStonesText() const { return SoulStonesText; }
	FText GetRunCountText() const { return RunCountText; }
	FText GetClearCountText() const { return ClearCountText; }

private:
	/* 진행 값으로 표시 값·항목 전부 갱신 */
	void RefreshProgress();

	/* 구독 해제 */
	void UnbindPlayerState();

private:
	/* 위젯이 바인딩하는 값 — WBP_JY_HUD(영혼석), WBP_JY_RecordBoard(전부) */

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
	UPROPERTY(Transient)
	TArray<TObjectPtr<UJY_MetaEntryViewModel>> MetaEntries;

	TWeakObjectPtr<AJY_PlayerState> PlayerState;

	FDelegateHandle ProgressChangedHandle;
};
