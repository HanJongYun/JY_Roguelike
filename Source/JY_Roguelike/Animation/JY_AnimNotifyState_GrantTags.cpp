#include "Animation/JY_AnimNotifyState_GrantTags.h"
#include "AbilitySystem/JY_AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/SkeletalMeshComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_AnimNotifyState_GrantTags)

namespace JY_AnimNotifyState_GrantTags
{
	UJY_AbilitySystemComponent* FindOwnerAbilitySystem(const USkeletalMeshComponent* MeshComp)
	{
		const AActor* OwnerActor = MeshComp->GetOwner();
		UAbilitySystemComponent* OwnerASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerActor);
		return Cast<UJY_AbilitySystemComponent>(OwnerASC);
	}
}

// [Server] [OwnClient]
/* 느슨한 태그 — 대시 GA의 무적과 같은 방식이라 UJY_HealthSet::PreGameplayEffectExecute 무적 검사가 그대로 먹는다 */
void UJY_AnimNotifyState_GrantTags::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) //override
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	/* 애니 편집기 미리보기 메시는 주인에게 ASC가 없다 */
	UJY_AbilitySystemComponent* OwnerJYASC = JY_AnimNotifyState_GrantTags::FindOwnerAbilitySystem(MeshComp);
	if (OwnerJYASC == nullptr)
		return;

	OwnerJYASC->AddLooseGameplayTags(Tags);
}

// [Server] [OwnClient]
/* 몽타주가 중간에 끊겨도 엔진이 활성 구간에 종료를 보낸다(AnimInstance.cpp:1951) → 태그가 남지 않음 */
void UJY_AnimNotifyState_GrantTags::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) //override
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	UJY_AbilitySystemComponent* OwnerJYASC = JY_AnimNotifyState_GrantTags::FindOwnerAbilitySystem(MeshComp);
	if (OwnerJYASC == nullptr)
		return;

	OwnerJYASC->RemoveLooseGameplayTags(Tags);
}

FString UJY_AnimNotifyState_GrantTags::GetNotifyName_Implementation() const //override
{
	if (Tags.IsEmpty() == true)
		return Super::GetNotifyName_Implementation();

	return Tags.ToStringSimple();
}
