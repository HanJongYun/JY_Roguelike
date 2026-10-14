#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpecHandle.h"
#include "JY_SkillTypes.generated.h"

class UJY_SkillDefinition;
class UJY_SkillUpgradeCard;

/* 강화 카드가 바꾸는 것 */
UENUM(BlueprintType)
enum class EJY_SkillUpgradeKind : uint8
{
	/* 피해 +N% */
	DamagePercent,
	/* 범위(반지름·폭) +N% */
	RadiusPercent,
	/* 쿨타임 -N% */
	CooldownPercent
};

/* 레벨 하나의 배율 */
USTRUCT(BlueprintType)
struct FJY_SkillLevelData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	float DamageScale = 1.0f;

	UPROPERTY(EditDefaultsOnly)
	float RadiusScale = 1.0f;

	/* 쿨타임 (초) */
	UPROPERTY(EditDefaultsOnly, Meta = (ClampMin = "0"))
	float Cooldown = 6.0f;
};

/* 레벨 × 카드를 반영한 최종 수치 — 스킬 GA가 읽는 값 */
USTRUCT()
struct FJY_SkillStats
{
	GENERATED_BODY()

	float DamageScale = 1.0f;

	float RadiusScale = 1.0f;

	float Cooldown = 6.0f;
};

/* 슬롯 한 칸 (소유 클라에 복제) */
USTRUCT()
struct FJY_SkillSlotEntry
{
	GENERATED_BODY()

	/* 비면 빈 슬롯 */
	UPROPERTY()
	TObjectPtr<UJY_SkillDefinition> Skill;

	/* 1부터 */
	UPROPERTY()
	int32 Level = 0;

	/* 고른 강화 카드 */
	UPROPERTY()
	TArray<TObjectPtr<const UJY_SkillUpgradeCard>> Cards;

	/* 서버만 — 지급한 어빌리티 */
	UPROPERTY(NotReplicated)
	FGameplayAbilitySpecHandle GrantedHandle;
};
