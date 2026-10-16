#include "Object/JY_HittableComponent.h"
#include "Object/JY_Object.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_HittableComponent)

UJY_HittableComponent::UJY_HittableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UJY_HittableComponent::CanReceiveHit() const //override
{
	const AJY_Object* Object = GetOwner<AJY_Object>();
	return Object != nullptr && Object->CanActivate() == true;
}

/* 피해량은 쓰지 않는다 — 물체는 한 대에 발동 (체력 없음) */
// [Server]
void UJY_HittableComponent::ReceiveHit(AActor* InstigatorActor, float Damage, const FVector& HitVelocity) //override
{
	AJY_Object* Object = GetOwner<AJY_Object>();
	if (Object == nullptr)
		return;

	Object->RequestActivate(InstigatorActor, ComputePushVelocity(InstigatorActor, HitVelocity));
}

/* 세기·최소값은 BP 설정값이라 서버가 최종 속도까지 계산해 보낸다 — 밀어내기 0인 타격의 방향(때린 쪽 위치)은 서버만 안다 */
// [Server]
FVector UJY_HittableComponent::ComputePushVelocity(const AActor* InstigatorActor, const FVector& HitVelocity) const
{
	FVector Direction = HitVelocity.GetSafeNormal();
	if (Direction.IsZero() == true && InstigatorActor != nullptr)
	{
		const FVector FromInstigator = GetOwner()->GetActorLocation() - InstigatorActor->GetActorLocation();
		Direction = FromInstigator.GetSafeNormal2D();
	}

	const float PushSpeed = FMath::Max(HitVelocity.Size() * PushScale, MinPushSpeed);
	return Direction * PushSpeed;
}
