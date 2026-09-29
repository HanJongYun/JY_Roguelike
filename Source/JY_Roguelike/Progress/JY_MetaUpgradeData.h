#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "JY_MetaUpgradeData.generated.h"

class UGameplayEffect;

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

	/* 단계별 가격 — 개수 = 최대 단계 (예: 30, 60, 100 → 최대 3단계) */
	UPROPERTY(EditDefaultsOnly)
	TArray<int32> RankCosts;

	/* 서버가 적용할 효과 (없으면 데이터만 — 던전 조각에서 연결) */
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
		return Upgrades.FindByPredicate([&UpgradeTag](const FJY_MetaUpgradeDefinition& Definition) { return Definition.UpgradeTag == UpgradeTag; });
	}

	const TArray<FJY_MetaUpgradeDefinition>& GetUpgrades() const { return Upgrades; }

private:
	/* 설정값 — DA_JY_MetaUpgrades 에셋에서 수정. 항목 추가 = 행 1개 + Meta.* 태그 1개 (코드 수정 없음) */

	/* 강화 목록 (제단에 이 순서로 나열) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Meta")
	TArray<FJY_MetaUpgradeDefinition> Upgrades;
};
