#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "GameplayTagContainer.h"
#include "JY_MetaEntryViewModel.generated.h"

struct FJY_MetaUpgradeDefinition;

/* 제단 목록의 항목 1개(메타 강화 1종) 표시 값 — 항목마다 인스턴스 1개 */
UCLASS(BlueprintType)
class UJY_MetaEntryViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	/* 규칙·현재 단계·보유 영혼석으로 표시 값 다시 계산 */
	void Refresh(const FJY_MetaUpgradeDefinition& Definition, int32 CurrentRank, int32 SoulStones);

	const FGameplayTag& GetUpgradeTag() const { return UpgradeTag; }
	FText GetDisplayName() const { return DisplayName; }
	FText GetDescription() const { return Description; }
	FText GetRankText() const { return RankText; }
	FText GetCostText() const { return CostText; }
	bool CanBuy() const { return bCanBuy; }

private:
	/* 구매 요청에 실어 보낼 강화 종류 (표시 안 함) */
	FGameplayTag UpgradeTag;

private:
	/* 위젯이 바인딩하는 값 (WBP_JY_MetaEntry의 View Bindings) */

	/* 이름 (예: 강인함) */
	UPROPERTY(BlueprintReadOnly, Transient, FieldNotify, Getter, Category = "JY|Meta", meta = (AllowPrivateAccess))
	FText DisplayName;

	/* 설명 */
	UPROPERTY(BlueprintReadOnly, Transient, FieldNotify, Getter, Category = "JY|Meta", meta = (AllowPrivateAccess))
	FText Description;

	/* "2 / 5" */
	UPROPERTY(BlueprintReadOnly, Transient, FieldNotify, Getter, Category = "JY|Meta", meta = (AllowPrivateAccess))
	FText RankText;

	/* 다음 단계 가격, 최대면 "최대" */
	UPROPERTY(BlueprintReadOnly, Transient, FieldNotify, Getter, Category = "JY|Meta", meta = (AllowPrivateAccess))
	FText CostText;

	/* 지금 살 수 있나 (최대 아님 + 영혼석 충분) — 구매 버튼 활성 */
	UPROPERTY(BlueprintReadOnly, Transient, FieldNotify, Getter = "CanBuy", Category = "JY|Meta", meta = (AllowPrivateAccess))
	bool bCanBuy = false;
};
