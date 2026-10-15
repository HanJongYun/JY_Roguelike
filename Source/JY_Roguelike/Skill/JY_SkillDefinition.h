#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Skill/JY_SkillTypes.h"
#include "JY_SkillDefinition.generated.h"

class UJY_GameplayAbility_Skill;
class UJY_SkillUpgradeCard;
class UTexture2D;

/* 스킬 1종 (DA_JY_Skill_*) — 지급할 GA·레벨 배율·카드 풀 */
UCLASS(BlueprintType, Const)
class UJY_SkillDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	/* 레벨(1부터)의 배율. 범위를 넘으면 가까운 끝 레벨 */
	const FJY_SkillLevelData& GetLevelData(int32 Level) const;

	int32 GetMaxLevel() const { return Levels.Num(); }

public:
	/* 설정값 — 스킬 데이터 에셋 */

	/* 스킬 식별 (Skill.Plunge 등) — 디버그 명령이 끝 이름으로 찾음 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Skill", Meta = (Categories = "Skill"))
	FGameplayTag SkillTag;

	UPROPERTY(EditDefaultsOnly, Category = "JY|Skill")
	FText DisplayName;

	/* HUD 슬롯 아이콘 (Soft — 서버에서 로드 안 함) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Skill")
	TSoftObjectPtr<UTexture2D> Icon;

	/* 지급할 GA (GA_JY_Skill_*) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Skill")
	TSubclassOf<UJY_GameplayAbility_Skill> AbilityClass;

	/* 레벨별 배율. 개수 = 최대 레벨 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Skill")
	TArray<FJY_SkillLevelData> Levels;

	/* 이 스킬에서 나올 수 있는 강화 카드 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Skill")
	TArray<TObjectPtr<const UJY_SkillUpgradeCard>> UpgradeCards;
};
