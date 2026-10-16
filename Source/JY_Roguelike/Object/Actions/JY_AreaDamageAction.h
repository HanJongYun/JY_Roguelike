#pragma once

#include "CoreMinimal.h"
#include "Object/JY_Action.h"
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

	/* [Server] 범위 안 부서지는 물체 등(IJY_HitReactable)을 중심 바깥으로 밀며 때림 — 폭발 통 연쇄 */
	void HitObjects(const FVector& Center) const;

	FVector GetCenter() const;

private:
	/* 설정값 */

	/* 한 번 피해 */
	UPROPERTY(EditAnywhere, Category = "JY|AreaDamage", Meta = (ClampMin = "0.0"))
	float Damage = 10.0f;

	/* 반지름 (cm, 물체 배율 무관) */
	UPROPERTY(EditAnywhere, Category = "JY|AreaDamage", Meta = (ClampMin = "0.0"))
	float Radius = 150.0f;

	/* 피해 간격 (초) — 0이면 상태에 들어갈 때 한 번만 */
	UPROPERTY(EditAnywhere, Category = "JY|AreaDamage", Meta = (ClampMin = "0.0"))
	float IntervalSeconds = 0.0f;

	/* 같은 대상을 다시 때리기까지 (초) — 0이면 간격마다 모두 맞음. 도끼 날이 한 번 지나가는 동안 여러 번 맞지 않게 */
	UPROPERTY(EditAnywhere, Category = "JY|AreaDamage", Meta = (ClampMin = "0.0", EditCondition = "IntervalSeconds > 0"))
	float RehitSeconds = 0.0f;

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

	/* 대상별 마지막으로 맞은 시각 (RehitSeconds용) */
	TMap<TWeakObjectPtr<AActor>, double> LastHitTimes;

	FTimerHandle DamageTimerHandle;
};
