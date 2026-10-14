#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/JY_GameplayAbility.h"
#include "JY_GameplayAbility_Dash.generated.h"

class UAnimMontage;
struct FGameplayAbilityTargetDataHandle;

/* 이동 입력 방향으로 구르기. 루트모션 몽타주, 횟수 소모·충전, 실행 중 무적. 몽타주는 GA_JY_Dash에서 지정 */
UCLASS(Abstract)
class UJY_GameplayAbility_Dash : public UJY_GameplayAbility
{
	GENERATED_BODY()

public:
	UJY_GameplayAbility_Dash(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	/* 조종 클라가 정한 대시 방향 (이동 입력, 없으면 바라보는 방향) */
	FVector ComputeDashDirection() const;

	/* 방향이 준비되면 실행 — 조종 클라는 바로, 서버는 클라가 보낸 방향이 도착했을 때 */
	void OnDashDirectionReady(const FGameplayAbilityTargetDataHandle& DirectionDataHandle, FGameplayTag ApplicationTag);

	/* 충전 시간 뒤 횟수 +1 예약 */
	void ScheduleRecharge();

	/* 몽타주가 정상으로 끝남 */
	UFUNCTION()
	void OnDashMontageFinished();

	/* 몽타주가 끊김 */
	UFUNCTION()
	void OnDashMontageInterrupted();

protected:
	/* 설정값 — GA_JY_Dash의 Class Defaults에서 수정 */

	/* 구르기 몽타주 (루트모션, DefaultSlot) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Dash")
	TObjectPtr<UAnimMontage> DashMontage;

	/* 루트모션 이동 거리 배율 (1 = 애니 그대로) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Dash")
	float RootMotionScale = 1.0f;

	/* 쓴 뒤 1회가 다시 차는 시간 (초). 0 이하면 타이머가 실행되지 않아 하한을 둔다 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Dash", Meta = (ClampMin = "0.1"))
	float RechargeSeconds = 1.0f;

private:
	/* 구르는 동안 붙이는 태그 (대시 중·무적) */
	FGameplayTagContainer RollingTags;

	/* 서버가 방향 도착을 기다리는 구독 */
	FDelegateHandle DirectionReadyDelegateHandle;

	/* 이번 실행에서 RollingTags를 붙였는지 (끝날 때 뗄지) */
	bool bRollingTagsAdded = false;
};
