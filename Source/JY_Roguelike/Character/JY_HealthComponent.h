#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "JY_HealthComponent.generated.h"

class UAbilitySystemComponent;
class UJY_HealthSet;
struct FGameplayEffectSpec;

/** 죽음 이벤트 (죽은 액터) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJY_HealthDeathEvent, AActor*, OwningActor);

/** 죽음 진행 단계 */
UENUM(BlueprintType)
enum class EJY_DeathState : uint8
{
	NotDead = 0,
	DeathStarted,
	DeathFinished
};

/** 체력 0을 듣고 죽음 상태를 복제하는 컴포넌트 */
UCLASS(ClassGroup = (JY), meta = (BlueprintSpawnableComponent))
class UJY_HealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UJY_HealthComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** ASC의 체력 세트에 연결 */
	void InitializeWithAbilitySystem(UAbilitySystemComponent* InASC);

	/** 연결 해제 */
	void UninitializeFromAbilitySystem();

	UFUNCTION(BlueprintCallable, Category = "JY|Health")
	EJY_DeathState GetDeathState() const { return DeathState; }

	UFUNCTION(BlueprintCallable, Category = "JY|Health")
	bool IsDeadOrDying() const { return DeathState > EJY_DeathState::NotDead; }

	/** 죽기 시작하면 방송 (서버·클라 모두) */
	UPROPERTY(BlueprintAssignable)
	FJY_HealthDeathEvent OnDeathStarted;

protected:
	virtual void OnUnregister() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void HandleOutOfHealth(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue);

	/** 죽음 시작 처리 */
	void StartDeath();

	UFUNCTION()
	void OnRep_DeathState(EJY_DeathState OldDeathState);

private:
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<const UJY_HealthSet> HealthSet;

	/** 죽음 단계 (복제, ReplicatedUsing=OnRep_DeathState) */
	UPROPERTY(ReplicatedUsing = OnRep_DeathState)
	EJY_DeathState DeathState;
};
