#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/JY_GameplayAbility_Skill.h"
#include "JY_GameplayAbility_Plunge.generated.h"

class UAnimMontage;
class UAbilityTask_ApplyRootMotionConstantForce;

/* 내려찍기 — 땅: 제자리 도약 후 원형 충격. 공중: 아래로 돌진 → 착지 순간 원형 충격(높을수록 넓음). 데이터는 GA_JY_Skill_Plunge */
UCLASS(Abstract)
class UJY_GameplayAbility_Plunge : public UJY_GameplayAbility_Skill
{
	GENERATED_BODY()

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	/* [Server] [OwnClient] 방향 도착 — 땅/공중 결정·비용·몽타주 */
	void OnDirectionReceived(const FGameplayAbilityTargetDataHandle& DirectionDataHandle, FGameplayTag ApplicationTag);

	/* 공중 — 하강 시작 노티파이 */
	UFUNCTION()
	void OnDiveStartEvent(FGameplayEventData Payload);

	UFUNCTION()
	void OnHitEvent(FGameplayEventData Payload);

	/* 공중 — 착지하면 하강 멈춤, 조종 클라는 착지 섹션으로 */
	UFUNCTION()
	void OnLanded(const FHitResult& Hit);

	UFUNCTION()
	void OnMontageFinished();

	UFUNCTION()
	void OnMontageInterrupted();

protected:
	/* 설정값 — GA_JY_Skill_Plunge 의 Class Defaults */

	/* 땅에서 (AM_JY_Plunge_Ground). 노티파이: Event.Skill.Hit */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Skill")
	TObjectPtr<UAnimMontage> GroundMontage;

	/* 공중에서 (AM_JY_Plunge_Air) — 섹션 Start → Loop(반복) / LandSectionName. 노티파이: Event.Skill.DiveStart(Start), Event.Skill.Hit(착지 섹션) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Skill")
	TObjectPtr<UAnimMontage> AirMontage;

	/* 땅·공중 몽타주 재생 속도 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Skill", Meta = (ClampMin = "0.1"))
	float PlayRate = 1.0f;

	/* 공중 몽타주의 착지 섹션 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Skill")
	FName LandSectionName = TEXT("End");

	/* 충격 판정 (범위 배율 전) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Skill")
	FJY_HitShape HitShape;

	/* 피해 (레벨·카드 배율 전) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Skill")
	float Damage = 40.0f;

	/* 맞은 적을 미는 속도 (cm/s) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Skill")
	float KnockbackSpeed = 1200.0f;

	/* 공중 하강 속도 (cm/s) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Skill", Meta = (ClampMin = "0"))
	float DiveSpeed = 2500.0f;

	/* 이 높이(cm)에서 떨어지면 범위가 최대 배율 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Skill", Meta = (ClampMin = "1"))
	float FallHeightForMaxRadius = 1000.0f;

	/* 낙하 높이로 커지는 범위의 최대 배율 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Skill", Meta = (ClampMin = "1"))
	float MaxFallRadiusScale = 1.5f;

private:
	/* 하강 루트모션 태스크 (착지하면 끝냄) */
	UPROPERTY()
	TObjectPtr<UAbilityTask_ApplyRootMotionConstantForce> DiveTask;

	/* 서버가 방향을 기다리는 구독 */
	FDelegateHandle DirectionDelegateHandle;

	/* 공중에서 시작했는지 */
	bool bAirPlunge = false;

	/* 이번 실행에서 이미 착지했는지 (튕겨 다시 착지해도 섹션을 다시 넘기지 않게) */
	bool bLanded = false;

	/* 공중 시작 높이 (Z) */
	float PlungeStartHeight = 0.0f;
};
