#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/JY_GameplayAbility_Skill.h"
#include "JY_GameplayAbility_Plunge.generated.h"

class UAnimMontage;
class UAbilityTask_ApplyRootMotionConstantForce;
class UAbilityTask_ApplyRootMotionJumpForce;

/* 내려찍기 — 땅: 앞으로 높이 도약, 공중: 앞+아래 대각선 돌진 → 둘 다 착지 순간 원형 충격(높을수록 넓음). 데이터는 GA_JY_Skill_Plunge */
UCLASS(Abstract)
class UJY_GameplayAbility_Plunge : public UJY_GameplayAbility_Skill
{
	GENERATED_BODY()

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	/* [OwnClient] 공중 하강 — 땅까지 걸릴 시간을 재서 착지 순간 SlamPoseTime이 되게 재생 속도 조절 */
	void MatchSlamToLanding(const FVector& DiveVelocity);

	/* [Server] [OwnClient] 방향 도착 — 땅/공중 결정·비용·몽타주·땅 도약 */
	void OnDirectionReceived(const FGameplayAbilityTargetDataHandle& DirectionDataHandle, FGameplayTag ApplicationTag);

	/* 공중 — 하강 시작 노티파이 */
	UFUNCTION()
	void OnDiveStartEvent(FGameplayEventData Payload);

	UFUNCTION()
	void OnHitEvent(FGameplayEventData Payload);

	/* 착지하면 하강 멈춤, 조종 클라는 착지 섹션으로 */
	UFUNCTION()
	void OnLanded(const FHitResult& Hit);

	UFUNCTION()
	void OnMontageFinished();

	UFUNCTION()
	void OnMontageInterrupted();

protected:
	/* 설정값 — GA_JY_Skill_Plunge 의 Class Defaults */

	/* 땅·공중 공용 (AM_JY_Plunge_Air) — 섹션 Start → Loop(반복) / LandSectionName. 노티파이: Event.Skill.DiveStart(Start, 공중만 사용), Event.Skill.SlamPose(칼이 땅을 치는 프레임, 착지와 맞춤), Event.Skill.Hit(착지 섹션) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Skill")
	TObjectPtr<UAnimMontage> PlungeMontage;

	/* 몽타주 재생 속도 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Skill", Meta = (ClampMin = "0.1"))
	float PlayRate = 1.0f;

	/* 높은 곳에서 떨어질 때 내려찍는 동작이 느려지는 한계 (재생 속도) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Skill", Meta = (ClampMin = "0.05"))
	float MinSlamPlayRate = 0.3f;

	/* 몽타주의 착지 섹션 */
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

	/* 땅 도약 — 최고 높이 (cm) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Skill", Meta = (ClampMin = "0"))
	float LeapHeight = 350.0f;

	/* 땅 도약 — 앞으로 가는 거리 (cm) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Skill", Meta = (ClampMin = "0"))
	float LeapDistance = 500.0f;

	/* 땅 도약 — 뛰어올라 같은 높이로 돌아오기까지 시간 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Skill", Meta = (ClampMin = "0.05"))
	float LeapDuration = 0.45f;

	/* 공중 하강 — 아래로 가는 속도 (cm/s) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Skill", Meta = (ClampMin = "0"))
	float DiveSpeed = 2500.0f;

	/* 공중 하강 — 앞으로 가는 속도 (cm/s) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Skill", Meta = (ClampMin = "0"))
	float DiveForwardSpeed = 900.0f;

	/* 공중 하강 — 바닥을 찾는 거리 = 하강 속도 × 이 시간 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Skill", Meta = (ClampMin = "0.1"))
	float FallTraceSeconds = 2.0f;

	/* 이 높이(cm)에서 떨어지면 범위가 최대 배율 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Skill", Meta = (ClampMin = "1"))
	float FallHeightForMaxRadius = 1000.0f;

	/* 낙하 높이로 커지는 범위의 최대 배율 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Skill", Meta = (ClampMin = "1"))
	float MaxFallRadiusScale = 1.5f;

private:
	/* 공중 하강 루트모션 태스크 (착지하면 끝냄) */
	UPROPERTY()
	TObjectPtr<UAbilityTask_ApplyRootMotionConstantForce> DiveTask;

	/* 땅 도약 루트모션 태스크 (착지하면 스스로 끝남) */
	UPROPERTY()
	TObjectPtr<UAbilityTask_ApplyRootMotionJumpForce> LeapTask;

	/* 서버가 방향을 기다리는 구독 */
	FDelegateHandle DirectionDelegateHandle;

	/* 공중에서 시작했는지 */
	bool bAirPlunge = false;

	/* 이번 실행에서 이미 착지했는지 (튕겨 다시 착지해도 섹션을 다시 넘기지 않게) */
	bool bLanded = false;

	/* 몽타주의 Event.Skill.SlamPose 노티파이 시각 (초) — 실행마다 몽타주에서 읽음 */
	float SlamPoseTime = 0.0f;

	/* 시작 높이 (Z) */
	float PlungeStartHeight = 0.0f;

	/* 내려찍는 방향 (수평) */
	FVector PlungeDirection = FVector::ZeroVector;
};
