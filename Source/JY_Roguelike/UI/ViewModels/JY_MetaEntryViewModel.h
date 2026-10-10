#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "GameplayTagContainer.h"
#include "JY_MetaEntryViewModel.generated.h"

class UJY_MetaUpgradeData;
class UTexture2D;
struct FJY_MetaUpgradeDefinition;

/* 제단 목록의 강화 한 줄 표시 값 — 줄마다 인스턴스 1개 */
UCLASS(BlueprintType)
class UJY_MetaEntryViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	/* 강화 규칙·현재 단계·보유 영혼석으로 표시 값 갱신 (바뀐 값만 위젯에 알림) */
	void Refresh(const UJY_MetaUpgradeData& MetaUpgradeData, const FJY_MetaUpgradeDefinition& Definition, int32 CurrentRank, int32 SoulStones);

	const FGameplayTag& GetUpgradeTag() const { return UpgradeTag; }
	const TSoftObjectPtr<UTexture2D>& GetIcon() const { return Icon; }

private:
	/* 위젯이 바인딩하는 값 — WBP_JY_MetaEntry의 ViewModel "MetaEntry" (Creation Type = Manual) */

	/* 이름 (예: 강인함) → NameText */
	UPROPERTY(BlueprintReadOnly, Transient, FieldNotify, Category = "JY|Meta", meta = (AllowPrivateAccess))
	FText DisplayName;

	/* 설명 → DescriptionText */
	UPROPERTY(BlueprintReadOnly, Transient, FieldNotify, Category = "JY|Meta", meta = (AllowPrivateAccess))
	FText Description;

	/* "2 / 5" → RankText */
	UPROPERTY(BlueprintReadOnly, Transient, FieldNotify, Category = "JY|Meta", meta = (AllowPrivateAccess))
	FText RankText;

	/* 다음 단계 가격, 최대면 "최대" → CostText */
	UPROPERTY(BlueprintReadOnly, Transient, FieldNotify, Category = "JY|Meta", meta = (AllowPrivateAccess))
	FText CostText;

	/* 지금 살 수 있나 (최대 아님 + 영혼석 충분) → BuyButton 활성 */
	UPROPERTY(BlueprintReadOnly, Transient, FieldNotify, Category = "JY|Meta", meta = (AllowPrivateAccess))
	bool bCanBuy = false;

private:
	/* 구매 요청에 실어 보낼 강화 종류 */
	FGameplayTag UpgradeTag;

	/* 줄 위젯이 처음 한 번 그리는 아이콘 — 바뀌지 않아 바인딩·알림 없음 */
	TSoftObjectPtr<UTexture2D> Icon;

	/* 마지막으로 글자를 만든 단계 (-1 = 아직 없음) */
	int32 DisplayedRank = INDEX_NONE;
};
