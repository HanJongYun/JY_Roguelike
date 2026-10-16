#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "JY_HitReactable.generated.h"

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UJY_HitReactable : public UInterface
{
	GENERATED_BODY()
};

/*
 * 공격·폭발에 맞는 쪽 — 전투(Combat)가 맞은 대상의 클래스를 모른 채 부른다.
 * 때리는 쪽은 여기까지만 부르고, 맞은 뒤 일(부서짐·넉백·사망)은 구현한 쪽이 정한다.
 * 액터나 컴포넌트가 구현한다 — 찾을 때는 JY_FindInterface (액터 Cast → FindComponentByInterface, Actor.h:3852)
 */
class IJY_HitReactable
{
	GENERATED_BODY()

public:
	/* 지금 맞을 수 있나 (예: 이미 부서진 항아리는 false) */
	virtual bool CanReceiveHit() const = 0;

	/* [Server] 맞음. HitVelocity = 맞은 방향 × 세기 (cm/s, 공격의 밀어내기 속도) */
	virtual void ReceiveHit(AActor* InstigatorActor, float Damage, const FVector& HitVelocity) = 0;
};
