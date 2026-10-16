#include "Animation/JY_AnimNotify_GameplayEvent.h"
#include "AbilitySystem/JY_AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/SkeletalMeshComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_AnimNotify_GameplayEvent)

// [Server] [OwnClient] [SimulatedProxy] — 몽타주가 도는 모든 곳
/* 데디 서버에서도 돈다 — bTriggerOnDedicatedServer 기본 true + 메시 AlwaysTickPose (AnimTypes.h:383) */
void UJY_AnimNotify_GameplayEvent::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) //override
{
	Super::Notify(MeshComp, Animation, EventReference);

	AActor* OwnerActor = MeshComp->GetOwner();
	UAbilitySystemComponent* OwnerASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerActor);
	UJY_AbilitySystemComponent* OwnerJYASC = Cast<UJY_AbilitySystemComponent>(OwnerASC);

	/* 애니 편집기 미리보기 메시는 주인에게 ASC가 없다 */
	if (OwnerJYASC == nullptr)
		return;

	FGameplayEventData EventData;
	EventData.EventTag = EventTag;
	EventData.Instigator = OwnerActor;
	OwnerJYASC->HandleGameplayEvent(EventTag, &EventData);
}

/* 재생 속도 맞춤처럼 시각을 미리 알아야 할 때 */
bool UJY_AnimNotify_GameplayEvent::FindEventTime(const UAnimSequenceBase* Animation, FGameplayTag Tag, float& OutEventTime)
{
	for (const FAnimNotifyEvent& NotifyEvent : Animation->Notifies)
	{
		const UJY_AnimNotify_GameplayEvent* EventNotify = Cast<UJY_AnimNotify_GameplayEvent>(NotifyEvent.Notify);
		if (EventNotify == nullptr || EventNotify->EventTag.MatchesTagExact(Tag) == false)
			continue;

		OutEventTime = NotifyEvent.GetTriggerTime();
		return true;
	}

	return false;
}

FString UJY_AnimNotify_GameplayEvent::GetNotifyName_Implementation() const //override
{
	if (EventTag.IsValid() == false)
		return Super::GetNotifyName_Implementation();

	return EventTag.ToString();
}
