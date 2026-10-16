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

	float Damage = 0.0f;

	/* 밀어내는 속도 (cm/s, 수평) */
	FVector2f KnockbackImpulse = FVector2f::ZeroVector;

	/* 피격 연출 큐 (비우면 연출 없음) */
	FGameplayTag HitCueTag;

	/* 때린 쪽 (플레이어 캐릭터) — 피해 GE 출처 ASC를 여기서 찾는다 */
	TWeakObjectPtr<AActor> Instigator;
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
