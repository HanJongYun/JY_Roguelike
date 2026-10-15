#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GenericTeamAgentInterface.h"
#include "JY_EnemyCharacter.generated.h"

class UJY_AbilitySystemComponent;
class UJY_HealthSet;
class UJY_HealthComponent;
class UAnimMontage;

/* 죽은 뒤 처리 */
UENUM()
enum class EJY_EnemyDeathBehavior : uint8
{
	/* 죽음 애니 후 제거 */
	Destroy,
	/* 죽음 애니 후 같은 자리에 새로 생성 (훈련용 허수아비) */
	Respawn
};

/* 맞은 방향별 반응 몽타주 */
USTRUCT(BlueprintType)
struct FJY_HitReactMontages
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UAnimMontage> Front;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UAnimMontage> Back;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UAnimMontage> Left;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UAnimMontage> Right;

	/* 큰 피해(LargeHitDamage 이상)일 때 */
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UAnimMontage> Large;
};

/* Actor 적 공용 부모 — 자체 ASC·체력·죽음·피격 반응 (허수아비·보스). Mass 적과는 별개 */
UCLASS(Abstract)
class AJY_EnemyCharacter : public ACharacter, public IAbilitySystemInterface, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	AJY_EnemyCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void BeginPlay() override;

public:
	//~IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//~IGenericTeamAgentInterface
	virtual FGenericTeamId GetGenericTeamId() const override;

public:
	/* [Server] 밀려나기 (저항만큼 줄인 속도로 KnockbackDuration 동안) */
	void ReceiveKnockback(const FVector2f& Impulse);

	/* [OwnClient] [SimulatedProxy] 피격 반응 재생 — 피격 큐가 부름 (연출 전용) */
	void PlayHitReact(const FVector& HitFromLocation, float Damage);

private:
	/* 죽음 시작 — 충돌 끄기·죽음 애니, 서버는 정리 예약 */
	UFUNCTION()
	void OnDeathStarted(AActor* OwningActor);

	/* [Server] 죽음 애니 뒤 제거 또는 새로 생성 */
	void FinishDeath();

private:
	UPROPERTY(VisibleAnywhere, Category = "JY|Enemy")
	TObjectPtr<UJY_AbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<const UJY_HealthSet> HealthSet;

	UPROPERTY(VisibleAnywhere, Category = "JY|Enemy")
	TObjectPtr<UJY_HealthComponent> HealthComponent;

private:
	/* 설정값 — BP_JY_TrainingDummy 등 BP의 Class Defaults */

	UPROPERTY(EditDefaultsOnly, Category = "JY|Enemy")
	float StartingHealth = 100.0f;

	/* 밀려남 저항 (0 = 그대로, 1 = 안 밀림) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Enemy", Meta = (ClampMin = "0", ClampMax = "1"))
	float KnockbackResistance = 0.0f;

	/* 밀려나는 시간 (초). 거리 = 미는 속도 × 이 시간 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Enemy", Meta = (ClampMin = "0.05"))
	float KnockbackDuration = 0.25f;

	UPROPERTY(EditDefaultsOnly, Category = "JY|Enemy|Hit")
	FJY_HitReactMontages HitReactMontages;

	/* 이 피해 이상이면 Large 반응 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Enemy|Hit")
	float LargeHitDamage = 30.0f;

	/* 무작위로 하나 재생 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Enemy|Death")
	TArray<TObjectPtr<UAnimMontage>> DeathMontages;

	UPROPERTY(EditDefaultsOnly, Category = "JY|Enemy|Death")
	EJY_EnemyDeathBehavior DeathBehavior = EJY_EnemyDeathBehavior::Destroy;

	/* 죽기 시작 → FinishDeath까지 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Enemy|Death", Meta = (ClampMin = "0.1"))
	float DeathLingerSeconds = 3.0f;

private:
	/* 처음 놓인 위치 — Respawn 자리 */
	FTransform SpawnTransform;

	/* 지금 밀려나는 루트모션 소스 (없으면 0) — 다시 맞으면 바꿔 끼운다 */
	uint16 KnockbackRootMotionSourceID = 0;
};
