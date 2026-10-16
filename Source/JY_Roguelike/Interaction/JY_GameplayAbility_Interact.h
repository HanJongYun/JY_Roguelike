#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/JY_GameplayAbility.h"
#include "JY_GameplayAbility_Interact.generated.h"

/* E키 상호작용 — 이벤트에 실린 대상을 쓴 플레이어 화면과 서버에서 각각 실행 */
UCLASS()
class UJY_GameplayAbility_Interact : public UJY_GameplayAbility
{
	GENERATED_BODY()

public:
	UJY_GameplayAbility_Interact(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
