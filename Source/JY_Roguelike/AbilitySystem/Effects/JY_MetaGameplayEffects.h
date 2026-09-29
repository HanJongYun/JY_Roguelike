#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "JY_MetaGameplayEffects.generated.h"

/* 메타 강화: 최대 체력 증가 (계속 유지, 크기는 SetByCaller.MetaValue) */
UCLASS()
class UJY_GameplayEffect_MetaMaxHealth : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UJY_GameplayEffect_MetaMaxHealth();
};
