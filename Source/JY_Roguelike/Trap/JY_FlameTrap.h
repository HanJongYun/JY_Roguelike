#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "JY_FlameTrap.generated.h"

class UBoxComponent;
class UNiagaraComponent;
class UStaticMeshComponent;

/* 화염 압력판의 상태 — 서버가 정하고 복제 */
UENUM()
enum class EJY_FlameTrapState : uint8
{
	/* 밟히길 기다림 */
	Idle,

	/* 밟힘 — 잠깐 뒤 불이 솟음 (피할 틈) */
	Arming,

	Burning,

	/* 다시 밟혀도 안 켜짐 */
	Cooldown
};

/* 화염 압력판 — 누구든(플레이어·적) 밟으면 잠시 뒤 불기둥이 솟아 위에 선 모두에게 지속 피해. 적을 끌어들여 쓰는 함정 */
UCLASS()
class AJY_FlameTrap : public AActor
{
	GENERATED_BODY()

public:
	AJY_FlameTrap();

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UFUNCTION()
	void HandleTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void SetState(EJY_FlameTrapState NewState);

	/* 상태에 맞게 발판 높이·연출을 바꿈 (서버·클라 공통) */
	void HandleStateChanged();

	UFUNCTION()
	void OnRep_State();

	void StartBurning();
	void ApplyFlameDamage();
	void StartCooldown();
	void FinishCooldown();

	/* 지금 발판 위에 폰이 있으면 다시 켬 — 대기 중에 올라와 계속 서 있던 경우 */
	void RetriggerIfOccupied();

private:
	/* 발판 테두리 */
	UPROPERTY(VisibleAnywhere, Category = "JY|Trap")
	TObjectPtr<UStaticMeshComponent> Frame;

	/* 눌리는 발판 */
	UPROPERTY(VisibleAnywhere, Category = "JY|Trap")
	TObjectPtr<UStaticMeshComponent> Plate;

	/* 밟힘 감지 */
	UPROPERTY(VisibleAnywhere, Category = "JY|Trap")
	TObjectPtr<UBoxComponent> Trigger;

	/* 밟힌 뒤 경고 연출 (불티) — 시스템은 BP 컴포넌트에서 지정 */
	UPROPERTY(VisibleAnywhere, Category = "JY|Trap")
	TObjectPtr<UNiagaraComponent> ArmingEffect;

	/* 불기둥 — 시스템은 BP 컴포넌트에서 지정 */
	UPROPERTY(VisibleAnywhere, Category = "JY|Trap")
	TObjectPtr<UNiagaraComponent> FlameEffect;

	/* 복제 값 — ReplicatedUsing=OnRep_State: 클라가 발판·불 연출을 각자 재생 */
	UPROPERTY(ReplicatedUsing = OnRep_State)
	EJY_FlameTrapState State = EJY_FlameTrapState::Idle;

	/* 설정값 — BP_JY_FlameTrap의 Class Defaults에서 수정 */

	/* 밟힌 뒤 불이 솟기까지 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Trap", Meta = (ClampMin = "0.0"))
	float ArmSeconds = 0.6f;

	UPROPERTY(EditDefaultsOnly, Category = "JY|Trap", Meta = (ClampMin = "0.1"))
	float BurnSeconds = 1.5f;

	/* 불이 꺼진 뒤 다시 켜질 수 있기까지 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Trap", Meta = (ClampMin = "0.0"))
	float CooldownSeconds = 2.5f;

	/* 한 틱 피해 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Trap", Meta = (ClampMin = "0.0"))
	float Damage = 10.0f;

	UPROPERTY(EditDefaultsOnly, Category = "JY|Trap", Meta = (ClampMin = "0.1"))
	float DamageIntervalSeconds = 0.3f;

	/* 불 피해 반지름 (cm, 액터 배율 무관) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Trap", Meta = (ClampMin = "0.0"))
	float DamageRadius = 140.0f;

	/* 밟혔을 때 발판이 내려가는 깊이 (Plate 상대 Z) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Trap")
	float PressedPlateHeight = -3.0f;

private:
	FTimerHandle StateTimerHandle;
	FTimerHandle DamageTimerHandle;
};
