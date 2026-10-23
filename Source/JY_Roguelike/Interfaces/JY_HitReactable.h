#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "JY_HitReactable.generated.h"

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UJY_HitReactable : public UInterface
{
	GENERATED_BODY()
};

/* 공격·폭발에 맞는 쪽 — 액터나 컴포넌트가 구현하고 JY_FindInterface로 찾는다 */
class IJY_HitReactable
{
	GENERATED_BODY()

public:
	/* 지금 맞을 수 있나 (예: 이미 부서진 항아리는 false) */
	virtual bool CanReceiveHit() const = 0;

	/* [Server] 맞음. HitVelocity = 맞은 방향 × 세기 (cm/s, 공격의 밀어내기 속도) */
	virtual void ReceiveHit(AActor* InstigatorActor, float Damage, const FVector& HitVelocity) = 0;
};
