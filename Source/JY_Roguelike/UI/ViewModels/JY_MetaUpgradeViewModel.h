#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "JY_MetaUpgradeViewModel.generated.h"

class AJY_PlayerState;
class UJY_MetaEntryViewModel;

/* 메타 강화 목록(강화 표 + 내 단계·영혼석)을 화면용 값으로 가공 — 줄마다 UJY_MetaEntryViewModel 1개를 소유 */
UCLASS(BlueprintType)
class UJY_MetaUpgradeViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	/* PlayerState 진행 변화 구독 + 현재 값으로 한 번 갱신 (맵 이동 후 새 PlayerState면 갈아탐) */
	void BindPlayerState(AJY_PlayerState* InPlayerState);

	/* PlayerState 구독 해제 */
	void UnbindPlayerState();

	/* 줄 ViewModel들 (강화 표 순서) — 제단 화면이 줄 위젯마다 하나씩 넣는다 */
	const TArray<TObjectPtr<UJY_MetaEntryViewModel>>& GetEntries() const { return Entries; }

private:
	/* 강화 표대로 줄 ViewModel 갱신 (처음엔 표 크기만큼 생성) */
	void RefreshEntries();

private:
	/* 델리게이트 */

	FDelegateHandle ProgressChangedHandle;

private:
	/* 줄마다 하나 — 제단 화면이 열릴 때마다 만들지 않고 여기서 들고 값만 갱신 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UJY_MetaEntryViewModel>> Entries;

	/* 구독 중인 PlayerState — 맵 이동 때 바뀌므로 약한 참조 */
	TWeakObjectPtr<AJY_PlayerState> PlayerState;
};
