#include "UI/ViewModels/JY_ProgressViewModel.h"
#include "Player/JY_PlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_ProgressViewModel)

/*
 * 1장 HUD(UJY_HUDLayout::BindProgress)에 있던 구독을 옮겨 왔다.
 * 값의 출처는 PlayerState 복제 값 하나 — 서버 CommitProgress → 복제 → OnRep_Progress → OnProgressChanged 방송.
 */
// [OwnClient]
void UJY_ProgressViewModel::BindPlayerState(AJY_PlayerState* InPlayerState)
{
	/* 두 번 불리면 구독이 두 겹이 되므로 먼저 끊고 다시 건다 */
	UnbindPlayerState();

	PlayerState = InPlayerState;
	if (InPlayerState == nullptr)
		return;

	ProgressChangedHandle = InPlayerState->OnProgressChanged.AddUObject(this, &ThisClass::RefreshProgress);

	/* 구독은 "다음 변화"부터라 지금 값은 직접 한 번 채운다 */
	RefreshProgress();
}

// [OwnClient]
void UJY_ProgressViewModel::UnbindPlayerState()
{
	AJY_PlayerState* OldPlayerState = PlayerState.Get();
	if (OldPlayerState != nullptr)
	{
		OldPlayerState->OnProgressChanged.Remove(ProgressChangedHandle);
	}

	ProgressChangedHandle.Reset();
	PlayerState.Reset();
}

/* FText는 새로 만들면 항상 "다른 값"이라(5-7) 숫자로 먼저 비교하고 바뀐 것만 글자를 만든다 */
// [OwnClient]
void UJY_ProgressViewModel::RefreshProgress()
{
	const AJY_PlayerState* JYPlayerState = PlayerState.Get();
	if (JYPlayerState == nullptr)
		return;

	const FJY_PlayerProgress& Progress = JYPlayerState->GetProgress();

	if (Progress.SoulStones != DisplayedSoulStones)
	{
		DisplayedSoulStones = Progress.SoulStones;
		UE_MVVM_SET_PROPERTY_VALUE(SoulStonesText, FText::AsNumber(Progress.SoulStones));
	}

	if (Progress.RunCount != DisplayedRunCount)
	{
		DisplayedRunCount = Progress.RunCount;
		UE_MVVM_SET_PROPERTY_VALUE(RunCountText, FText::AsNumber(Progress.RunCount));
	}

	if (Progress.ClearCount != DisplayedClearCount)
	{
		DisplayedClearCount = Progress.ClearCount;
		UE_MVVM_SET_PROPERTY_VALUE(ClearCountText, FText::AsNumber(Progress.ClearCount));
	}
}
