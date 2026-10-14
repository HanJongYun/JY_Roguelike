#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "JY_AnimNotify_GameplayEvent.generated.h"

class UAnimSequenceBase;

/* 몽타주 시점에 메시 주인 ASC로 GameplayEvent 보내기 (타격·콤보 타 시작 등) */
UCLASS(Const, HideCategories = Object, CollapseCategories, Meta = (DisplayName = "JY Gameplay Event"))
class UJY_AnimNotify_GameplayEvent : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;

public:
	/* 애니에서 이 태그 이벤트 노티파이가 놓인 시각(초)을 찾음 — 없으면 false */
	static bool FindEventTime(const UAnimSequenceBase* Animation, FGameplayTag Tag, float& OutEventTime);

protected:
	/* 설정값 — 몽타주 노티파이 디테일 패널 */

	/* 보낼 이벤트 (Event.Melee.Hit 등) */
	UPROPERTY(EditAnywhere, Category = "JY", Meta = (Categories = "Event"))
	FGameplayTag EventTag;
};
