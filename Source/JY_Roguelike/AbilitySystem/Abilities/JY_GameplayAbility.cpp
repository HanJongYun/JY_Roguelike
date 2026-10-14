#include "AbilitySystem/Abilities/JY_GameplayAbility.h"
#include "System/JY_GameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_GameplayAbility)

UJY_GameplayAbility::UJY_GameplayAbility(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	/* 어빌리티 안에 상태(태스크·델리게이트 핸들)를 두므로 액터마다 인스턴스 1개 */
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	/* 죽으면 모든 어빌리티 차단 — Status.Death.Dying도 부모 태그 일치로 걸린다(FGameplayTagContainer::HasAny) */
	ActivationBlockedTags.AddTag(JYGameplayTags::Status_Death);
}
