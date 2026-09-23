#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "JY_AttributeSet.generated.h"

class UJY_AbilitySystemComponent;
struct FGameplayEffectSpec;

/** 속성 변화 알림 (가해자, 원인 액터, 효과 명세, 변화량, 이전 값, 새 값) */
DECLARE_MULTICAST_DELEGATE_SixParams(FJY_AttributeEvent, AActor* /*EffectInstigator*/, AActor* /*EffectCauser*/, const FGameplayEffectSpec* /*EffectSpec*/, float /*EffectMagnitude*/, float /*OldValue*/, float /*NewValue*/);

/** 프로젝트 속성 세트 공용 부모 */
UCLASS(Abstract)
class UJY_AttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UJY_AttributeSet();

	virtual UWorld* GetWorld() const override;

	/** 이 속성 세트를 가진 ASC */
	UJY_AbilitySystemComponent* GetJYAbilitySystemComponent() const;
};
