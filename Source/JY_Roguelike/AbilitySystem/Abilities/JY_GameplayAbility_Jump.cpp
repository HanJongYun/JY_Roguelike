#include "AbilitySystem/Abilities/JY_GameplayAbility_Jump.h"
#include "System/JY_GameplayTags.h"
#include "GameFramework/Character.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_GameplayAbility_Jump)

UJY_GameplayAbility_Jump::UJY_GameplayAbility_Jump(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	/* 대시 중 점프 금지 */
	ActivationBlockedTags.AddTag(JYGameplayTags::State_Dashing);
}

/* 2단 점프 횟수·공중 여부는 엔진 판정 그대로 (ACharacter::CanJump → JumpMaxCount) */
bool UJY_GameplayAbility_Jump::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const //override
{
	if (Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags) == false)
		return false;

	const AActor* AvatarActor = ActorInfo->AvatarActor.Get();
	const ACharacter* Character = CastChecked<ACharacter>(AvatarActor);
	return Character->CanJump();
}

// [Server] [OwnClient]
/*
 * 점프 자체는 CMC 몫 — 조종 클라가 Jump()로 점프 플래그를 세우면 이동 RPC와 함께 서버로 가서 서버 CMC가 같은 점프를 한다.
 * 그래서 Jump()는 조종 클라에서만 (LyraGameplayAbility_Jump::CharacterJumpStart와 같음).
 * JumpMaxHoldTime=0이라 플래그는 다음 이동 틱에 엔진이 내린다(ACharacter::ClearJumpInput) → 뗌을 기다리지 않고 바로 끝낸다.
 */
void UJY_GameplayAbility_Jump::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) //override
{
	if (CommitAbility(Handle, ActorInfo, ActivationInfo) == false)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (ActorInfo->IsLocallyControlled() == true)
	{
		AActor* AvatarActor = ActorInfo->AvatarActor.Get();
		ACharacter* Character = CastChecked<ACharacter>(AvatarActor);
		Character->Jump();
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
