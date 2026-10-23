#include "AbilitySystem/Abilities/JY_GameplayAbility_Jump.h"
#include "System/JY_GameplayTags.h"
#include "GameFramework/Character.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_GameplayAbility_Jump)

UJY_GameplayAbility_Jump::UJY_GameplayAbility_Jump(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	FGameplayTagContainer JumpTags;
	JumpTags.AddTag(JYGameplayTags::Ability_Type_Jump);
	SetAssetTags(JumpTags);

	/* 대시 중 점프 금지 */
	ActivationBlockedTags.AddTag(JYGameplayTags::State_Dashing);

	/* 점프는 공격을 끊는다 */
	CancelAbilitiesWithTag.AddTag(JYGameplayTags::Ability_Type_Attack);
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
 * Jump()는 조종 클라에서만 — 점프 플래그가 이동 RPC로 서버 CMC에 전달된다
 * JumpMaxHoldTime=0이라 플래그는 엔진이 다음 이동 틱에 내린다(ACharacter::ClearJumpInput) → 바로 끝낸다
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
