#include "Object/Actions/JY_AreaDamageAction.h"
#include "Object/JY_Object.h"
#include "Combat/JY_CombatQuerySubsystem.h"
#include "Interfaces/JY_HitReactable.h"
#include "Interfaces/JY_InterfaceUtils.h"
#include "Components/SceneComponent.h"
#include "HAL/IConsoleManager.h"
#include "Engine/World.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_AreaDamageAction)

namespace JY_AreaDamage
{
	static TAutoConsoleVariable<int32> CVarShowRanges(TEXT("jy.Debug.ShowRanges"), 0, TEXT("1이면 함정·폭발 피해 범위를 그린다 (클라)"));

	/* 범위 그리기 갱신 간격 (초) */
	static constexpr float DebugDrawInterval = 0.1f;
}

// [Server] [Client]
void UJY_AreaDamageAction::Initialize(AJY_Object* Object) //override
{
	OwnerObject = Object;

#if !UE_BUILD_SHIPPING
	/* 시험용 — 데디 서버 그림은 클라 화면에 안 보여 클라에서 그린다. 출시 빌드에선 빠진다 */
	if (Object->IsNetMode(NM_DedicatedServer) == false)
	{
		GetWorld()->GetTimerManager().SetTimer(DebugDrawTimerHandle, this, &ThisClass::DrawDebugRange, JY_AreaDamage::DebugDrawInterval, true);
	}
#endif
}

// [Client]
/* 도끼처럼 중심 부품이 움직여도 따라가게 매번 다시 그린다. 실제로는 대상 캡슐 반지름만큼 더 멀리서 맞는다 */
void UJY_AreaDamageAction::DrawDebugRange() const
{
	if (JY_AreaDamage::CVarShowRanges.GetValueOnGameThread() == 0 || OwnerObject.IsValid() == false)
		return;

	UJY_CombatQuerySubsystem::DrawHitShape(GetWorld(), GetScaledHitShape(), GetHitOrigin(), false, JY_AreaDamage::DebugDrawInterval * 1.5f);
}

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

/* 판은 가로로 놓여 있어 가로(X·Y) 배율 중 큰 쪽을 따른다 — 엔진 콜리전도 배율을 곱한다 (SphereComponent.h:71). 높이는 그대로 */
FJY_HitShape UJY_AreaDamageAction::GetScaledHitShape() const
{
	const FVector ActorScale = OwnerObject->GetActorScale3D();
	const float HorizontalScale = FMath::Max(FMath::Abs(ActorScale.X), FMath::Abs(ActorScale.Y));

	FJY_HitShape ScaledShape = HitShape;
	ScaledShape.Offset *= HorizontalScale;
	ScaledShape.Radius *= HorizontalScale;
	ScaledShape.Length *= HorizontalScale;
	ScaledShape.Width *= HorizontalScale;
	return ScaledShape;
}

FTransform UJY_AreaDamageAction::GetHitOrigin() const
{
	const FRotator YawRotation(0.0f, OwnerObject->GetActorRotation().Yaw, 0.0f);
	return FTransform(YawRotation, GetCenter());
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
	const FTransform HitOrigin = GetHitOrigin();
	const FJY_HitShape ScaledShape = GetScaledHitShape();
	AActor* InstigatorActor = Object->GetActivationInstigator();

	const UJY_CombatQuerySubsystem* CombatQuery = World->GetSubsystem<UJY_CombatQuerySubsystem>();
	TArray<AActor*> ActorsInShape;
	CombatQuery->FindActorsInShape(ScaledShape, HitOrigin, Object, FCollisionObjectQueryParams(ECC_Pawn), OUT ActorsInShape);
	const double Now = World->GetTimeSeconds();

	FJY_HitPayload HitPayload;
	HitPayload.Damage = Damage;
	HitPayload.HitCueTag = HitCueTag;
	HitPayload.Instigator = InstigatorActor;
	HitPayload.StatusEffect = StatusEffect;
	HitPayload.StatusTickDamage = StatusTickDamage;
	HitPayload.KnockbackHeight = KnockbackHeight;

	for (AActor* Actor : ActorsInShape)
	{
		const double* LastHitTime = LastHitTimes.Find(Actor);
		if (bHitEachTargetOnce == true && LastHitTime != nullptr)
			continue;

		if (RehitSeconds > 0.0f && LastHitTime != nullptr && Now - *LastHitTime < RehitSeconds)
			continue;

		const FVector AwayFromCenter = (Actor->GetActorLocation() - Center).GetSafeNormal2D() * KnockbackSpeed;
		HitPayload.KnockbackImpulse = FVector2f(AwayFromCenter.X, AwayFromCenter.Y);

		/* 무적(대시)으로 못 맞았으면 기록하지 않는다 — 무적이 끝난 뒤 간격에 맞는다 */
		const bool bDamaged = CombatQuery->ApplyEnvironmentDamage(Actor, HitPayload, Center);
		if (bDamaged == true && (bHitEachTargetOnce == true || RehitSeconds > 0.0f))
		{
			LastHitTimes.Add(Actor, Now);
		}
	}

	if (bHitObjects == true)
	{
		HitObjects(ScaledShape, HitOrigin, Center);
	}
}

/* 폭발 통끼리는 점화 시간(ActivationDelay)만큼 차례로 터진다 */
// [Server]
void UJY_AreaDamageAction::HitObjects(const FJY_HitShape& Shape, const FTransform& HitOrigin, const FVector& Center) const
{
	AJY_Object* Object = OwnerObject.Get();
	const UJY_CombatQuerySubsystem* CombatQuery = GetWorld()->GetSubsystem<UJY_CombatQuerySubsystem>();
	TArray<AActor*> ObjectsInShape;
	CombatQuery->FindActorsInShape(Shape, HitOrigin, Object, FCollisionObjectQueryParams(ECC_WorldDynamic), OUT ObjectsInShape);

	for (AActor* Actor : ObjectsInShape)
	{
		IJY_HitReactable* HitReactable = JY_FindInterface<IJY_HitReactable>(Actor);
		if (HitReactable == nullptr || HitReactable->CanReceiveHit() == false)
			continue;

		const FVector FromCenter = Actor->GetActorLocation() - Center;
		const FVector PushVelocity = FromCenter.GetSafeNormal() * ObjectPushSpeed;
		HitReactable->ReceiveHit(Object->GetActivationInstigator(), Damage, PushVelocity);
	}
}
