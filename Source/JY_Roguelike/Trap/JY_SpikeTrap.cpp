#include "Trap/JY_SpikeTrap.h"
#include "Combat/JY_CombatQuerySubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Trap/JY_TrapClock.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_SpikeTrap)

/*
 * 박자를 서버 시각(JY_TrapClock)으로 계산해 복제하지 않는다 — 서버는 피해만, 클라는 가시 움직임만 각자 그린다.
 * Tick 사용: 가시가 솟는 움직임을 매 프레임 그려야 하고, 방당 0~2개라 비용이 작다.
 */
AJY_SpikeTrap::AJY_SpikeTrap()
{
	PrimaryActorTick.bCanEverTick = true;

	Grate = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Grate"));
	SetRootComponent(Grate);
	Grate->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Grate->SetCanEverAffectNavigation(false);

	/* 피해는 범위 판정으로 주므로 가시는 몸을 막지 않는다 */
	Spikes = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Spikes"));
	Spikes->SetupAttachment(Grate);
	Spikes->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Spikes->SetCanEverAffectNavigation(false);
	Spikes->SetRelativeLocation(FVector(0.0f, 0.0f, HiddenHeight));
}

// [Server] [SimulatedProxy]
void AJY_SpikeTrap::Tick(float DeltaSeconds) //override
{
	Super::Tick(DeltaSeconds);

	const double Now = JY_TrapClock::GetSyncedSeconds(GetWorld());
	const double CycleTimeDouble = FMath::Fmod(Now + PhaseOffsetSeconds, static_cast<double>(CycleSeconds));
	const float CycleTime = static_cast<float>(CycleTimeDouble);

	if (IsNetMode(NM_DedicatedServer) == false)
	{
		const FVector CurrentLocation = Spikes->GetRelativeLocation();
		const float TargetHeight = GetTargetSpikeHeight(CycleTime);
		const float NewHeight = FMath::FInterpTo(CurrentLocation.Z, TargetHeight, DeltaSeconds, SpikeMoveSpeed);
		Spikes->SetRelativeLocation(FVector(0.0f, 0.0f, NewHeight));
	}

	if (HasAuthority() == false)
		return;

	if (IsSpikeRaised(CycleTime) == false || Now < NextDamageTime)
		return;

	NextDamageTime = Now + DamageIntervalSeconds;
	const UJY_CombatQuerySubsystem* CombatQuery = GetWorld()->GetSubsystem<UJY_CombatQuerySubsystem>();
	const FVector Center = GetActorLocation() + FVector(0.0f, 0.0f, DamageRadius * 0.5f);
	CombatQuery->ApplyAreaDamage(Center, DamageRadius, Damage, nullptr, this);
}

float AJY_SpikeTrap::GetTargetSpikeHeight(float CycleTime) const
{
	if (CycleTime < WarningSeconds)
		return WarningHeight;

	if (IsSpikeRaised(CycleTime) == true)
		return RaisedHeight;

	return HiddenHeight;
}

bool AJY_SpikeTrap::IsSpikeRaised(float CycleTime) const
{
	return CycleTime >= WarningSeconds && CycleTime < WarningSeconds + RaisedSeconds;
}
