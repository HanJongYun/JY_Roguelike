#include "AbilitySystem/Effects/JY_SkillGameplayEffects.h"
#include "System/JY_GameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_SkillGameplayEffects)

UJY_GameplayEffect_SkillCooldown::UJY_GameplayEffect_SkillCooldown()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	/* 적용 시점에 길이를 다시 계산하므로 적용 전에 넣은 SetByCaller가 반영된다(GameplayEffect.cpp:4429) */
	FSetByCallerFloat CooldownSetByCaller;
	CooldownSetByCaller.DataTag = JYGameplayTags::SetByCaller_Cooldown;
	DurationMagnitude = FGameplayEffectModifierMagnitude(CooldownSetByCaller);
}
