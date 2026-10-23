#include "AbilitySystem/Tasks/JY_AbilityTask_WaitLanded.h"
#include "GameFramework/Character.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_AbilityTask_WaitLanded)

UJY_AbilityTask_WaitLanded* UJY_AbilityTask_WaitLanded::WaitLanded(UGameplayAbility* OwningAbility)
{
	return NewAbilityTask<UJY_AbilityTask_WaitLanded>(OwningAbility);
}

// [Server] [OwnClient]
void UJY_AbilityTask_WaitLanded::Activate() //override
{
	Super::Activate();

	ACharacter* Character = CastChecked<ACharacter>(GetAvatarActor());
	Character->LandedDelegate.AddDynamic(this, &ThisClass::OnCharacterLanded);
}

void UJY_AbilityTask_WaitLanded::OnCharacterLanded(const FHitResult& Hit)
{
	if (ShouldBroadcastAbilityTaskDelegates() == false)
		return;

	OnLanded.Broadcast(Hit);
}

void UJY_AbilityTask_WaitLanded::OnDestroy(bool bInOwnerFinished) //override
{
	/* 캐릭터가 파괴되면서 끝날 때(PIE 종료·리스폰)는 약한 참조가 이미 비어 있다 */
	ACharacter* Character = Cast<ACharacter>(GetAvatarActor());
	if (Character != nullptr)
	{
		Character->LandedDelegate.RemoveDynamic(this, &ThisClass::OnCharacterLanded);
	}

	Super::OnDestroy(bInOwnerFinished);
}
