#include "AbilitySystem/Effects/JY_DashGameplayEffects.h"
#include "AbilitySystem/Attributes/JY_DashSet.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_DashGameplayEffects)

namespace
{
	/* 대시 횟수에 고정 값을 더하는 모디파이어. 비용 검사(FActiveGameplayEffectsContainer::CanApplyAttributeModifiers)는 Additive만 보므로 Additive */
	FGameplayModifierInfo MakeDashChargesModifier(float Amount)
	{
		const FScalableFloat ScalableAmount(Amount);

		FGameplayModifierInfo ModifierInfo;
		ModifierInfo.Attribute = UJY_DashSet::GetDashChargesAttribute();
		ModifierInfo.ModifierOp = EGameplayModOp::Additive;
		ModifierInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(ScalableAmount);
		return ModifierInfo;
	}
}

UJY_GameplayEffect_DashCost::UJY_GameplayEffect_DashCost()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	Modifiers.Add(MakeDashChargesModifier(-1.0f));
}

UJY_GameplayEffect_DashRecharge::UJY_GameplayEffect_DashRecharge()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	Modifiers.Add(MakeDashChargesModifier(1.0f));
}
