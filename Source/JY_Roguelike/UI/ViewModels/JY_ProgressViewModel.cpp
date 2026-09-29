#include "UI/ViewModels/JY_ProgressViewModel.h"
#include "UI/ViewModels/JY_MetaEntryViewModel.h"
#include "Player/JY_PlayerState.h"
#include "Progress/JY_MetaUpgradeData.h"
#include "System/JY_AssetManager.h"
#include "System/JY_GameData.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_ProgressViewModel)

// [OwnClient]
void UJY_ProgressViewModel::BeginDestroy() //override
{
	UnbindPlayerState();

	Super::BeginDestroy();
}

/* 값의 출처는 PlayerState 복제 값 하나 — 클라가 숫자를 직접 바꾸지 않는다 (구매는 서버 → 복제 → 여기) */
// [OwnClient]
void UJY_ProgressViewModel::BindPlayerState(AJY_PlayerState* InPlayerState)
{
	if (InPlayerState == PlayerState.Get())
		return;

	UnbindPlayerState();

	PlayerState = InPlayerState;
	if (InPlayerState == nullptr)
		return;

	ProgressChangedHandle = InPlayerState->OnProgressChanged.AddUObject(this, &UJY_ProgressViewModel::RefreshProgress);
	RefreshProgress();
}

// [OwnClient]
void UJY_ProgressViewModel::UnbindPlayerState()
{
	if (PlayerState.IsValid() == true)
	{
		PlayerState->OnProgressChanged.Remove(ProgressChangedHandle);
	}

	ProgressChangedHandle.Reset();
	PlayerState.Reset();
}

/*
 * 항목 ViewModel은 강화 표 크기만큼 처음 1번만 만들고 이후엔 값만 갱신 — 제단 화면이 매번 새로 만들지 않는다.
 * 표는 GameData(클라에도 로드됨)의 DA_JY_MetaUpgrades.
 */
// [OwnClient]
void UJY_ProgressViewModel::RefreshProgress()
{
	if (PlayerState.IsValid() == false)
		return;

	const FJY_PlayerProgress& Progress = PlayerState->GetProgress();

	UE_MVVM_SET_PROPERTY_VALUE(SoulStonesText, FText::AsNumber(Progress.SoulStones));
	UE_MVVM_SET_PROPERTY_VALUE(RunCountText, FText::AsNumber(Progress.RunCount));
	UE_MVVM_SET_PROPERTY_VALUE(ClearCountText, FText::AsNumber(Progress.ClearCount));

	const UJY_GameData* GameData = UJY_AssetManager::Get().GetGameData();
	const UJY_MetaUpgradeData* MetaData = GameData != nullptr ? GameData->MetaUpgradeData.Get() : nullptr;
	if (MetaData == nullptr)
		return;

	const TArray<FJY_MetaUpgradeDefinition>& Definitions = MetaData->GetUpgrades();
	while (MetaEntries.Num() < Definitions.Num())
	{
		MetaEntries.Add(NewObject<UJY_MetaEntryViewModel>(this));
	}

	for (int32 Index = 0; Index < Definitions.Num(); ++Index)
	{
		const FJY_MetaUpgradeDefinition& Definition = Definitions[Index];
		MetaEntries[Index]->Refresh(Definition, Progress.GetMetaRank(Definition.UpgradeTag), Progress.SoulStones);
	}
}
