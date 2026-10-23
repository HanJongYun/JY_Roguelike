#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/JY_GameplayAbility.h"
#include "JY_GameplayAbility_MeleeCombo.generated.h"

class UJY_MeleeComboData;

/* 기본 공격 — 지상·공중 콤보. 입력 구간에 누르면 커서 방향으로 다음 타 */
UCLASS(Abstract)
class UJY_GameplayAbility_MeleeCombo : public UJY_GameplayAbility
{
	GENERATED_BODY()

public:
	UJY_GameplayAbility_MeleeCombo(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	/* 방향 도착 — 첫 도착은 콤보 시작, 이후는 다음 타 방향 저장 */
	void OnDirectionReceived(const FGameplayAbilityTargetDataHandle& DirectionDataHandle, FGameplayTag ApplicationTag);

	/* 지금 몽타주 섹션의 타 번호 (Recover 섹션이면 INDEX_NONE) */
	int32 GetCurrentStepIndex() const;

	/* 각 타 시작 직후 — 예약된 방향으로 몸 돌리기 */
	UFUNCTION()
	void OnStepStartEvent(FGameplayEventData Payload);

	/* 타격 시점 — 판정·피해 */
	UFUNCTION()
	void OnHitEvent(FGameplayEventData Payload);

	UFUNCTION()
	void OnMontageFinished();

	UFUNCTION()
	void OnMontageInterrupted();

	/* 후딜레이 중 이동 입력이면 끝내기 */
	UFUNCTION()
	void OnMoveInput();

	/* 공중 콤보 중 착지하면 끝내기 */
	UFUNCTION()
	void OnLanded(const FHitResult& Hit);

protected:
	/* 설정값 — GA_JY_MeleeCombo의 Class Defaults */

	/* 땅에서 시작한 콤보 (DA_JY_Combo_Ground) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Combo")
	TObjectPtr<const UJY_MeleeComboData> GroundCombo;

	/* 공중에서 시작한 콤보 (DA_JY_Combo_Air) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Combo")
	TObjectPtr<const UJY_MeleeComboData> AirCombo;

	/* 휘두른 방향을 잴 뼈 — 부서지는 물체가 이 뼈가 움직인 방향으로 날아감 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Combo")
	FName SwingBoneName = TEXT("hand_r");

	/* 타격 순간보다 이만큼 앞 시점과 비교해 휘두른 방향을 잰다 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Combo", Meta = (ClampMin = "0.01"))
	float SwingSampleSeconds = 0.05f;

private:
	/* 이번 실행의 콤보 (첫 방향의 ApplicationTag로 결정) */
	UPROPERTY()
	TObjectPtr<const UJY_MeleeComboData> ActiveCombo;

	/* 서버가 방향을 받는 구독 (실행 내내 유지) */
	FDelegateHandle DirectionDelegateHandle;

	/* 다음 타 시작 때 바라볼 방향 (가장 최근 요청, 없으면 0) */
	FVector NextStepDirection = FVector::ZeroVector;

	/* 이번 타에 다음 타를 이미 예약했는지 (연타 중복 방지) */
	bool bNextStepQueued = false;

	/* 지금 타 번호 — 각 타 시작 노티파이마다 1 증가 (타격 노티파이가 어느 타인지) */
	int32 ActiveStepIndex = INDEX_NONE;
};
