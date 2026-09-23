#include "AbilitySystem/Effects/JY_DebugGameplayEffects.h"
#include "AbilitySystem/Attributes/JY_HealthSet.h"
#include "System/JY_GameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_DebugGameplayEffects)

namespace
{
	/** 메타 속성에 SetByCaller 크기를 AddBase로 더하는 모디파이어 */
	FGameplayModifierInfo MakeSetByCallerModifier(const FGameplayAttribute& Attribute, const FGameplayTag& DataTag)
	{
		FSetByCallerFloat SetByCaller;
		SetByCaller.DataTag = DataTag;

		FGameplayModifierInfo ModifierInfo;
		ModifierInfo.Attribute = Attribute;
		ModifierInfo.ModifierOp = EGameplayModOp::AddBase;
		ModifierInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
		return ModifierInfo;
	}
}

UJY_GameplayEffect_DebugDamage::UJY_GameplayEffect_DebugDamage()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	Modifiers.Add(MakeSetByCallerModifier(UJY_HealthSet::GetDamageAttribute(), JYGameplayTags::SetByCaller_Damage));
}

UJY_GameplayEffect_DebugHeal::UJY_GameplayEffect_DebugHeal()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	Modifiers.Add(MakeSetByCallerModifier(UJY_HealthSet::GetHealingAttribute(), JYGameplayTags::SetByCaller_Heal));
}
