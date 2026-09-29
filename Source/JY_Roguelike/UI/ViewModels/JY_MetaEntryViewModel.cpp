#include "UI/ViewModels/JY_MetaEntryViewModel.h"
#include "Progress/JY_MetaUpgradeData.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_MetaEntryViewModel)

/*
 * 글자(FText)는 새로 만들 때마다 "다른 값"으로 비교된다(FText::IdenticalTo는 내부 데이터 포인터 비교, Text.cpp:1455).
 * 그래서 숫자(단계)가 바뀌었을 때만 글자를 새로 만든다 — 영혼석만 바뀌면 bCanBuy 하나만 알림.
 * 이름·설명은 표의 FText를 그대로 복사해 같은 데이터를 가리키므로 두 번째부터는 알림이 없다.
 */
// [OwnClient]
void UJY_MetaEntryViewModel::Refresh(const UJY_MetaUpgradeData& MetaUpgradeData, const FJY_MetaUpgradeDefinition& Definition, int32 CurrentRank, int32 SoulStones)
{
	UpgradeTag = Definition.UpgradeTag;
	Icon = Definition.Icon;
	UE_MVVM_SET_PROPERTY_VALUE(DisplayName, Definition.DisplayName);
	UE_MVVM_SET_PROPERTY_VALUE(Description, Definition.Description);

	const int32 MaxRank = Definition.GetMaxRank();
	const bool bMaxed = CurrentRank >= MaxRank;
	const int32 NextCost = bMaxed == false ? Definition.RankCosts[FMath::Max(CurrentRank, 0)] : 0;

	if (CurrentRank != DisplayedRank)
	{
		DisplayedRank = CurrentRank;
		UE_MVVM_SET_PROPERTY_VALUE(RankText, FText::Format(MetaUpgradeData.GetRankFormat(), CurrentRank, MaxRank));
		UE_MVVM_SET_PROPERTY_VALUE(CostText, bMaxed == true ? MetaUpgradeData.GetMaxedCostText() : FText::AsNumber(NextCost));
	}

	UE_MVVM_SET_PROPERTY_VALUE(bCanBuy, bMaxed == false && SoulStones >= NextCost);
}
