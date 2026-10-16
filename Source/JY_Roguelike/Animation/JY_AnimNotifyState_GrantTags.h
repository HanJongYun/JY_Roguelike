#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameplayTagContainer.h"
#include "JY_AnimNotifyState_GrantTags.generated.h"

/* 노티파이 구간 동안 메시 주인 ASC에 태그 (무적 등) */
UCLASS(Const, HideCategories = Object, CollapseCategories, Meta = (DisplayName = "JY Grant Tags"))
class UJY_AnimNotifyState_GrantTags : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;

protected:
	/* 설정값 — 몽타주 노티파이 디테일 패널 */

	/* 구간 동안 붙일 태그 (State.Dashing·Gameplay.DamageImmunity 등) */
	UPROPERTY(EditAnywhere, Category = "JY")
	FGameplayTagContainer Tags;
};
