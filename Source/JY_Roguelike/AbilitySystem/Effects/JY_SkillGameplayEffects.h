#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "JY_SkillGameplayEffects.generated.h"

/* 스킬 쿨타임 — 길이 = SetByCaller.Cooldown, 태그는 스킬 GA가 스펙에 넣음 */
UCLASS()
class UJY_GameplayEffect_SkillCooldown : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UJY_GameplayEffect_SkillCooldown();
};
