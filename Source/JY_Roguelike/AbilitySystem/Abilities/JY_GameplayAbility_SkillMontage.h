#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/JY_GameplayAbility_Skill.h"
#include "JY_GameplayAbility_SkillMontage.generated.h"

class UAnimMontage;

/* 몽타주 하나 + 타격 이벤트 순서별 판정 모양 (회전베기 등). 커서 방향으로 시작 */
UCLASS(Abstract)
class UJY_GameplayAbility_SkillMontage : public UJY_GameplayAbility_Skill
{
	GENERATED_BODY()

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	/* 이번 타격의 판정 모양. InOutOrigin = 아바타 위치·방향 (발도 돌진은 경로 상자로 바꿈) */
	virtual FJY_HitShape MakeHitShapeForEvent(int32 EventIndex, FTransform& InOutOrigin) const;

	/* [Server] [OwnClient] 방향이 정해지고 몽타주를 틀기 직전 */
	virtual void OnSkillStarted();

private:
	/* [Server] [OwnClient] 방향 도착 — 비용·몸 돌리기·몽타주 */
	void OnDirectionReceived(const FGameplayAbilityTargetDataHandle& DirectionDataHandle, FGameplayTag ApplicationTag);

	UFUNCTION()
	void OnHitEvent(FGameplayEventData Payload);

	UFUNCTION()
	void OnMontageFinished();

	UFUNCTION()
	void OnMontageInterrupted();

protected:
	/* 설정값 — GA_JY_Skill_* 의 Class Defaults */

	/* 스킬 몽타주 (루트모션, DefaultSlot). 노티파이: Event.Skill.Hit */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Skill")
	TObjectPtr<UAnimMontage> Montage;

	UPROPERTY(EditDefaultsOnly, Category = "JY|Skill")
	float PlayRate = 1.0f;

	/* 시작 섹션 (비우면 처음부터) — 애니 앞부분 준비 동작을 건너뛸 때 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Skill")
	FName StartSectionName;

	/* 타격 이벤트 순서대로 (모자라면 마지막 것 반복) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Skill")
	TArray<FJY_HitShape> HitShapes;

	/* 타격 1회 피해 (레벨·카드 배율 전) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Skill")
	float Damage = 30.0f;

	/* 맞은 적을 미는 속도 (cm/s) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Skill")
	float KnockbackSpeed = 400.0f;

private:
	/* 이번 실행에서 몇 번째 타격인지 */
	int32 HitEventIndex = 0;

	/* 서버가 방향을 기다리는 구독 */
	FDelegateHandle DirectionDelegateHandle;
};
