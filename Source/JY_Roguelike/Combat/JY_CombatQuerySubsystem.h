#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Combat/JY_CombatTypes.h"
#include "JY_CombatQuerySubsystem.generated.h"

class UJY_AbilitySystemComponent;

/* 전투 판정 창구 — 모양 안 대상 찾기·피해 적용 */
UCLASS()
class UJY_CombatQuerySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

public:
	/* 모양 안의 적대 대상 모으기. Origin = 공격자 위치 + 수평 방향 */
	void FindTargets(const FJY_HitShape& Shape, const FTransform& Origin, const AActor* Attacker, TArray<FJY_CombatTarget>& OutTargets) const;

	/* 대상 하나에 피해·밀어내기·연출 (서버만). 맞으면 발동하는 물체(IJY_HitReactable)면 그쪽에 넘긴다 */
	void ApplyHit(const FJY_CombatTarget& Target, const FJY_HitPayload& Payload) const;

	/* 대상 하나에 팀 무관 피해 — 폭발·독구름·함정 (서버만). Origin = 피해 중심. 체력이 없거나 죽는 중·무적이면 false */
	bool ApplyEnvironmentDamage(AActor* TargetActor, const FJY_HitPayload& Payload, const FVector& Origin) const;

	/* 판정 모양 그리기 (콘솔 jy.Combat.DrawHitShapes 1) */
	static void DrawHitShape(const UWorld* World, const FJY_HitShape& Shape, const FTransform& Origin, bool bHitAny);

private:
	/* [Server] 체력 있는 대상에 피해 GE·밀어내기·피격 큐. Origin = 때린 위치 (맞은 방향 계산). 죽는 중·무적이면 false */
	bool ApplyHitToCharacter(AActor* TargetActor, UJY_AbilitySystemComponent* TargetASC, UJY_AbilitySystemComponent* SourceASC, const FJY_HitPayload& Payload, const FVector& Origin) const;
};
