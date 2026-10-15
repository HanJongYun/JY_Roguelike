#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "JY_SpikeTrap.generated.h"

class UStaticMeshComponent;

/* 바닥 가시 — 일정 주기로 살짝 비친 뒤 솟아 위에 선 모두(플레이어·적)에게 피해 */
UCLASS()
class AJY_SpikeTrap : public AActor
{
	GENERATED_BODY()

public:
	AJY_SpikeTrap();

protected:
	virtual void Tick(float DeltaSeconds) override;

private:
	/* 가시가 목표로 하는 높이 (가시 메시 상대 Z) */
	float GetTargetSpikeHeight(float CycleTime) const;

	bool IsSpikeRaised(float CycleTime) const;

private:
	/* 바닥 철망 */
	UPROPERTY(VisibleAnywhere, Category = "JY|Trap")
	TObjectPtr<UStaticMeshComponent> Grate;

	UPROPERTY(VisibleAnywhere, Category = "JY|Trap")
	TObjectPtr<UStaticMeshComponent> Spikes;

	/* 설정값 — BP_JY_SpikeTrap의 Class Defaults에서 수정 (박자 어긋내기만 배치한 액터마다) */

	/* 한 주기 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Trap", Meta = (ClampMin = "0.5"))
	float CycleSeconds = 3.0f;

	/* 솟기 전 살짝 비치는 경고 시간 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Trap", Meta = (ClampMin = "0.0"))
	float WarningSeconds = 0.6f;

	/* 솟아 있는 시간 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Trap", Meta = (ClampMin = "0.1"))
	float RaisedSeconds = 1.0f;

	/* 숨음·경고·솟음 높이 (가시 메시 상대 Z, 액터 배율 적용 전) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Trap")
	float HiddenHeight = -50.0f;

	UPROPERTY(EditDefaultsOnly, Category = "JY|Trap")
	float WarningHeight = -32.0f;

	UPROPERTY(EditDefaultsOnly, Category = "JY|Trap")
	float RaisedHeight = 0.0f;

	/* 높이가 따라가는 속도 (클수록 빨리 솟음) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Trap", Meta = (ClampMin = "1.0"))
	float SpikeMoveSpeed = 30.0f;

	UPROPERTY(EditDefaultsOnly, Category = "JY|Trap", Meta = (ClampMin = "0.0"))
	float Damage = 15.0f;

	/* 솟아 있는 동안 피해 간격 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Trap", Meta = (ClampMin = "0.1"))
	float DamageIntervalSeconds = 0.5f;

	/* 피해 반지름 (cm, 액터 배율 무관) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Trap", Meta = (ClampMin = "0.0"))
	float DamageRadius = 110.0f;

	/* 이 함정의 박자 어긋냄 (초) — 나란히 놓은 가시를 번갈아 솟게 */
	UPROPERTY(EditInstanceOnly, Category = "JY|Trap", Meta = (ClampMin = "0.0"))
	float PhaseOffsetSeconds = 0.0f;

private:
	/* 다음 피해 시각 (서버) */
	double NextDamageTime = 0.0;
};
