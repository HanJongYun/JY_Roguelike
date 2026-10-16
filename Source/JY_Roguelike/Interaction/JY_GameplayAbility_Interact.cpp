#include "Interaction/JY_GameplayAbility_Interact.h"
#include "Interfaces/JY_InteractableTarget.h"
#include "Interfaces/JY_InterfaceUtils.h"
#include "System/JY_GameplayTags.h"
#include "GameFramework/Pawn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_GameplayAbility_Interact)

/* 예측 발동 — 조종 클라가 이벤트로 발동하면 이벤트 데이터째 서버도 발동 (AbilitySystemComponent_Abilities.cpp:1937) */
UJY_GameplayAbility_Interact::UJY_GameplayAbility_Interact(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	FAbilityTriggerData InteractTrigger;
	InteractTrigger.TriggerTag = JYGameplayTags::Event_Interact;
	InteractTrigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(InteractTrigger);

	/* 죽는 중엔 못 씀 */
	ActivationBlockedTags.AddTag(JYGameplayTags::Status_Death);
}

/*
 * 화면 열기(기록판·제단)는 쓴 플레이어 화면만, 맵 이동(포털)은 서버만.
 * 서버는 클라의 "가까이 있다" 판정을 믿지 않고 거리를 다시 확인한다 — 개조 클라가 먼 대상을 보내도 거절
 */
// [Server] [OwnClient]
void UJY_GameplayAbility_Interact::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) //override
{
	if (TriggerEventData == nullptr || CommitAbility(Handle, ActorInfo, ActivationInfo) == false)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AActor* TargetActor = const_cast<AActor*>(TriggerEventData->Target.Get());
	IJY_InteractableTarget* Target = JY_FindInterface<IJY_InteractableTarget>(TargetActor);
	APawn* User = Cast<APawn>(ActorInfo->AvatarActor.Get());
	if (Target != nullptr && User != nullptr)
	{
		if (IsLocallyControlled() == true)
		{
			Target->InteractLocal(User);
		}

		if (HasAuthority(&ActivationInfo) == true && Target->IsWithinInteractRange(User) == true)
		{
			Target->InteractServer(User);
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
