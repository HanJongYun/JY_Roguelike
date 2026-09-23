#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "JY_AbilitySystemComponent.generated.h"

/** 프로젝트 전용 ASC (지금은 비어 있는 확장 지점) */
UCLASS()
class UJY_AbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UJY_AbilitySystemComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
