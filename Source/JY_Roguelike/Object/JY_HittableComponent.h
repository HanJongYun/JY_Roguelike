#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interfaces/JY_HitReactable.h"
#include "JY_HittableComponent.generated.h"

/* 맞으면 발동 — 공격·폭발에 맞으면 물체를 발동시킨다 (항아리·상자·통·폭발 통·독 항아리). 전투 판정은 WorldDynamic 몸체를 찾는다 */
UCLASS(ClassGroup = (JY), meta = (BlueprintSpawnableComponent))
class UJY_HittableComponent : public UActorComponent, public IJY_HitReactable
{
	GENERATED_BODY()

public:
	UJY_HittableComponent();

public:
	//~IJY_HitReactable
	virtual bool CanReceiveHit() const override;
	virtual void ReceiveHit(AActor* InstigatorActor, float Damage, const FVector& HitVelocity) override;
	//~End of IJY_HitReactable

private:
	/* [Server] 발동 세기(파편을 미는 속도) — 약한 타격(밀어내기 0)도 때린 쪽 → 물체 방향으로 최소 세기 */
	FVector ComputePushVelocity(const AActor* InstigatorActor, const FVector& HitVelocity) const;

private:
	/* 설정값 — BP 컴포넌트 디테일에서 수정 */

	/* 맞은 공격의 밀어내기 속도 중 발동 세기로 넘기는 비율 — 세게 칠수록 파편이 멀리 */
	UPROPERTY(EditAnywhere, Category = "JY|Hittable", Meta = (ClampMin = "0.0"))
	float PushScale = 0.5f;

	/* 최소 발동 세기 (cm/s) — 밀어내기가 없거나 약한 공격. 0이면 맞은 방향과 무관 (폭발 통은 안에서 터짐) */
	UPROPERTY(EditAnywhere, Category = "JY|Hittable", Meta = (ClampMin = "0.0"))
	float MinPushSpeed = 200.0f;
};
