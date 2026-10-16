#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "JY_PeriodicActivatorComponent.generated.h"

/* 주기 발동 — 서버 시각 기준 일정 주기마다 물체를 발동시킨다 (가시) */
UCLASS(ClassGroup = (JY), meta = (BlueprintSpawnableComponent))
class UJY_PeriodicActivatorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UJY_PeriodicActivatorComponent();

protected:
	virtual void BeginPlay() override;

private:
	// [Server]
	void HandleCycle();

private:
	/* 설정값 — BP 컴포넌트 디테일에서 수정 (박자 어긋내기만 배치한 물체마다) */

	/* 한 주기 (초) — 물체의 PreActive + Active + InActive 시간보다 길게 */
	UPROPERTY(EditAnywhere, Category = "JY|Periodic", Meta = (ClampMin = "0.5"))
	float CycleSeconds = 3.0f;

	/* 이 물체의 박자 어긋냄 (초) — 나란히 놓은 가시를 번갈아 솟게 */
	UPROPERTY(EditAnywhere, Category = "JY|Periodic", Meta = (ClampMin = "0.0"))
	float PhaseOffsetSeconds = 0.0f;

private:
	FTimerHandle CycleTimerHandle;
};
