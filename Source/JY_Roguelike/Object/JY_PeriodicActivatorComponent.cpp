#include "Object/JY_PeriodicActivatorComponent.h"
#include "Object/JY_Object.h"
#include "Object/JY_ObjectClock.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_PeriodicActivatorComponent)

UJY_PeriodicActivatorComponent::UJY_PeriodicActivatorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

/* 주기의 시작을 서버 시각에 맞춘다 — 같은 박자 설정의 물체끼리 언제 켜져도 같이 움직인다 */
// [Server] [Client]
void UJY_PeriodicActivatorComponent::BeginPlay() //override
{
	Super::BeginPlay();

	if (GetOwner()->HasAuthority() == false)
		return;

	const double Now = JY_ObjectClock::GetSyncedSeconds(GetWorld());
	const double CycleTime = FMath::Fmod(Now + PhaseOffsetSeconds, static_cast<double>(CycleSeconds));
	const float FirstDelay = static_cast<float>(CycleSeconds - CycleTime);
	GetWorld()->GetTimerManager().SetTimer(CycleTimerHandle, this, &ThisClass::HandleCycle, CycleSeconds, true, FirstDelay);
}

// [Server]
void UJY_PeriodicActivatorComponent::HandleCycle()
{
	GetOwner<AJY_Object>()->RequestActivate(nullptr);
}
