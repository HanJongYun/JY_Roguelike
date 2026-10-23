#include "AbilitySystem/Abilities/JY_GameplayAbility_Attack.h"
#include "AbilitySystemComponent.h"
#include "System/JY_GameplayTags.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_GameplayAbility_Attack)

UJY_GameplayAbility_Attack::UJY_GameplayAbility_Attack(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ActivationBlockedTags.AddTag(JYGameplayTags::State_Dashing);
}

// [Server] [OwnClient]
/* 서버도 같은 검사를 하므로 서버 몽타주가 발동 RPC 지연만큼 늦게 시작해도 클라와 같은 위치에서 판정한다 */
bool UJY_GameplayAbility_Attack::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const //override
{
	if (Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags) == false)
		return false;

	return IsBlockingAttackActive(ActorInfo) == false;
}

bool UJY_GameplayAbility_Attack::IsBlockingOtherAttacks() const
{
	return false;
}

// [Server] [OwnClient]
/* 막기 태그(BlockAbilitiesWithTag)는 실행 내내 고정이라 "끊기 구간 안에서만 허용"을 표현할 수 없다 → 실행 중 공격에 직접 묻는다 */
bool UJY_GameplayAbility_Attack::IsBlockingAttackActive(const FGameplayAbilityActorInfo* ActorInfo)
{
	const UAbilitySystemComponent* AbilitySystemComponent = ActorInfo->AbilitySystemComponent.Get();
	for (const FGameplayAbilitySpec& AbilitySpec : AbilitySystemComponent->GetActivatableAbilities())
	{
		if (AbilitySpec.IsActive() == false)
			continue;

		const UJY_GameplayAbility_Attack* ActiveAttack = Cast<UJY_GameplayAbility_Attack>(AbilitySpec.GetPrimaryInstance());
		if (ActiveAttack == nullptr)
			continue;

		if (ActiveAttack->IsBlockingOtherAttacks() == true)
			return true;
	}
	return false;
}

// [OwnClient]
/* 공중 여부는 클라가 정해 보낸다 — 서버가 자기 낙하 상태로 정하면 지연만큼 어긋난다 */
FGameplayTag UJY_GameplayAbility_Attack::MakeStartTag() const
{
	const ACharacter* Character = CastChecked<ACharacter>(GetAvatarActorFromActorInfo());
	const UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement();
	if (MovementComponent->IsFalling() == true)
		return JYGameplayTags::Ability_Start_Air;

	return FGameplayTag();
}

FTransform UJY_GameplayAbility_Attack::GetAvatarHitOrigin() const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	const FRotator AvatarRotation = AvatarActor->GetActorRotation();
	const FRotator YawRotation(0.0f, AvatarRotation.Yaw, 0.0f);
	const FVector AvatarLocation = AvatarActor->GetActorLocation();
	return FTransform(YawRotation, AvatarLocation);
}
