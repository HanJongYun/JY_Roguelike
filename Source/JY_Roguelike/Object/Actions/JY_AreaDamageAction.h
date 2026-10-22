#pragma once

#include "CoreMinimal.h"
#include "Object/JY_Action.h"
#include "GameplayTagContainer.h"
#include "JY_AreaDamageAction.generated.h"

/* 범위 안 모두(플레이어·적, 팀 무관)에게 피해 — 한 번(폭발) 또는 상태에 있는 동안 반복(가시·화염·독구름·도끼) */
UCLASS(meta = (DisplayName = "Area Damage"))
class UJY_AreaDamageAction : public UJY_Action
{
	GENERATED_BODY()

public:
	virtual void OnServerEnter(AJY_Object* Object) override;
	virtual void OnServerExit(AJY_Object* Object) override;

private:
	// [Server]
	void ApplyDamage();

	/* [Server] 범위 안 부서지는 물체(IJY_HitReactable)를 중심 바깥으로 밀며 때림 */
	void HitObjects(const FVector& Center) const;

	FVector GetCenter() const;

	/* 물체 가로 배율을 곱한 반지름 */
	float GetScaledRadius() const;

private:
	/* 설정값 */

	/* 한 번 피해 */
	UPROPERTY(EditAnywhere, Category = "JY|AreaDamage", Meta = (ClampMin = "0.0"))
	float Damage = 10.0f;

	/* 반지름 (cm, 물체 배율 1 기준) — 배치한 물체를 키우거나 줄이면 함께 바뀐다 */
	UPROPERTY(EditAnywhere, Category = "JY|AreaDamage", Meta = (ClampMin = "0.0"))
	float Radius = 150.0f;

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
};
