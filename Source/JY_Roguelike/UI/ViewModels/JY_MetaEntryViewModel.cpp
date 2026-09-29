#include "UI/ViewModels/JY_MetaEntryViewModel.h"
#include "Progress/JY_MetaUpgradeData.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_MetaEntryViewModel)

/* UE_MVVM_SET_PROPERTY_VALUE = 값이 실제로 바뀔 때만 위젯에 알림 → 같은 값으로 다시 불려도 위젯 갱신 없음 */
// [OwnClient]
void UJY_MetaEntryViewModel::Refresh(const FJY_MetaUpgradeDefinition& Definition, int32 CurrentRank, int32 SoulStones)
{
	UpgradeTag = Definition.UpgradeTag;

	const int32 MaxRank = Definition.GetMaxRank();
	const bool bMaxed = CurrentRank >= MaxRank;
	const int32 NextCost = bMaxed == false ? Definition.RankCosts[CurrentRank] : 0;

	UE_MVVM_SET_PROPERTY_VALUE(DisplayName, Definition.DisplayName);
	UE_MVVM_SET_PROPERTY_VALUE(Description, Definition.Description);
	UE_MVVM_SET_PROPERTY_VALUE(RankText, FText::FromString(FString::Printf(TEXT("%d / %d"), CurrentRank, MaxRank)));
	UE_MVVM_SET_PROPERTY_VALUE(CostText, bMaxed == true ? FText::FromString(TEXT("최대")) : FText::AsNumber(NextCost));
	UE_MVVM_SET_PROPERTY_VALUE(bCanBuy, bMaxed == false && SoulStones >= NextCost);
}
