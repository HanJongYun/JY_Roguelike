#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Skill/JY_SkillTypes.h"
#include "JY_SkillUpgradeCard.generated.h"

/* 강화 카드 1장 (DA_JY_Card_*) — 스킬이 이미 있을 때 다시 얻으면 3장 중 1장 고름 */
UCLASS(BlueprintType, Const)
class UJY_SkillUpgradeCard : public UDataAsset
{
	GENERATED_BODY()

public:
	/* 설정값 — 카드 데이터 에셋 */

	UPROPERTY(EditDefaultsOnly, Category = "JY|Skill")
	EJY_SkillUpgradeKind Kind = EJY_SkillUpgradeKind::DamagePercent;

	/* 바꾸는 양 (%) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Skill", Meta = (ClampMin = "0"))
	float Percent = 20.0f;

	/* 카드 문구 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Skill")
	FText DisplayText;
};
