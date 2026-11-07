#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "JY_SkillData.generated.h"

class UJY_GameplayAbility_Skill;
class UGameplayEffect;
class UTexture2D;

/* 스킬 1종 — GA 밖(스킬 컴포넌트·쿨타임·피해·HUD)에서도 읽는 값 */
USTRUCT(BlueprintType)
struct FJY_SkillEntry
{
	GENERATED_BODY()

	/* 스킬 식별 (Skill.Plunge 등) — 지급할 때 스펙에 붙여 GA가 이 칸을 찾는다 */
	UPROPERTY(EditDefaultsOnly, Meta = (Categories = "Skill"))
	FGameplayTag SkillTag;

	/* 지급할 GA (GA_JY_Skill_*) */
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UJY_GameplayAbility_Skill> AbilityClass;

	/* 쿨타임 (초) */
	UPROPERTY(EditDefaultsOnly, Meta = (ClampMin = "0"))
	float Cooldown = 6.0f;

	/* 쿨타임 중 붙는 태그 (Cooldown.Skill.Plunge 등) */
	UPROPERTY(EditDefaultsOnly, Meta = (Categories = "Cooldown"))
	FGameplayTagContainer CooldownTags;

	/* 타격 1회 피해 */
	UPROPERTY(EditDefaultsOnly, Meta = (ClampMin = "0"))
	float Damage = 30.0f;

	UPROPERTY(EditDefaultsOnly)
	FText DisplayName;

	/* HUD 슬롯 아이콘 (Soft — 서버에서 로드 안 함) */
	UPROPERTY(EditDefaultsOnly)
	TSoftObjectPtr<UTexture2D> Icon;
};

/* 슬롯과 별개로 항상 가진 스킬과 그 입력 (우클릭 찌르기 등) */
USTRUCT(BlueprintType)
struct FJY_FixedSkill
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Meta = (Categories = "Skill"))
	FGameplayTag SkillTag;

	UPROPERTY(EditDefaultsOnly, Meta = (Categories = "InputTag"))
	FGameplayTag InputTag;
};

/* 모든 스킬 정보와 시작 배치 (DA_JY_SkillData) */
UCLASS(BlueprintType, Const)
class UJY_SkillData : public UDataAsset
{
	GENERATED_BODY()

public:
	/* 태그로 스킬 칸 찾기. 없으면 nullptr */
	const FJY_SkillEntry* FindSkill(const FGameplayTag& SkillTag) const;

public:
	/* 설정값 — DA_JY_SkillData */

	/* 스킬 목록 */
	UPROPERTY(EditDefaultsOnly, Category = "Skill", Meta = (TitleProperty = "SkillTag"))
	TArray<FJY_SkillEntry> Skills;

	/* 시작 때 슬롯 순서대로 넣을 스킬 */
	UPROPERTY(EditDefaultsOnly, Category = "Skill", Meta = (Categories = "Skill"))
	TArray<FGameplayTag> StartSlotSkills;

	/* 슬롯 번호 → 입력 태그 (InputTag.Skill.Slot1~). 개수 = 최대 슬롯 수 */
	UPROPERTY(EditDefaultsOnly, Category = "Skill", Meta = (Categories = "InputTag.Skill"))
	TArray<FGameplayTag> SlotInputTags;

	/* 슬롯과 별개로 항상 가진 스킬 (우클릭 찌르기) */
	UPROPERTY(EditDefaultsOnly, Category = "Skill")
	TArray<FJY_FixedSkill> FixedSkills;

	/* 스킬 쿨타임 GE (GE_JY_SkillCooldown) — 길이는 SetByCaller.Cooldown, 태그는 스킬 GA가 스펙에 넣음 */
	UPROPERTY(EditDefaultsOnly, Category = "Skill")
	TSubclassOf<UGameplayEffect> CooldownGameplayEffect;
};
