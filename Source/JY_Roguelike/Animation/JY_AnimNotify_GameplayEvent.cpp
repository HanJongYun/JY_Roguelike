#include "Animation/JY_AnimNotify_GameplayEvent.h"
#include "AbilitySystem/JY_AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/SkeletalMeshComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_AnimNotify_GameplayEvent)

// [Server] [OwnClient] [SimulatedProxy] — 몽타주가 도는 모든 곳
/*
 * 엔진엔 GameplayCue용 노티파이만 있고 이벤트용은 없다(Lyra도 C++ 없음).
 * 데디 서버에서도 돈다 — FAnimNotifyEvent::bTriggerOnDedicatedServer 기본 true(AnimTypes.h:383) + 캐릭터 메시 AlwaysTickPose.
 * 듣는 어빌리티가 없으면(적·시뮬 프록시) 아무 일도 없다.
 */
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

/* 시각을 미리 알아야 할 때(재생 속도 맞춤 등) 쓴다 — 노티파이가 몽타주 시간축에 저장된 값(FAnimNotifyEvent::GetTriggerTime)을 읽기만 함 */
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
