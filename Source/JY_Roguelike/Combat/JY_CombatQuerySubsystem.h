#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Combat/JY_CombatTypes.h"
#include "JY_CombatQuerySubsystem.generated.h"

struct FCollisionObjectQueryParams;

class UJY_AbilitySystemComponent;

/* 전투 판정 창구 — 모양 안 대상 찾기·피해 적용 */
UCLASS()
class UJY_CombatQuerySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

public:
	/* 모양 안 액터 모으기 (팀 무관) — 주어진 채널 후보 중 높이 허용치·2D 모양에 걸치는 것 */
	void FindActorsInShape(const FJY_HitShape& Shape, const FTransform& Origin, const AActor* IgnoreActor, const FCollisionObjectQueryParams& ObjectQueryParams, TArray<AActor*>& OutActors) const;

	/* 모양 안의 적대 대상 모으기. Origin = 공격자 위치 + 수평 방향 */
	void FindTargets(const FJY_HitShape& Shape, const FTransform& Origin, const AActor* Attacker, TArray<FJY_CombatTarget>& OutTargets) const;

	/* 대상 하나에 피해·밀어내기·연출 (서버만). 맞으면 발동하는 물체(IJY_HitReactable)면 그쪽에 넘긴다 */
	void ApplyHit(const FJY_CombatTarget& Target, const FJY_HitPayload& Payload) const;

	/* 모양 안의 팀 무관 대상 모으기 — 폭발·독구름·함정. Pawn 채널 Actor + Mass 적 */
	void FindEnvironmentTargets(const FJY_HitShape& Shape, const FTransform& Origin, const AActor* IgnoreActor, TArray<FJY_CombatTarget>& OutTargets) const;

	/* 대상 하나에 팀 무관 피해 — 폭발·독구름·함정 (서버만). Origin = 피해 중심. 체력이 없거나 죽는 중·무적이면 false */
	bool ApplyEnvironmentDamage(const FJY_CombatTarget& Target, const FJY_HitPayload& Payload, const FVector& Origin) const;

	/* 판정 모양 그리기 (콘솔 jy.Combat.DrawHitShapes 1) */
	static void DrawHitShape(const UWorld* World, const FJY_HitShape& Shape, const FTransform& Origin, bool bHitAny, float DrawSeconds = 1.0f);

	/* 반지름 TargetRadius인 대상이 수평 2D 모양에 걸치는지 (높이는 안 봄) */
	static bool IsInsideShape2D(const FJY_HitShape& Shape, const FVector& Center, const FVector& Forward, const FVector& TargetLocation, float TargetRadius);

	/* 모양 중심에서 모양 끝까지 가장 먼 수평 거리 — 후보를 넓게 모을 때의 반지름 */
	static float GetShapeReachRadius(const FJY_HitShape& Shape);

private:
	/* 모양 안의 Mass 적을 대상 목록에 더한다 (팀 무관) */
	void AppendMassTargetsInShape(const FJY_HitShape& Shape, const FTransform& Origin, TArray<FJY_CombatTarget>& OutTargets) const;

	/* [Server] Mass 적 1마리의 최종 피해를 계산해 피해 목록에 쌓는다 */
	void AddMassEnemyHit(const FJY_CombatTarget& Target, const FJY_HitPayload& Payload) const;

	/* [Server] 맞는 순간 최종 피해 — 공격자 전투 속성을 모아 피해 공식에 넣는다 */
	FJY_DamageResult ComputeDamage(const FJY_HitPayload& Payload) const;

	/* 피해 공식 — 공격력 × 행동 배율 × (1 + 증가분) × (1 − 방어율) × (1 − 저항) × 치명 */
	static FJY_DamageResult CalculateDamage(const FJY_DamageInput& Input);

	/* [Server] 체력 있는 대상에 피해 GE·밀어내기·피격 큐. Origin = 때린 위치 (맞은 방향 계산). 죽는 중·무적이면 false */
	bool ApplyHitToCharacter(AActor* TargetActor, UJY_AbilitySystemComponent* TargetASC, UJY_AbilitySystemComponent* SourceASC, const FJY_HitPayload& Payload, float Damage, const FVector& Origin) const;
};
