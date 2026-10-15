#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "JY_BladeTrap.generated.h"

class UStaticMeshComponent;

/* 흔들리는 도끼 — 진자처럼 오가며 날에 닿은 모두(플레이어·적)에게 피해. 통로를 가로지르게 놓아 박자 맞춰 지나가게 함 */
UCLASS()
class AJY_BladeTrap : public AActor
{
	GENERATED_BODY()

public:
	AJY_BladeTrap();

protected:
	virtual void Tick(float DeltaSeconds) override;

private:
	/* 날에 닿은 대상에게 피해 — 같은 대상은 HitCooldownSeconds 동안 다시 안 맞음 (서버) */
	void ApplyBladeDamage(double Now);

private:
	/* 회전 중심 (매다는 곳) */
	UPROPERTY(VisibleAnywhere, Category = "JY|Trap")
	TObjectPtr<USceneComponent> Pivot;

	/* 천장 걸이 */
	UPROPERTY(VisibleAnywhere, Category = "JY|Trap")
	TObjectPtr<UStaticMeshComponent> Hanger;

	/* 도끼 — Pivot 기준으로 앞뒤(X)로 흔들린다 */
	UPROPERTY(VisibleAnywhere, Category = "JY|Trap")
	TObjectPtr<UStaticMeshComponent> Blade;

	/* 설정값 — BP_JY_BladeTrap의 Class Defaults에서 수정 (박자 어긋내기만 배치한 액터마다) */

	/* 한 번 왕복 시간 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Trap", Meta = (ClampMin = "0.5"))
	float SwingPeriodSeconds = 2.4f;

	/* 한쪽 최대 각도 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Trap", Meta = (ClampMin = "0.0", ClampMax = "89.0"))
	float SwingAngleDegrees = 60.0f;

	/* 회전 중심에서 날 가운데까지 거리 (cm, Blade 메시 상대) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Trap", Meta = (ClampMin = "0.0"))
	float BladeReach = 380.0f;

	/* 날 판정 반지름 (cm) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Trap", Meta = (ClampMin = "0.0"))
	float HitRadius = 110.0f;

	UPROPERTY(EditDefaultsOnly, Category = "JY|Trap", Meta = (ClampMin = "0.0"))
	float Damage = 20.0f;

	/* 같은 대상을 다시 때리기까지 (초) — 날이 한 번 지나가는 동안 여러 번 맞지 않게 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Trap", Meta = (ClampMin = "0.0"))
	float HitCooldownSeconds = 0.8f;

	/* 이 함정의 박자 어긋냄 (초) */
	UPROPERTY(EditInstanceOnly, Category = "JY|Trap", Meta = (ClampMin = "0.0"))
	float PhaseOffsetSeconds = 0.0f;

private:
	/* 대상별 마지막으로 맞은 시각 (서버) */
	TMap<TWeakObjectPtr<AActor>, double> LastHitTimes;
};
