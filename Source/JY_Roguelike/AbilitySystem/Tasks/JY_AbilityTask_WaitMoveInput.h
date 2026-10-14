#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "JY_AbilityTask_WaitMoveInput.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FJY_WaitMoveInputDelegate);

/* 이동 입력이 있는 동안 매 프레임 알림 (조종 클라 전용) */
UCLASS()
class UJY_AbilityTask_WaitMoveInput : public UAbilityTask
{
	GENERATED_BODY()

public:
	UJY_AbilityTask_WaitMoveInput(const FObjectInitializer& ObjectInitializer);

public:
	virtual void TickTask(float DeltaTime) override;

public:
	static UJY_AbilityTask_WaitMoveInput* WaitMoveInput(UGameplayAbility* OwningAbility);

public:
	/* 델리게이트 */

	UPROPERTY(BlueprintAssignable)
	FJY_WaitMoveInputDelegate OnMoveInput;
};
