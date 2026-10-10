#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "JY_MetaUpgradeData.generated.h"

class UGameplayEffect;
class UTexture2D;

/* 메타 강화 한 종류의 규칙 */
USTRUCT(BlueprintType)
struct FJY_MetaUpgradeDefinition
{
	GENERATED_BODY()

	/* 강화 종류 */
	UPROPERTY(EditDefaultsOnly, meta = (Categories = "Meta"))
	FGameplayTag UpgradeTag;

	/* 제단에 보일 이름 (예: 강인함) */
	UPROPERTY(EditDefaultsOnly)
	FText DisplayName;

	/* 제단에 보일 설명 (예: 최대 체력 +20 / 단계) */
	UPROPERTY(EditDefaultsOnly)
	FText Description;

	/* 제단 줄 앞 아이콘 (Soft — 서버는 텍스처를 로드하지 않는다) */
	UPROPERTY(EditDefaultsOnly)
	TSoftObjectPtr<UTexture2D> Icon;

	/* 단계별 가격 — 개수 = 최대 단계 (예: 30, 60, 100 → 최대 3단계) */
	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0"))
	TArray<int32> RankCosts;

	/* 서버가 적용할 효과. 비우면 단계만 저장되고 효과 없음 (지금 효과가 있는 건 강인함 1종, 나머지는 쓰는 곳이 생길 때 연결) */
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> Effect;

	/* 단계당 효과 크기 — 효과의 SetByCaller.MetaValue로 넘김 (예: 20 → 3단계면 60) */
	UPROPERTY(EditDefaultsOnly)
	float ValuePerRank = 0.0f;

	int32 GetMaxRank() const { return RankCosts.Num(); }
};

/* 메타 강화 목록 한 벌 (에셋 DA_JY_MetaUpgrades) — 서버 검증과 제단 화면이 같은 표를 쓴다 */
UCLASS(BlueprintType, Const)
class UJY_MetaUpgradeData : public UDataAsset
{
	GENERATED_BODY()

public:
	/* 종류로 규칙 찾기 (없으면 nullptr) */
	const FJY_MetaUpgradeDefinition* FindUpgrade(const FGameplayTag& UpgradeTag) const
	{
		auto HasSameTag = [&UpgradeTag](const FJY_MetaUpgradeDefinition& Definition) { return Definition.UpgradeTag == UpgradeTag; };
		return Upgrades.FindByPredicate(HasSameTag);
	}

	const TArray<FJY_MetaUpgradeDefinition>& GetUpgrades() const { return Upgrades; }
	const FText& GetRankFormat() const { return RankFormat; }
	const FText& GetMaxedCostText() const { return MaxedCostText; }
	const FText& GetPurchaseToastFormat() const { return PurchaseToastFormat; }

private:
	/* 설정값 — DA_JY_MetaUpgrades 에셋에서 수정. 항목 추가 = 행 1개 + Config/DefaultGameplayTags.ini에 Meta.* 태그 1줄 (코드 수정 없음) */

	/* 강화 목록 (제단에 이 순서로 나열) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Meta")
	TArray<FJY_MetaUpgradeDefinition> Upgrades;

	/* 제단 단계 표시 형식 — {0} 현재 단계, {1} 최대 단계 (예: {0} / {1}) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Meta")
	FText RankFormat;

	/* 최대 단계일 때 가격 자리에 보일 글자 (예: 최대) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Meta")
	FText MaxedCostText;

	/* 구매 성공 토스트 형식 — {0} 이름, {1} 새 단계 (예: {0} {1}단계) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Meta")
	FText PurchaseToastFormat;
};
