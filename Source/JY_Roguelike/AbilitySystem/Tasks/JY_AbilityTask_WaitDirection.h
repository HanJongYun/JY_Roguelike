#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "JY_AbilityTask_WaitDirection.generated.h"

struct FGameplayAbilityTargetDataHandle;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FJY_WaitDirectionDelegate, const FVector&, Direction, FGameplayTag, ApplicationTag);

/* 조종 클라가 정한 수평 방향을 서버와 주고받기 — 조종 클라는 보낸 즉시, 서버는 도착하면 알림 */
UCLASS()
class UJY_AbilityTask_WaitDirection : public UAbilityTask
{
	GENERATED_BODY()

public:
	virtual void Activate() override;

protected:
	virtual void OnDestroy(bool bInOwnerFinished) override;

public:
	/* bAcceptRepeated = 실행 중 방향을 여러 번 받는지 (콤보 다음 타). false면 첫 방향만 */
	static UJY_AbilityTask_WaitDirection* WaitDirection(UGameplayAbility* OwningAbility, bool bAcceptRepeated);

	/* [OwnClient] 서버로 보내고 이 클라에도 바로 알림. 예측 창은 부르는 쪽이 연다 */
	void SendDirection(const FVector& Direction, FGameplayTag ApplicationTag);

private:
	/* [Server] [OwnClient] 방향 도착 */
	void OnDirectionReceived(const FGameplayAbilityTargetDataHandle& DirectionDataHandle, FGameplayTag ApplicationTag);

	/* 서버 구독 끊기 */
	void StopListening();

public:
	/* 델리게이트 */

	/* 방향 도착 (서버에선 네트워크로 온 값이라 0 벡터일 수 있다) */
	UPROPERTY(BlueprintAssignable)
	FJY_WaitDirectionDelegate OnDirection;

private:
	/* 구독 키 — 실행 동안 고정 */
	FGameplayAbilitySpecHandle SpecHandle;

	FPredictionKey ActivationPredictionKey;

	FDelegateHandle DirectionDelegateHandle;

	bool bAcceptRepeated = false;
};
