#include "UI/ViewModels/JY_MetaUpgradeViewModel.h"
#include "UI/ViewModels/JY_MetaEntryViewModel.h"
#include "Player/JY_PlayerState.h"
#include "Progress/JY_MetaUpgradeData.h"
#include "System/JY_AssetManager.h"
#include "System/JY_GameData.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_MetaUpgradeViewModel)

/*
 * 영혼석·기록(UJY_ProgressViewModel)과 같은 진행 값을 읽지만 대상이 다른 데이터 묶음이라 따로 둔다 (Epic: 작은 ViewModel 여러 개).
 * 값의 출처는 PlayerState 복제 값 하나 — 서버 CommitProgress → 복제 → OnRep_Progress → OnProgressChanged 방송.
 */
// [OwnClient]
void UJY_MetaUpgradeViewModel::BindPlayerState(AJY_PlayerState* InPlayerState)
{
	/* 두 번 불리면 구독이 두 겹이 되므로 먼저 끊고 다시 건다 */
	UnbindPlayerState();

	PlayerState = InPlayerState;
	if (InPlayerState == nullptr)
		return;

	ProgressChangedHandle = InPlayerState->OnProgressChanged.AddUObject(this, &ThisClass::RefreshEntries);

	/* 구독은 "다음 변화"부터라 지금 값은 직접 한 번 채운다 */
	RefreshEntries();
}

// [OwnClient]
void UJY_MetaUpgradeViewModel::UnbindPlayerState()
{
	AJY_PlayerState* OldPlayerState = PlayerState.Get();
	if (OldPlayerState != nullptr)
	{
		OldPlayerState->OnProgressChanged.Remove(ProgressChangedHandle);
	}

	ProgressChangedHandle.Reset();
	PlayerState.Reset();
}

/*
 * 표는 GameData의 DA_JY_MetaUpgrades — 서버 검증과 같은 표라 화면 가격과 실제 가격이 어긋나지 않는다.
 * 줄 ViewModel은 표 크기만큼 처음 1번만 만들고, 이후엔 값만 갱신 — 줄 위젯은 같은 ViewModel을 계속 본다.
 */
// [OwnClient]
void UJY_MetaUpgradeViewModel::RefreshEntries()
{
	const AJY_PlayerState* JYPlayerState = PlayerState.Get();
	if (JYPlayerState == nullptr)
		return;

	const UJY_GameData* GameData = UJY_AssetManager::Get().GetGameData();
	const UJY_MetaUpgradeData* MetaData = GameData != nullptr ? GameData->MetaUpgradeData.Get() : nullptr;
	if (MetaData == nullptr)
		return;

	const TArray<FJY_MetaUpgradeDefinition>& Definitions = MetaData->GetUpgrades();
	if (Entries.IsEmpty() == true)
	{
		for (int32 Index = 0; Index < Definitions.Num(); ++Index)
		{
			Entries.Add(NewObject<UJY_MetaEntryViewModel>(this));
		}
	}

	const FJY_PlayerProgress& Progress = JYPlayerState->GetProgress();
	for (int32 Index = 0; Index < Definitions.Num(); ++Index)
	{
		const FJY_MetaUpgradeDefinition& Definition = Definitions[Index];
		const int32 CurrentRank = Progress.GetMetaRank(Definition.UpgradeTag);
		Entries[Index]->Refresh(*MetaData, Definition, CurrentRank, Progress.SoulStones);
	}
}
