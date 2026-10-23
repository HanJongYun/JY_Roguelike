#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "JY_AbilityTask_WaitLanded.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJY_WaitLandedDelegate, const FHitResult&, Hit);

/* 아바타 캐릭터가 착지할 때마다 알림 (서버·조종 클라 각자 자기 이동 기준) */
UCLASS()
class UJY_AbilityTask_WaitLanded : public UAbilityTask
{
	GENERATED_BODY()

public:
	virtual void Activate() override;

protected:
	virtual void OnDestroy(bool bInOwnerFinished) override;

public:
	static UJY_AbilityTask_WaitLanded* WaitLanded(UGameplayAbility* OwningAbility);

private:
	UFUNCTION()
	void OnCharacterLanded(const FHitResult& Hit);

public:
	/* 델리게이트 */

	UPROPERTY(BlueprintAssignable)
	FJY_WaitLandedDelegate OnLanded;
};
