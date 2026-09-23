#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "JY_DebugGameplayEffects.generated.h"

/** 디버그용 즉시 데미지 (크기는 SetByCaller.Damage) */
UCLASS()
class UJY_GameplayEffect_DebugDamage : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UJY_GameplayEffect_DebugDamage();
};

/** 디버그용 즉시 회복 (크기는 SetByCaller.Heal) */
UCLASS()
class UJY_GameplayEffect_DebugHeal : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UJY_GameplayEffect_DebugHeal();
};
