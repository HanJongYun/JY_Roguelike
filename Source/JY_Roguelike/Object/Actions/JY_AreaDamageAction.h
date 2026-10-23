#pragma once

#include "CoreMinimal.h"
#include "Object/JY_Action.h"
#include "GameplayTagContainer.h"
#include "Combat/JY_CombatTypes.h"
#include "JY_AreaDamageAction.generated.h"

class UGameplayEffect;

/* 범위 안 모두(플레이어·적, 팀 무관)에게 피해 — 한 번(폭발) 또는 상태에 있는 동안 반복(가시·화염·독구름·도끼) */
UCLASS(meta = (DisplayName = "Area Damage"))
class UJY_AreaDamageAction : public UJY_Action
{
	GENERATED_BODY()

public:
	virtual void Initialize(AJY_Object* Object) override;
	virtual void OnServerEnter(AJY_Object* Object) override;
	virtual void OnServerExit(AJY_Object* Object) override;

private:
	// [Server]
	void ApplyDamage();

	/* [Server] 범위 안 부서지는 물체(IJY_HitReactable)를 중심 바깥으로 밀며 때림 */
	void HitObjects(const FJY_HitShape& Shape, const FTransform& HitOrigin, const FVector& Center) const;

	FVector GetCenter() const;

	/* 물체 가로 배율을 곱한 판정 모양 */
	FJY_HitShape GetScaledHitShape() const;

	/* 판정 기준 — 중심 위치 + 물체가 바라보는 수평 방향 */
	FTransform GetHitOrigin() const;

	/* [Client] 콘솔 jy.Debug.ShowRanges 1이면 범위를 그린다 (JYShowRanges) */
	void DrawDebugRange() const;

private:
	/* 설정값 */

	/* 한 번 피해 */
	UPROPERTY(EditAnywhere, Category = "JY|AreaDamage", Meta = (ClampMin = "0.0"))
	float Damage = 10.0f;

	/* 판정 모양 (물체 배율 1 기준, 가로 배율만큼 커짐) — 공격과 같은 판정. 바닥 함정은 HalfHeight를 작게 해 점프로 피한다 */
	UPROPERTY(EditAnywhere, Category = "JY|AreaDamage")
	FJY_HitShape HitShape;

	/* 피해 간격 (초) — 0이면 상태에 들어갈 때 한 번만 */
	UPROPERTY(EditAnywhere, Category = "JY|AreaDamage", Meta = (ClampMin = "0.0"))
	float IntervalSeconds = 0.0f;

	/* 상태에 있는 동안 대상마다 한 번만 (가시·화염: 발동 한 번에 한 대) — 간격은 늦게 들어온 대상을 잡는 용도 */
	UPROPERTY(EditAnywhere, Category = "JY|AreaDamage", Meta = (EditCondition = "IntervalSeconds > 0"))
	bool bHitEachTargetOnce = false;

	/* 같은 대상을 다시 때리기까지 (초) — 0이면 간격마다 모두 맞음 */
	UPROPERTY(EditAnywhere, Category = "JY|AreaDamage", Meta = (ClampMin = "0.0", EditCondition = "IntervalSeconds > 0 && bHitEachTargetOnce == false"))
	float RehitSeconds = 0.0f;

	/* 맞은 캐릭터 피격 연출 (GCN_JY_Hit_*) — 비우면 연출 없음 */
	UPROPERTY(EditAnywhere, Category = "JY|AreaDamage", Meta = (Categories = "GameplayCue"))
	FGameplayTag HitCueTag;

	/* 범위 안 대상에 거는 상태이상 (GE_JY_Burn·GE_JY_Poison 등 기간형 GE) — 비우면 없음. 다시 걸면 기간만 갱신 */
	UPROPERTY(EditAnywhere, Category = "JY|AreaDamage|Status")
	TSubclassOf<UGameplayEffect> StatusEffect;

	/* 상태이상 한 틱 피해 */
	UPROPERTY(EditAnywhere, Category = "JY|AreaDamage|Status", Meta = (ClampMin = "0.0", EditCondition = "StatusEffect != nullptr"))
	float StatusTickDamage = 5.0f;

	/* 중심 바깥으로 밀어내는 속도 (cm/s) — 0이면 안 밂 */
	UPROPERTY(EditAnywhere, Category = "JY|AreaDamage|Knockback", Meta = (ClampMin = "0.0"))
	float KnockbackSpeed = 0.0f;

	/* 밀려나며 떠오르는 높이 (cm) — 0이면 바닥을 미끄러짐 */
	UPROPERTY(EditAnywhere, Category = "JY|AreaDamage|Knockback", Meta = (ClampMin = "0.0"))
	float KnockbackHeight = 0.0f;

	/* 중심 부품의 Component Tags — 비우면 루트 (도끼는 Blade) */
	UPROPERTY(EditAnywhere, Category = "JY|AreaDamage")
	FName CenterPartTag;

	/* 중심 부품 기준 위치 (부품 배율 포함) */
	UPROPERTY(EditAnywhere, Category = "JY|AreaDamage")
	FVector CenterOffset = FVector::ZeroVector;

	/* 범위 안 부서지는 물체도 때림 (폭발) */
	UPROPERTY(EditAnywhere, Category = "JY|AreaDamage")
	bool bHitObjects = false;

	/* 때린 물체를 중심 바깥으로 미는 속도 (cm/s) — 파편이 튀는 세기 */
	UPROPERTY(EditAnywhere, Category = "JY|AreaDamage", Meta = (ClampMin = "0.0", EditCondition = "bHitObjects"))
	float ObjectPushSpeed = 800.0f;

private:
	TWeakObjectPtr<AJY_Object> OwnerObject;

	/* 대상별 마지막으로 맞은 시각 (bHitEachTargetOnce·RehitSeconds용, 상태에서 나가면 비움) */
	TMap<TWeakObjectPtr<AActor>, double> LastHitTimes;

	FTimerHandle DamageTimerHandle;

	FTimerHandle DebugDrawTimerHandle;
};
