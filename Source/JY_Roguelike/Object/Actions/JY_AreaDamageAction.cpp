#include "Object/Actions/JY_AreaDamageAction.h"
#include "Object/JY_Object.h"
#include "Combat/JY_CombatQuerySubsystem.h"
#include "Interfaces/JY_HitReactable.h"
#include "Interfaces/JY_InterfaceUtils.h"
#include "Components/SceneComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_AreaDamageAction)

// [Server]
void UJY_AreaDamageAction::OnServerEnter(AJY_Object* Object) //override
{
	OwnerObject = Object;
	ApplyDamage();

	if (IntervalSeconds <= 0.0f)
		return;

	GetWorld()->GetTimerManager().SetTimer(DamageTimerHandle, this, &ThisClass::ApplyDamage, IntervalSeconds, true);
}

// [Server]
void UJY_AreaDamageAction::OnServerExit(AJY_Object* Object) //override
{
	GetWorld()->GetTimerManager().ClearTimer(DamageTimerHandle);
	LastHitTimes.Reset();
}

FVector UJY_AreaDamageAction::GetCenter() const
{
	const USceneComponent* CenterPart = FindPart<USceneComponent>(OwnerObject.Get(), CenterPartTag);
	if (CenterPart == nullptr)
		return OwnerObject->GetActorLocation();

	return CenterPart->GetComponentTransform().TransformPosition(CenterOffset);
}

/*
 * 피해 원인 = 발동시킨 쪽 — 폭발 통 처치 기록이 부순 플레이어에게 남는다.
 * 전투 창구의 환경 피해로 보내 대시 무적 등 같은 규칙을 적용
 */
// [Server]
void UJY_AreaDamageAction::ApplyDamage()
{
	AJY_Object* Object = OwnerObject.Get();
	if (Object == nullptr)
		return;

	UWorld* World = GetWorld();
	const FVector Center = GetCenter();
	AActor* InstigatorActor = Object->GetActivationInstigator();

	const FCollisionObjectQueryParams ObjectQueryParams(ECC_Pawn);
	const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(JY_AreaDamage), false, Object);
	const FCollisionShape QuerySphere = FCollisionShape::MakeSphere(Radius);
	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(OUT Overlaps, Center, FQuat::Identity, ObjectQueryParams, QuerySphere, QueryParams);

	const UJY_CombatQuerySubsystem* CombatQuery = World->GetSubsystem<UJY_CombatQuerySubsystem>();
	const double Now = World->GetTimeSeconds();

	/* 캡슐·메시가 둘 다 Pawn 채널이라 같은 액터가 여러 번 걸린다 */
	TSet<const AActor*> CheckedActors;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Actor = Overlap.GetActor();
		bool bAlreadyChecked = false;
		CheckedActors.Add(Actor, &bAlreadyChecked);
		if (bAlreadyChecked == true)
			continue;

		const double* LastHitTime = LastHitTimes.Find(Actor);
		if (RehitSeconds > 0.0f && LastHitTime != nullptr && Now - *LastHitTime < RehitSeconds)
			continue;

		const bool bDamaged = CombatQuery->ApplyEnvironmentDamage(Actor, Damage, InstigatorActor, Center);
		if (bDamaged == true && RehitSeconds > 0.0f)
		{
			LastHitTimes.Add(Actor, Now);
		}
	}

	if (bHitObjects == true)
	{
		HitObjects(Center);
	}
}

/* 폭발 통끼리는 점화 시간(ActivationDelay)만큼 차례로 터진다 */
// [Server]
void UJY_AreaDamageAction::HitObjects(const FVector& Center) const
{
	AJY_Object* Object = OwnerObject.Get();
	const FCollisionObjectQueryParams ObjectQueryParams(ECC_WorldDynamic);
	const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(JY_AreaDamageObjects), false, Object);
	const FCollisionShape QuerySphere = FCollisionShape::MakeSphere(Radius);
	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByObjectType(OUT Overlaps, Center, FQuat::Identity, ObjectQueryParams, QuerySphere, QueryParams);

	TSet<const AActor*> CheckedActors;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Actor = Overlap.GetActor();
		bool bAlreadyChecked = false;
		CheckedActors.Add(Actor, &bAlreadyChecked);
		if (bAlreadyChecked == true)
			continue;

		IJY_HitReactable* HitReactable = JY_FindInterface<IJY_HitReactable>(Actor);
		if (HitReactable == nullptr || HitReactable->CanReceiveHit() == false)
			continue;

		const FVector FromCenter = Actor->GetActorLocation() - Center;
		const FVector PushVelocity = FromCenter.GetSafeNormal() * ObjectPushSpeed;
		HitReactable->ReceiveHit(Object->GetActivationInstigator(), Damage, PushVelocity);
	}
}
