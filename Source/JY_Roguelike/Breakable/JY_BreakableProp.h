#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "JY_BreakableProp.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;
class UStaticMeshComponent;

/* 부서질 때 주변에 주는 효과의 종류 */
UENUM()
enum class EJY_BreakEffect : uint8
{
	/* 그냥 부서짐 (항아리·상자·통) */
	None,

	/* 점화 시간 뒤 폭발 — 범위 안 모두(플레이어·적)에게 피해, 범위 안 다른 부서지는 물체도 부숨 (폭발 통) */
	Explosion,

	/* 독구름 — 범위 안 모두에게 일정 시간 지속 피해 (독 항아리) */
	Poison
};

/* 부서지는 물체의 상태 — 서버가 정하고 복제 */
UENUM()
enum class EJY_BreakableState : uint8
{
	Intact,

	/* 폭발 통이 맞고 터지기 전 */
	Fusing,

	Broken
};

/* 한 대 맞으면 부서지는 월드 물체 — 종류(항아리·상자·통·폭발 통·독 항아리)는 BP_JY_Breakable_* 설정값 */
UCLASS()
class AJY_BreakableProp : public AActor
{
	GENERATED_BODY()

public:
	AJY_BreakableProp();

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	/* 공격·폭발에 맞음 (서버만). 이미 점화·부서졌으면 무시 */
	void BreakBy(AActor* InstigatorActor);

	bool IsIntact() const { return State == EJY_BreakableState::Intact; }

private:
	void SetState(EJY_BreakableState NewState);

	/* 상태에 맞게 겉모습·충돌·연출을 바꿈 (서버·클라 공통) */
	void HandleStateChanged();

	UFUNCTION()
	void OnRep_State();

	void Detonate();
	void ApplyPoisonTick();
	void StopPoison();
	void StopPoisonCloudEffect();

private:
	UPROPERTY(VisibleAnywhere, Category = "JY|Breakable")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/* 멀쩡할 때 계속 도는 표시 연출 (폭발 통 붉은 불티 등) — 시스템은 BP 컴포넌트에서 지정, 비우면 안 보임 */
	UPROPERTY(VisibleAnywhere, Category = "JY|Breakable")
	TObjectPtr<UNiagaraComponent> IdleEffect;

	/* 복제 값 — ReplicatedUsing=OnRep_State: 클라가 부서짐 연출을 각자 재생 */
	UPROPERTY(ReplicatedUsing = OnRep_State)
	EJY_BreakableState State = EJY_BreakableState::Intact;

	/* 설정값 — BP_JY_Breakable_*의 Class Defaults에서 수정 */

	UPROPERTY(EditDefaultsOnly, Category = "JY|Breakable")
	EJY_BreakEffect BreakEffect = EJY_BreakEffect::None;

	/* 부서질 때 그 자리에 한 번 재생 (먼지·파편) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Breakable")
	TObjectPtr<UNiagaraSystem> BreakFX;

	/* 맞은 뒤 터지기까지 시간 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Breakable", Meta = (EditCondition = "BreakEffect == EJY_BreakEffect::Explosion", EditConditionHides, ClampMin = "0.0"))
	float FuseSeconds = 1.0f;

	/* 점화 중 붙는 연출 (불꽃 튐) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Breakable", Meta = (EditCondition = "BreakEffect == EJY_BreakEffect::Explosion", EditConditionHides))
	TObjectPtr<UNiagaraSystem> FuseFX;

	/* 폭발·독구름 연출 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Breakable", Meta = (EditCondition = "BreakEffect != EJY_BreakEffect::None", EditConditionHides))
	TObjectPtr<UNiagaraSystem> AreaFX;

	/* 폭발·독구름 반지름 (cm) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Breakable", Meta = (EditCondition = "BreakEffect != EJY_BreakEffect::None", EditConditionHides, ClampMin = "0.0"))
	float EffectRadius = 350.0f;

	/* 폭발은 한 번, 독은 한 틱마다 주는 피해 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Breakable", Meta = (EditCondition = "BreakEffect != EJY_BreakEffect::None", EditConditionHides, ClampMin = "0.0"))
	float EffectDamage = 30.0f;

	/* 독구름이 남는 시간 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Breakable", Meta = (EditCondition = "BreakEffect == EJY_BreakEffect::Poison", EditConditionHides, ClampMin = "0.0"))
	float PoisonDurationSeconds = 4.0f;

	/* 독 피해 간격 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Breakable", Meta = (EditCondition = "BreakEffect == EJY_BreakEffect::Poison", EditConditionHides, ClampMin = "0.1"))
	float PoisonTickSeconds = 0.5f;

private:
	/* 부순 쪽 — 폭발·독 피해의 원인으로 남긴다 (연쇄 폭발도 처음 부순 쪽을 이어받음) */
	TWeakObjectPtr<AActor> BreakInstigator;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> FuseEffect;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> PoisonCloudEffect;

	FTimerHandle FuseTimerHandle;
	FTimerHandle PoisonTickTimerHandle;
	FTimerHandle PoisonEndTimerHandle;
	FTimerHandle PoisonCloudTimerHandle;
};
