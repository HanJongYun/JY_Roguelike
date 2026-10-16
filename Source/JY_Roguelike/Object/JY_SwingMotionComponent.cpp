#include "Object/JY_SwingMotionComponent.h"
#include "Object/JY_ObjectClock.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_SwingMotionComponent)

/*
 * Tick 사용: 흔들리는 움직임을 매 프레임 그려야 하고, 방당 0~1개라 비용이 작다.
 * 데디 서버도 돌린다 — 날 위치로 범위 피해(Area Damage의 CenterPartTag)를 판정하므로 서버에도 같은 회전이 필요하다
 */
UJY_SwingMotionComponent::UJY_SwingMotionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

// [Server] [Client]
void UJY_SwingMotionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) //override
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	/* BP가 붙인 부품이라 저장하지 않고 찾는다 */
	TArray<UActorComponent*> Parts = GetOwner()->GetComponentsByTag(USceneComponent::StaticClass(), PartTag);
	if (Parts.Num() == 0)
		return;

	const double Now = JY_ObjectClock::GetSyncedSeconds(GetWorld());
	const double CycleTime = FMath::Fmod(Now + PhaseOffsetSeconds, static_cast<double>(PeriodSeconds));
	const float Phase = static_cast<float>(CycleTime) / PeriodSeconds;
	const float SwingAngle = AngleDegrees * FMath::Sin(Phase * UE_TWO_PI);

	/* 피치 = Y축 회전 → 부품이 X 방향으로 오간다 */
	CastChecked<USceneComponent>(Parts[0])->SetRelativeRotation(FRotator(SwingAngle, 0.0f, 0.0f));
}
