#include "Trap/JY_BladeTrap.h"
#include "Combat/JY_CombatQuerySubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Trap/JY_TrapClock.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_BladeTrap)

/*
 * 각도를 서버 시각(JY_TrapClock)의 사인으로 계산해 복제하지 않는다 — 서버는 피해만, 클라는 흔들림만 각자 그린다.
 * Tick 사용: 흔들리는 움직임을 매 프레임 그려야 하고, 방당 0~1개라 비용이 작다.
 */
AJY_BladeTrap::AJY_BladeTrap()
{
	PrimaryActorTick.bCanEverTick = true;

	Pivot = CreateDefaultSubobject<USceneComponent>(TEXT("Pivot"));
	SetRootComponent(Pivot);

	Hanger = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Hanger"));
	Hanger->SetupAttachment(Pivot);
	Hanger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Hanger->SetCanEverAffectNavigation(false);

	/* 피해는 날 위치 범위 판정으로 주므로 몸을 막지 않는다 */
	Blade = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Blade"));
	Blade->SetupAttachment(Pivot);
	Blade->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Blade->SetCanEverAffectNavigation(false);
}

// [Server] [SimulatedProxy]
void AJY_BladeTrap::Tick(float DeltaSeconds) //override
{
	Super::Tick(DeltaSeconds);

	const double Now = JY_TrapClock::GetSyncedSeconds(GetWorld());
	const double CycleTime = FMath::Fmod(Now + PhaseOffsetSeconds, static_cast<double>(SwingPeriodSeconds));
	const float Phase = static_cast<float>(CycleTime) / SwingPeriodSeconds;
	const float SwingAngle = SwingAngleDegrees * FMath::Sin(Phase * UE_TWO_PI);

	/* 피치 = Y축 회전 → 날이 X 방향으로 오간다. 서버도 날 위치 계산에 같은 회전이 필요해 항상 돌린다 */
	Blade->SetRelativeRotation(FRotator(SwingAngle, 0.0f, 0.0f));

	if (HasAuthority() == false)
		return;

	ApplyBladeDamage(Now);
}

// [Server]
void AJY_BladeTrap::ApplyBladeDamage(double Now)
{
	UWorld* World = GetWorld();
	const FVector BladeCenter = Blade->GetComponentTransform().TransformPosition(FVector(0.0f, 0.0f, -BladeReach));

	const FCollisionObjectQueryParams ObjectQueryParams(ECC_Pawn);
	const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(JY_BladeTrap), false, this);
	const FCollisionShape QuerySphere = FCollisionShape::MakeSphere(HitRadius);
	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(OUT Overlaps, BladeCenter, FQuat::Identity, ObjectQueryParams, QuerySphere, QueryParams);
	if (Overlaps.Num() == 0)
		return;

	const UJY_CombatQuerySubsystem* CombatQuery = World->GetSubsystem<UJY_CombatQuerySubsystem>();
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Actor = Overlap.GetActor();

		/* 캡슐·메시가 둘 다 Pawn 채널이라 같은 프레임에 두 번 걸려도 이 대기 시간이 막는다 */
		const double* LastHitTime = LastHitTimes.Find(Actor);
		if (LastHitTime != nullptr && Now - *LastHitTime < HitCooldownSeconds)
			continue;

		const bool bDamaged = CombatQuery->ApplyEnvironmentDamage(Actor, Damage, nullptr, BladeCenter);
		if (bDamaged == true)
		{
			LastHitTimes.Add(Actor, Now);
		}
	}
}
