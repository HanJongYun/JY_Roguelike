#include "AbilitySystem/Effects/JY_CombatGameplayEffects.h"
#include "AbilitySystem/Attributes/JY_HealthSet.h"
#include "System/JY_GameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_CombatGameplayEffects)

/* 디버그 피해(UJY_GameplayEffect_DebugDamage)와 나눈 이유: 나중에 전투 피해에만 치명타·속성 배율 계산이 붙는다 */
UJY_GameplayEffect_Damage::UJY_GameplayEffect_Damage()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FSetByCallerFloat DamageSetByCaller;
	DamageSetByCaller.DataTag = JYGameplayTags::SetByCaller_Damage;

	FGameplayModifierInfo DamageModifier;
	DamageModifier.Attribute = UJY_HealthSet::GetDamageAttribute();
	DamageModifier.ModifierOp = EGameplayModOp::AddBase;
	DamageModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(DamageSetByCaller);
	Modifiers.Add(DamageModifier);
}
