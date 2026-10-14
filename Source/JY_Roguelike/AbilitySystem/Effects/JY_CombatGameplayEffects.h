#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "JY_CombatGameplayEffects.generated.h"

/* 전투 피해 — Damage 메타 속성 += SetByCaller.Damage (UJY_HealthSet이 Health로 바꿈) */
UCLASS()
class UJY_GameplayEffect_Damage : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UJY_GameplayEffect_Damage();
};
