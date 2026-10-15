#include "UI/ViewModels/JY_MetaEntryViewModel.h"
#include "Progress/JY_MetaUpgradeData.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_MetaEntryViewModel)

/*
 * FText는 새로 만들면 항상 "다른 값"이라(5-7) 단계 숫자가 바뀔 때만 설명·단계·가격 글자를 만든다 — 영혼석만 바뀌면 bCanBuy만 알림.
 * 이름은 표의 FText를 그대로 복사해 같은 글자 데이터를 가리키므로 두 번째부터는 알림이 없다.
 */
// [OwnClient]
void UJY_MetaEntryViewModel::Refresh(const UJY_MetaUpgradeData& MetaUpgradeData, const FJY_MetaUpgradeDefinition& Definition, int32 CurrentRank, int32 SoulStones)
{
	UpgradeTag = Definition.UpgradeTag;
	Icon = Definition.Icon;
	UE_MVVM_SET_PROPERTY_VALUE(DisplayName, Definition.DisplayName);

	const int32 MaxRank = Definition.GetMaxRank();
	const bool bMaxed = CurrentRank >= MaxRank;
	const int32 NextCost = bMaxed == false ? Definition.RankCosts[CurrentRank] : 0;

	if (CurrentRank != DisplayedRank)
	{
		DisplayedRank = CurrentRank;

		/* 설명은 "사면 갖게 될 효과" — 0단계면 1단계 효과(+20), 2단계면 3단계 효과(+60). 최대면 지금 효과 */
		const int32 DescribedRank = bMaxed == true ? CurrentRank : CurrentRank + 1;
		const FText EffectValueText = FText::AsNumber(DescribedRank * Definition.ValuePerRank);
		const FText NewDescription = FText::Format(Definition.Description, EffectValueText);
		UE_MVVM_SET_PROPERTY_VALUE(Description, NewDescription);

		const FText NewRankText = FText::Format(MetaUpgradeData.GetRankFormat(), CurrentRank, MaxRank);
		UE_MVVM_SET_PROPERTY_VALUE(RankText, NewRankText);

		const FText NewCostText = bMaxed == true ? MetaUpgradeData.GetMaxedCostText() : FText::AsNumber(NextCost);
		UE_MVVM_SET_PROPERTY_VALUE(CostText, NewCostText);
	}

	UE_MVVM_SET_PROPERTY_VALUE(bCanBuy, bMaxed == false && SoulStones >= NextCost);
}
