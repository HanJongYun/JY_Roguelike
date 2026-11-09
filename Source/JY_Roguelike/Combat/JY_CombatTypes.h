#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "JY_CombatTypes.generated.h"

/* 판정 모양 종류 */
UENUM(BlueprintType)
enum class EJY_HitShapeType : uint8
{
	/* 원 (중심 = 공격자 + Offset) */
	Sphere,
	/* 부채꼴 (공격자 앞 방향 기준 AngleDegrees) */
	Arc,
	/* 앞으로 뻗은 상자 (Length × Width) */
	Box
};

/* 피해 종류 — 계산이 첫 숫자와 붙는 항목(축복 증가분·치명)을 이걸로 가른다 */
UENUM()
enum class EJY_DamageType : uint8
{
	/* 플레이어 타격 — 공격력 × 행동 배율, 증가분·치명 적용 */
	Strike,
	/* 상태이상 틱·축복 특수 효과 — 에셋 수치, 속성 증가분만, 치명 없음 */
	Effect,
	/* 함정·폭발·적 공격 — 고정 피해, 증가분·치명 없음 */
	Fixed
};

/* 팀 번호 — FGenericTeamId 값. 다르면 적대 */
UENUM()
enum class EJY_Team : uint8
{
	Player = 0,
	Enemy = 1
};

/* 한 번의 타격 판정 모양 (수평 2D + 높이 허용치) */
USTRUCT(BlueprintType)
struct FJY_HitShape
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	EJY_HitShapeType Type = EJY_HitShapeType::Arc;

	/* 공격자 기준 앞(X)·오른쪽(Y) 이동 (cm) */
	UPROPERTY(EditDefaultsOnly)
	FVector2D Offset = FVector2D::ZeroVector;

	/* 원·부채꼴 반지름 (cm) */
	UPROPERTY(EditDefaultsOnly, Meta = (EditCondition = "Type != EJY_HitShapeType::Box"))
	float Radius = 220.0f;

	/* 부채꼴 전체 각도 */
	UPROPERTY(EditDefaultsOnly, Meta = (EditCondition = "Type == EJY_HitShapeType::Arc", ClampMax = "360"))
	float AngleDegrees = 140.0f;

	/* 상자 길이(앞) (cm) */
	UPROPERTY(EditDefaultsOnly, Meta = (EditCondition = "Type == EJY_HitShapeType::Box"))
	float Length = 300.0f;

	/* 상자 폭 (cm) */
	UPROPERTY(EditDefaultsOnly, Meta = (EditCondition = "Type == EJY_HitShapeType::Box"))
	float Width = 150.0f;

	/* 위아래 허용 높이 (cm) — 2층 적 오판정 방지 */
	UPROPERTY(EditDefaultsOnly)
	float HalfHeight = 150.0f;
};

/* Actor·Mass 공통 피격 묶음 */
USTRUCT()
struct FJY_HitPayload
{
	GENERATED_BODY()

	EJY_DamageType DamageType = EJY_DamageType::Fixed;

	/* 타격 행동 배율 (만분율, 10000 = 공격력 그대로) — Strike만 */
	int32 ActionMultiplier = 10000;

	/* 첫 피해 숫자 — Effect·Fixed만 */
	float BaseDamage = 0.0f;

	/* 어느 행동의 피해인지 (Skill.Plunge·Skill.BasicAttack 등) — 축복 증가분 조건 */
	FGameplayTag ActionTag;

	/* 피해 속성 (Element.*) — 대상 저항·속성 증가분 조건 */
	FGameplayTag ElementTag;

	/* 밀어내는 속도 (cm/s, 수평) */
	FVector2f KnockbackImpulse = FVector2f::ZeroVector;

	/* 밀려나며 떠오르는 높이 (cm) — 0이면 바닥을 미끄러짐 */
	float KnockbackHeight = 0.0f;

	/* 부서지는 물체를 날리는 방향 (월드, 수평) — 0이면 공격자 → 물체 */
	FVector ObjectPushDirection = FVector::ZeroVector;

	/* 맞은 대상에 거는 상태이상 (Status.Burning 등, 비우면 없음) — 수치는 DA_JY_StatusEffectData */
	FGameplayTag StatusTag;

	/* 피격 연출 큐 (비우면 연출 없음) */
	FGameplayTag HitCueTag;

	/* 때린 쪽 (플레이어 캐릭터) — 피해 GE 출처 ASC를 여기서 찾는다 */
	TWeakObjectPtr<AActor> Instigator;
};

/* 피해 계산 입력 — 비율은 만분율 (10000 = 100%) */
struct FJY_DamageInput
{
	EJY_DamageType DamageType = EJY_DamageType::Fixed;

	/* 공격자 (Strike만) */
	float BaseAttack = 0.0f;
	int32 AttackBonus = 0;
	int32 CritChance = 0;
	int32 CritMultiplier = 10000;

	/* 공격 */
	int32 ActionMultiplier = 10000;
	float BaseDamage = 0.0f;

	/* 맞는 축복 증가분 합 (Strike·Effect만) */
	int32 IncreaseRate = 0;

	/* 대상 */
	int32 DefenseRate = 0;
	int32 ElementResistance = 0;
};

/* 피해 계산 결과 */
struct FJY_DamageResult
{
	float Damage = 0.0f;
	bool bCritical = false;
};

/* 판정에 걸린 대상 하나 */
USTRUCT()
struct FJY_CombatTarget
{
	GENERATED_BODY()

	TWeakObjectPtr<AActor> Actor;

	/* 판정에 쓴 위치 — 이펙트 위치로도 씀 */
	FVector Location = FVector::ZeroVector;
};
